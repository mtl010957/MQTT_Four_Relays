#include <Homie.h>

#define FW_NAME "four-relays"
#define FW_VERSION "1.0.0"

const unsigned char NUMBER_OF_RELAYS = 4;
const unsigned char RELAY_PINS[NUMBER_OF_RELAYS] = { 13, 12, 14, 16 };

HomieNode relayNode("relay", "switch");

bool relayHandler(const HomieRange& range, const String& value) {
  Serial << "IN RELAYHANDLER" << endl;
  
  if (!range.isRange) return false;  // if it's not a range
  Serial << "Addressing relay " << range.index << endl;

  if (range.index < 1 || range.index > NUMBER_OF_RELAYS) return false;  // if it's not a valid range
  Serial << "Addressing relay " << range.index << endl;

  if (value != "on" && value != "off") return false;  // if the value is not valid
  Serial << "Value " << value << endl;

  bool on = (value == "on");

  digitalWrite(RELAY_PINS[range.index - 1], on ? LOW : HIGH); // Relay board is active low logic
  relayNode.setProperty("relay").setRange(range).send(value);  // Update the state of the relay
  Serial << "Relay " << range.index << " is " << value << endl;

  return true;
}

void setup() {
  for (int i = 0; i < NUMBER_OF_RELAYS; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], HIGH); // Relay board is active low logic
  }

  Serial.begin(115200);
  Serial << "Setup relays" << endl << endl;

  Homie_setFirmware(FW_NAME, FW_VERSION);

  relayNode.advertiseRange("relay", 1, NUMBER_OF_RELAYS).settable(relayHandler);

  Homie.setup();
}

void loop() {
  Homie.loop();
}
