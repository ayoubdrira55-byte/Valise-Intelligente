// Test 1 — LEDs rouge (GPIO25), verte (GPIO27) et jaune Qi (GPIO13)
// Câblage : GPIO -> résistance 220 Ω -> anode LED, cathode -> GND
// Attendu : chenillard rouge -> vert -> jaune.
const int LEDS[] = {25, 27, 13};
const char* NOMS[] = {"rouge", "verte", "jaune (Qi)"};

void setup() {
  Serial.begin(115200);
  for (int p : LEDS) pinMode(p, OUTPUT);
}

void loop() {
  for (int i = 0; i < 3; i++) {
    Serial.printf("LED %s\n", NOMS[i]);
    digitalWrite(LEDS[i], HIGH);
    delay(500);
    digitalWrite(LEDS[i], LOW);
  }
}
