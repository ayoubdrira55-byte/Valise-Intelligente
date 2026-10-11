// Test 2 — Buzzer commandé par transistor BC547 (GPIO26)
// Câblage : GPIO26 -> 1 kΩ -> base BC547 ; émetteur -> GND ;
//           collecteur -> (-) buzzer ; (+) buzzer -> VIN (5 V)
// Attendu : bip de 300 ms toutes les secondes.
const int BUZZER = 26;

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER, OUTPUT);
  Serial.println("Test 2 : buzzer");
}

void loop() {
  tone(BUZZER, 2500);
  delay(300);
  noTone(BUZZER);
  delay(700);
}
