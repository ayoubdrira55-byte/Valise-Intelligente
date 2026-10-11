// Test 6 — Connexion WiFi et écriture dans Firebase Realtime Database
// Remplir les 3 constantes ci-dessous. Attendu : "HTTP 200" et une valeur dans /test de la base.
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

const char* SSID = "Wokwi-GUEST";
const char* MOT_DE_PASSE = "";
const char* FIREBASE_URL = "https://votre-projet-default-rtdb.europe-west1.firebasedatabase.app";

void setup() {
  Serial.begin(115200);
  WiFi.begin(SSID, MOT_DE_PASSE);
  Serial.print("Connexion WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
  Serial.printf(" OK, IP : %s\n", WiFi.localIP().toString().c_str());
}

void loop() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, String(FIREBASE_URL) + "/test.json");
  int code = http.PUT(String("{\"uptime_s\":") + millis() / 1000 + "}");
  Serial.printf("Écriture Firebase -> HTTP %d\n", code);
  http.end();
  delay(5000);
}
