/*
 *  This code switches four relays on a module connected to an ESP8266.
 *
 *  It will connect to MQTT and listens on topic relay/1/+/set
 *  where the '+' is the relay number given by a single digit 1-4
 *  for 'on' and 'off' commands. Every 60 seconds, it will publish the current
 *  state of each of the relays on relay/1/+/state
 *
 *  based on example by
 *  Dimitar Manovski
 *  support@smart-republic.com
 */

#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
// for WiFiManager
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#define TRIGGER_PIN 0 // Forces WiFiManager to reset and ask for new setup

#define deviceName "relay"

#define red_led 0
#define blue_led 2

//define your default values here, if there are different values in config.json, they are overwritten.
char client_name[40] = "ESP8266Client_relay_1";
char mqtt_server[40] = "";
char mqtt_port[6] = "1883";
char mqtt_user[40] = "";
char mqtt_password[40] = "";
char device_set[10] = "1";

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// hard coded based on the relay board and connection design
const unsigned char NUMBER_OF_RELAYS = 4;
const unsigned char RELAY_PINS[NUMBER_OF_RELAYS] = { 13, 12, 14, 16 };

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  

  Serial.print("Switching relay ");
  Serial.println(topic);

  // Get the relay number at third part of the topic
  // rework when changing to web based setup which could support arbitrary
  // topic strings
  String pinString = "";
  int pinIndex = 0;
  int pinNumberPos = 0;
  for (int i = 0; i < 10; i++) {
    if ((char)device_set[i] == '\0') {
      pinNumberPos = i + 7;
      break;
    }
  }
  if (isDigit((char)topic[pinNumberPos])) {
    pinString += (char)topic[pinNumberPos];
  }
  pinIndex = pinString.toInt()-1;
  // Insure the pin index we got is valid
  if (pinIndex >= 0 && pinIndex < NUMBER_OF_RELAYS) {
    Serial.print(String(pinIndex));
    Serial.print(" - ");
    Serial.println(RELAY_PINS[pinIndex]);
  
    // Convert the incoming topic into the state topic
    String stateTopic = deviceName;
    stateTopic.concat("/");
    stateTopic.concat(String(device_set));
    stateTopic.concat("/");
    stateTopic.concat(pinString);
    stateTopic.concat("/state");
    char stateTopicBuf[stateTopic.length()+1];
    stateTopic.toCharArray(stateTopicBuf, stateTopic.length()+1);
    
    if ((char)payload[0] == 'o' && (char)payload[1] == 'n') {
      Serial.println("Turning relay ON");
      digitalWrite(RELAY_PINS[pinIndex], LOW);   // turn the RELAY on
      client.publish(stateTopicBuf, "on", true); // publish with retained flag set to true
    } else if ((char)payload[0] == 'o' && (char)payload[1] == 'f' && (char)payload[2] == 'f') {
      Serial.println("Turning relay OFF");
      digitalWrite(RELAY_PINS[pinIndex], HIGH);    // turn the RELAY off
      client.publish(stateTopicBuf, "off", true); // publish with retained flag set to true
    } else {
      Serial.print("I do not know what to do with ");
      for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
      }
      Serial.print(" on topic ");
      Serial.println(topic);
    }
  } else {
    Serial.print("I do not know what to do with ");
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.print(" on topic ");
    Serial.println(topic);
  }
}


void setup() {
  // Setup the two LED ports to use for signaling status
  pinMode(red_led, OUTPUT);
  pinMode(blue_led, OUTPUT);
  digitalWrite(red_led, HIGH);  
  digitalWrite(blue_led, HIGH);  
  Serial.begin(115200);
  // If the trigger pin is held during setup we will force a WiFi configuration
  // setup instead of using the stored values
  if ( digitalRead(TRIGGER_PIN) == LOW ) {
    setup_wifi(true);
  } else {
    setup_wifi(false);
  }

//  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  // initialize relay pins.
  for (int i = 0; i < NUMBER_OF_RELAYS; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], HIGH);
    pinMode(TRIGGER_PIN, INPUT); // Pin to force wifi setup
  }
}

void blink_red() {
  digitalWrite(red_led, LOW);
  delay(20);
  digitalWrite(red_led, HIGH);  
}

void blink_blue() {
  digitalWrite(blue_led, LOW);
  delay(20);
  digitalWrite(blue_led, HIGH);  
}

void setup_wifi(bool force) {
  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.prettyPrintTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(client_name, json["client_name"]);
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_user, json["mqtt_user"]);
          strcpy(mqtt_password, json["mqtt_password"]);
          strcpy(device_set, json["device_set"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_client_name("client", "client name", client_name, 40);
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, 40);
  WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password, 40);
  WiFiManagerParameter custom_device_set("set", "device set", device_set, 10);
  
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //add all your parameters here
  wifiManager.addParameter(&custom_client_name);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_device_set);

  //reset settings - for testing
  //wifiManager.resetSettings();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  //it starts an access point with the specified name
  //here  "OnDemandAP"
  //and goes into a blocking loop awaiting configuration

  if (force) {
    // Force to start the configuration AP
    //WITHOUT THIS THE AP DOES NOT SEEM TO WORK PROPERLY WITH SDK 1.5 , update to at least 1.5.1
    WiFi.mode(WIFI_STA);
  
    if (!wifiManager.startConfigPortal("OnDemandAP")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }
  } else {
    // Connect with saved settings, only start the configuration AP if connection fails
    wifiManager.autoConnect("OnDemandAP");
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  
  //read updated parameters
  strcpy(client_name, custom_client_name.getValue());
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_password, custom_mqtt_password.getValue());
  strcpy(device_set, custom_device_set.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["client_name"] = client_name;
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_password"] = mqtt_password;
    json["device_set"] = device_set;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.prettyPrintTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.print("local ip ");
  Serial.println(WiFi.localIP());

}

void reconnect() {
  // Loop until we're reconnected
  // Server or port may have been updated
  Serial.println(mqtt_port);
  int mqtt_port_int = String(mqtt_port).toInt();
  Serial.println(mqtt_port_int);
  client.setServer(mqtt_server, mqtt_port_int);
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect(client_name)) {
    if (client.connect(client_name, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      String setTopic = deviceName;
      setTopic.concat("/");
      setTopic.concat(String(device_set));
      setTopic.concat("/+/set");
      char setTopicBuf[setTopic.length()+1];
      setTopic.toCharArray(setTopicBuf, setTopic.length()+1);
      client.subscribe(setTopicBuf);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      blink_red();
      delay(200);
      blink_red();
      delay(4760);
      // If the trigger pin is held this will get us out of a MQTT reconnect loop
      // to set new MQTT information in case the WiFi connection info is good
      // but the MQTT setup needs to be changed
      if ( digitalRead(TRIGGER_PIN) == LOW ) {
        setup_wifi(true);
      }
    }
  }
}

int tellstate = 0;

void loop() {
  // If the trigger pin is held we'll force a new WiFi and MQTT setup cycle
  if ( digitalRead(TRIGGER_PIN) == LOW ) {
    setup_wifi(true);
  }
  
  if (! client.loop()) { // run the client loop and if it fails try to reconnect
    Serial.println("Not connected to MQTT....");
    reconnect();
    delay(5000);
  }

  // Tell the current state every 60 seconds
  if ( (millis() - tellstate) > 60000 ) {
    for (int i = 0; i < NUMBER_OF_RELAYS; i++) {
      String stateTopic = deviceName;
      stateTopic.concat("/");
      stateTopic.concat(String(device_set));
      stateTopic.concat("/");
      stateTopic.concat(String(i + 1));
      stateTopic.concat("/state");
      Serial.print("Refreshing ");
      Serial.println(stateTopic);
      char stateTopicBuf[stateTopic.length()+1];
      stateTopic.toCharArray(stateTopicBuf, stateTopic.length()+1);
      if ( !digitalRead(RELAY_PINS[i]) ) {
        client.publish(stateTopicBuf, "on", true); // publish with retained flag set to true
      } else {
        client.publish(stateTopicBuf, "off", true); // publish with retained flag set to true
      }
      blink_blue();
    }
    tellstate = millis();
  }
}
