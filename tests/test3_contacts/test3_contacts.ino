// Test 3 — Capteur d'ouverture ILS (GPIO33) et interrupteur d'armement (GPIO32)
// Câblage : chaque contact entre la broche et GND (pull-up interne activé).
// Attendu : l'état s'affiche à chaque changement.
const int REED = 33, ARM = 32;
int dernierReed = -1, dernierArm = -1;

void setup() {
  Serial.begin(115200);
  pinMode(REED, INPUT_PULLUP);
  pinMode(ARM, INPUT_PULLUP);
}

void loop() {
  int r = digitalRead(REED), a = digitalRead(ARM);
  if (r != dernierReed || a != dernierArm) {
    Serial.printf("Couvercle : %s | Interrupteur : %s\n",
                  r == HIGH ? "OUVERT" : "fermé", a == LOW ? "ARMÉ" : "désarmé");
    dernierReed = r;
    dernierArm = a;
  }
  delay(50);
}
