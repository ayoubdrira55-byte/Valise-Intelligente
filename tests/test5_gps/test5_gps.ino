// Test 5 — GPS NEO-6M sur Serial2 (TX GPS -> GPIO16, RX GPS -> GPIO17, alimentation 3V3)
// Attendu : les trames NMEA brutes ($GPGGA, $GPRMC...) s'affichent.
// Le premier "fix" peut prendre plusieurs minutes : tester à l'extérieur ou près d'une fenêtre.
void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("Test 5 : lecture des trames NMEA du GPS");
}

void loop() {
  while (Serial2.available()) Serial.write(Serial2.read());
}
