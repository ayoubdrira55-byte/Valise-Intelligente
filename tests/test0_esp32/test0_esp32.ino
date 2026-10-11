// Test 0 — La carte ESP32 fonctionne et communique avec le PC
// Attendu : la LED bleue intégrée (GPIO2) clignote et le moniteur série (115200 bauds) affiche un compteur.
const int LED_INTEGREE = 2;
int compteur = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_INTEGREE, OUTPUT);
  Serial.println("Test 0 : ESP32 OK");
}

void loop() {
  digitalWrite(LED_INTEGREE, !digitalRead(LED_INTEGREE));
  Serial.printf("Battement %d\n", compteur++);
  delay(500);
}
