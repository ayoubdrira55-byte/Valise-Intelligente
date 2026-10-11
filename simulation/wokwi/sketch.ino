// Version tout-en-un pour la simulation Wokwi (https://wokwi.com)
// Générée à partir de firmware/valise_esp32 : configuration intégrée, Firebase désactivé.
// Pour tester Firebase dans Wokwi, passer USE_FIREBASE à 1 et ajouter un onglet secrets.h.

// =====================================================================
//  Valise intelligente connectée — programme principal ESP32
//
//  Fonctions :
//   - Sécurité anti-intrusion : ouverture (capteur ILS) + mouvement (MPU6050)
//   - Alarme sonore (buzzer via transistor BC547) et lumineuse (LED rouge)
//   - Localisation GPS (NEO-6M, ou position simulée)
//   - Indicateur de recharge sans fil (LED jaune à la place du module Qi)
//   - Envoi de l'état vers Firebase Realtime Database (application Flutter)
//
//  Machine à états :  DESARMEE  --(interrupteur)-->  ARMEE
//                     ARMEE     --(ouverture ou mouvement)-->  ALARME
//                     ALARME    --(désarmement)-->  DESARMEE
// =====================================================================
#include <Wire.h>
// =====================================================================
//  Valise intelligente connectée — configuration matérielle
// =====================================================================

// ---------------- Brochage (ESP32 WROOM-32) ----------------
#define PIN_GPS_RX     16   // RX2 de l'ESP32  <- TX du GPS NEO-6M
#define PIN_GPS_TX     17   // TX2 de l'ESP32  -> RX du GPS NEO-6M
#define PIN_SDA        21   // I2C SDA  -> MPU6050
#define PIN_SCL        22   // I2C SCL  -> MPU6050
#define PIN_REED       33   // Capteur d'ouverture (ILS / reed switch), INPUT_PULLUP
#define PIN_ARM        32   // Interrupteur d'armement, INPUT_PULLUP (LOW = armée)
#define PIN_BUZZER     26   // Base du transistor BC547 (via résistance 1 kΩ)
#define PIN_LED_RED    25   // LED rouge : alarme
#define PIN_LED_GREEN  27   // LED verte : système armé
#define PIN_LED_QI     13   // LED jaune : indicateur de recharge sans fil (Qi)

// ---------------- Options de compilation ----------------
// 0 = position GPS simulée (module NEO-6M non disponible)
// 1 = vrai GPS NEO-6M (nécessite la bibliothèque TinyGPSPlus)
#define USE_REAL_GPS   0

// 0 = fonctionnement autonome (sans WiFi)
// 1 = envoi de l'état vers Firebase Realtime Database
#define USE_FIREBASE   0

// Position simulée : centre-ville de Sfax
#define SIM_LAT        34.7406
#define SIM_LON        10.7603

// ---------------- Réglages ----------------
#define MOTION_THRESHOLD_G   0.35f   // écart d'accélération déclenchant l'alarme [g]
#define SEND_PERIOD_MS       5000    // période d'envoi vers Firebase [ms]
#define SENSOR_PERIOD_MS     100     // période de lecture des capteurs [ms]
#define BUZZER_FREQ_HZ       2500    // fréquence de la sirène [Hz]


#if USE_FIREBASE
  #include <WiFi.h>
  #include <WiFiClientSecure.h>
  #include <HTTPClient.h>
  #if __has_include("secrets.h")
    #include "secrets.h"
  #else
    #include "secrets.example.h"
  #endif
#endif

#if USE_REAL_GPS
  #include <TinyGPSPlus.h>
  TinyGPSPlus gps;
#endif

// ---------------------------------------------------------------------
// État global
// ---------------------------------------------------------------------
enum Etat { DESARMEE, ARMEE, ALARME };
const char* NOM_ETAT[] = {"DESARMEE", "ARMEE", "ALARME"};

Etat etat = DESARMEE;
const char* causeAlarme = "";

bool  valiseOuverte = false;
bool  mouvement     = false;
float accelG        = 1.0f;     // norme de l'accélération [g]
float refG          = 1.0f;     // référence filtrée (valise immobile)
double latitude     = SIM_LAT;
double longitude    = SIM_LON;
bool  gpsValide     = false;
bool  chargeQi      = true;     // commande de recharge (modifiable depuis l'app)
bool  mpuPresent    = false;

unsigned long tCapteurs = 0, tEnvoi = 0, tSirene = 0;
bool sireneOn = false;

// ---------------------------------------------------------------------
// MPU6050 (lecture directe I2C, sans bibliothèque)
// ---------------------------------------------------------------------
const uint8_t MPU_ADDR = 0x68;

bool mpuInit() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);  // PWR_MGMT_1
  Wire.write(0x00);  // sortie du mode veille
  return Wire.endTransmission() == 0;
}

bool mpuLireAccel(float &ax, float &ay, float &az) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);  // ACCEL_XOUT_H
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MPU_ADDR, (uint8_t)6) != 6) return false;
  int16_t x = (Wire.read() << 8) | Wire.read();
  int16_t y = (Wire.read() << 8) | Wire.read();
  int16_t z = (Wire.read() << 8) | Wire.read();
  ax = x / 16384.0f;  // échelle ±2 g
  ay = y / 16384.0f;
  az = z / 16384.0f;
  return true;
}

// ---------------------------------------------------------------------
// Capteurs
// ---------------------------------------------------------------------
void lireCapteurs() {
  // ILS : aimant présent (couvercle fermé) -> contact fermé -> LOW
  valiseOuverte = digitalRead(PIN_REED) == HIGH;

  // Détection de mouvement : écart entre l'accélération mesurée et sa moyenne lente
  float ax, ay, az;
  if (mpuPresent && mpuLireAccel(ax, ay, az)) {
    accelG = sqrtf(ax * ax + ay * ay + az * az);
    mouvement = fabsf(accelG - refG) > MOTION_THRESHOLD_G;
    if (!mouvement) refG = 0.95f * refG + 0.05f * accelG;  // filtre passe-bas
  } else {
    mouvement = false;
  }

  // GPS
#if USE_REAL_GPS
  while (Serial2.available()) gps.encode(Serial2.read());
  if (gps.location.isValid()) {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
    gpsValide = true;
  }
#else
  // Petite dérive aléatoire autour de Sfax pour animer la carte de l'application
  latitude  = SIM_LAT + random(-50, 51) * 1e-5;
  longitude = SIM_LON + random(-50, 51) * 1e-5;
  gpsValide = true;
#endif
}

// ---------------------------------------------------------------------
// Machine à états
// ---------------------------------------------------------------------
void changerEtat(Etat nouvel, const char* cause = "");

void majEtat() {
  bool interrupteurArme = digitalRead(PIN_ARM) == LOW;

  switch (etat) {
    case DESARMEE:
      if (interrupteurArme) {
        refG = accelG;  // la position actuelle devient la référence
        changerEtat(ARMEE);
      }
      break;

    case ARMEE:
      if (!interrupteurArme)   changerEtat(DESARMEE);
      else if (valiseOuverte)  changerEtat(ALARME, "ouverture");
      else if (mouvement)      changerEtat(ALARME, "mouvement");
      break;

    case ALARME:
      if (!interrupteurArme) changerEtat(DESARMEE);  // seul le propriétaire arrête l'alarme
      break;
  }
}

// ---------------------------------------------------------------------
// Sorties : LEDs et sirène
// ---------------------------------------------------------------------
void majSorties() {
  digitalWrite(PIN_LED_GREEN, etat == ARMEE);
  digitalWrite(PIN_LED_QI, chargeQi);

  if (etat == ALARME) {
    // Sirène intermittente : 300 ms ON / 200 ms OFF, LED rouge synchronisée
    unsigned long periode = sireneOn ? 300 : 200;
    if (millis() - tSirene >= periode) {
      tSirene = millis();
      sireneOn = !sireneOn;
      if (sireneOn) tone(PIN_BUZZER, BUZZER_FREQ_HZ);
      else          noTone(PIN_BUZZER);
      digitalWrite(PIN_LED_RED, sireneOn);
    }
  } else if (sireneOn || digitalRead(PIN_LED_RED)) {
    sireneOn = false;
    noTone(PIN_BUZZER);
    digitalWrite(PIN_LED_RED, LOW);
  }
}

// ---------------------------------------------------------------------
// Firebase Realtime Database (API REST)
// ---------------------------------------------------------------------
#if USE_FIREBASE
String urlFirebase(const char* chemin) {
  String url = String(FIREBASE_URL) + chemin + ".json";
  if (strlen(FIREBASE_AUTH) > 0) url += String("?auth=") + FIREBASE_AUTH;
  return url;
}

int requeteFirebase(const char* methode, const char* chemin, const String& corps, String* reponse = nullptr) {
  if (WiFi.status() != WL_CONNECTED) return -1;
  WiFiClientSecure client;
  client.setInsecure();  // projet pédagogique : pas de vérification du certificat
  HTTPClient http;
  http.setTimeout(3000);
  if (!http.begin(client, urlFirebase(chemin))) return -1;
  http.addHeader("Content-Type", "application/json");
  int code = http.sendRequest(methode, corps);
  if (reponse && code == 200) *reponse = http.getString();
  http.end();
  return code;
}

void envoyerEtat() {
  char json[320];
  snprintf(json, sizeof(json),
    "{\"etat\":\"%s\",\"ouverte\":%s,\"mouvement\":%s,\"accel_g\":%.2f,"
    "\"gps\":{\"lat\":%.6f,\"lon\":%.6f,\"valide\":%s,\"simule\":%s},"
    "\"charge_qi\":%s,\"uptime_s\":%lu}",
    NOM_ETAT[etat], valiseOuverte ? "true" : "false", mouvement ? "true" : "false", accelG,
    latitude, longitude, gpsValide ? "true" : "false", USE_REAL_GPS ? "false" : "true",
    chargeQi ? "true" : "false", millis() / 1000);
  int code = requeteFirebase("PUT", "/valise/etat", json);
  Serial.printf("[Firebase] envoi état -> HTTP %d\n", code);
}

void lireCommandes() {
  // L'application écrit {"charge_qi": true/false} dans /valise/commandes
  String rep;
  if (requeteFirebase("GET", "/valise/commandes", "", &rep) == 200) {
    if (rep.indexOf("\"charge_qi\":false") >= 0) chargeQi = false;
    else if (rep.indexOf("\"charge_qi\":true") >= 0) chargeQi = true;
  }
}

void journaliser(const char* evenement) {
  char json[160];
  snprintf(json, sizeof(json), "{\"evenement\":\"%s\",\"uptime_s\":%lu,\"lat\":%.6f,\"lon\":%.6f}",
           evenement, millis() / 1000, latitude, longitude);
  requeteFirebase("POST", "/valise/historique", json);
}

void connecterWiFi() {
  Serial.printf("[WiFi] connexion à %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? " OK" : " échec (mode autonome)");
}
#endif

void changerEtat(Etat nouvel, const char* cause) {
  etat = nouvel;
  causeAlarme = cause;
  Serial.printf("[Etat] %s %s\n", NOM_ETAT[etat], cause);
#if USE_FIREBASE
  char evt[48];
  snprintf(evt, sizeof(evt), "%s%s%s", NOM_ETAT[etat], strlen(cause) ? " - " : "", cause);
  journaliser(evt);
  tEnvoi = 0;  // envoi immédiat du nouvel état
#endif
}

// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Valise intelligente connectée ===");

  pinMode(PIN_REED, INPUT_PULLUP);
  pinMode(PIN_ARM, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_QI, OUTPUT);

  Wire.begin(PIN_SDA, PIN_SCL);
  mpuPresent = mpuInit();
  Serial.printf("[MPU6050] %s\n", mpuPresent ? "détecté" : "absent (détection de mouvement désactivée)");

#if USE_REAL_GPS
  Serial2.begin(9600, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
  Serial.println("[GPS] NEO-6M sur Serial2");
#else
  Serial.println("[GPS] position simulée (Sfax)");
#endif

#if USE_FIREBASE
  connecterWiFi();
#endif
}

void loop() {
  if (millis() - tCapteurs >= SENSOR_PERIOD_MS) {
    tCapteurs = millis();
    lireCapteurs();
    majEtat();
  }
  majSorties();

#if USE_FIREBASE
  if (tEnvoi == 0 || millis() - tEnvoi >= SEND_PERIOD_MS) {
    tEnvoi = millis();
    envoyerEtat();
    lireCommandes();
  }
#endif
}
