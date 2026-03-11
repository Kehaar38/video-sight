#include <Arduino.h>

// Keep the companion MCU firmware isolated from the ESP32 app.
// Shared protocol details should live in repository docs, not duplicated ad hoc.
static constexpr uint8_t kInputMcuI2cAddr = INPUT_MCU_I2C_ADDR;

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println(F("VideoSight ATmega328P companion boot"));
  Serial.print(F("I2C address: 0x"));
  Serial.println(kInputMcuI2cAddr, HEX);
}

void loop() {
  delay(1000);
}

