// =====================================================================
//  Valise intelligente connectée — configuration matérielle
// =====================================================================
#pragma once

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
#define USE_FIREBASE   1

// Position simulée : centre-ville de Sfax
#define SIM_LAT        34.7406
#define SIM_LON        10.7603

// ---------------- Réglages ----------------
#define MOTION_THRESHOLD_G   0.35f   // écart d'accélération déclenchant l'alarme [g]
#define SEND_PERIOD_MS       5000    // période d'envoi vers Firebase [ms]
#define SENSOR_PERIOD_MS     100     // période de lecture des capteurs [ms]
#define BUZZER_FREQ_HZ       2500    // fréquence de la sirène [Hz]
