// Test 4 — Accéléromètre MPU6050 en I2C (SDA = GPIO21, SCL = GPIO22, alimentation 3V3)
// Attendu : "MPU6050 détecté", puis ax, ay, az en g (az ≈ 1 g à plat).
#include <Wire.h>
const uint8_t MPU = 0x68;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Serial.println(Wire.endTransmission() == 0 ? "MPU6050 détecté" : "MPU6050 NON détecté : vérifier le câblage");
}

void loop() {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  if (Wire.requestFrom(MPU, (uint8_t)6) == 6) {
    float ax = (int16_t)((Wire.read() << 8) | Wire.read()) / 16384.0;
    float ay = (int16_t)((Wire.read() << 8) | Wire.read()) / 16384.0;
    float az = (int16_t)((Wire.read() << 8) | Wire.read()) / 16384.0;
    Serial.printf("ax=%.2f g  ay=%.2f g  az=%.2f g  |a|=%.2f g\n", ax, ay, az, sqrt(ax*ax + ay*ay + az*az));
  }
  delay(200);
}
