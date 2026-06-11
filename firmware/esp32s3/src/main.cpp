#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include <SPI.h>

// XIAO ESP32S3 Sense のmicroSD CS確認用。
// Seeed Wikiには GPIO21 と D2/GPIO3 の両方の記述があるため、
// 下の値を 21 または 3 に手動で書き換えて、それぞれ書き込み/読み出しを試す。
//
//   const int SD_CS_PIN = 21;
//   const int SD_CS_PIN = 3;
//
// 参考: https://wiki.seeedstudio.com/ja/xiao_esp32s3_sense_filesystem/
const int SD_CS_PIN = 21;

const char *TEST_FILE = "/video_sight_sd_cs_test.txt";

void printCardType(uint8_t cardType) {
  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
}

void listRoot(fs::FS &fs) {
  Serial.println("Listing root directory:");

  File root = fs.open("/");
  if (!root) {
    Serial.println("  Failed to open root directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("  Root is not a directory");
    root.close();
    return;
  }

  File file = root.openNextFile();
  while (file) {
    Serial.print("  ");
    Serial.print(file.isDirectory() ? "DIR : " : "FILE: ");
    Serial.print(file.name());
    if (!file.isDirectory()) {
      Serial.print("  SIZE: ");
      Serial.print(file.size());
    }
    Serial.println();
    file.close();
    file = root.openNextFile();
  }
  root.close();
}

bool writeTestFile(fs::FS &fs) {
  Serial.print("Writing test file: ");
  Serial.println(TEST_FILE);

  File file = fs.open(TEST_FILE, FILE_WRITE);
  if (!file) {
    Serial.println("  Failed to open file for writing");
    return false;
  }

  file.printf("VIDEO SIGHT SD CS test\n");
  file.printf("SD_CS_PIN=%d\n", SD_CS_PIN);
  file.printf("millis=%lu\n", static_cast<unsigned long>(millis()));
  file.close();

  Serial.println("  Write OK");
  return true;
}

bool readTestFile(fs::FS &fs) {
  Serial.print("Reading test file: ");
  Serial.println(TEST_FILE);

  File file = fs.open(TEST_FILE, FILE_READ);
  if (!file) {
    Serial.println("  Failed to open file for reading");
    return false;
  }

  Serial.println("  File content:");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();

  Serial.println("  Read OK");
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("========================================");
  Serial.println("VIDEO SIGHT - XIAO ESP32S3 Sense SD test");
  Serial.println("========================================");
  Serial.print("Testing SD.begin(");
  Serial.print(SD_CS_PIN);
  Serial.println(")");

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD.begin() failed.");
    Serial.println("Change SD_CS_PIN to 3 or 21, rebuild, upload, and try again.");
    return;
  }

  Serial.println("SD.begin() OK");

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached, or CS pin is wrong.");
    return;
  }

  printCardType(cardType);

  uint64_t cardSizeMB = SD.cardSize() / (1024ULL * 1024ULL);
  uint64_t totalMB = SD.totalBytes() / (1024ULL * 1024ULL);
  uint64_t usedMB = SD.usedBytes() / (1024ULL * 1024ULL);

  Serial.printf("SD Card Size: %llu MB\n", cardSizeMB);
  Serial.printf("Total space : %llu MB\n", totalMB);
  Serial.printf("Used space  : %llu MB\n", usedMB);

  listRoot(SD);

  bool writeOk = writeTestFile(SD);
  bool readOk = false;
  if (writeOk) {
    readOk = readTestFile(SD);
  }

  Serial.println("----------------------------------------");
  if (writeOk && readOk) {
    Serial.print("SD CS test PASSED with CS pin ");
    Serial.println(SD_CS_PIN);
  } else {
    Serial.print("SD CS test FAILED with CS pin ");
    Serial.println(SD_CS_PIN);
  }
  Serial.println("----------------------------------------");
}

void loop() {
  delay(1000);
}
