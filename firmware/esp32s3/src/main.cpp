#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "FS.h"
#include "SD.h"
#include <SPI.h>
#include "esp_camera.h"
#include "esp_heap_caps.h"

// VIDEO SIGHT camera live-view / FOV snapshot test.
// - LCD: Waveshare 1.83inch LCD Rev2 / ST7789P, 240x284, rotation 0.
// - Camera: XIAO ESP32S3 Sense bundled OV3660, DVP, RGB565 frame.
// - Wake button: D0 / GPIO1, active LOW. Press to save the current camera frame to SD.
// - SD: XIAO ESP32S3 Sense onboard microSD, CS GPIO21.
//
// The saved BMP is the camera-acquired frame size, before LCD crop/resize.
// LCD live view crops the left/right sides of the 4:3 camera frame to match the
// portrait LCD aspect ratio, then scales it to 240x284 for aiming/alignment.

namespace pins {
constexpr int WAKE_BUTTON = 1;

constexpr int LCD_RST = 2;
constexpr int LCD_DC = 3;
constexpr int LCD_BL = 43;
constexpr int LCD_CS = 44;
constexpr int SPI_SCK = 7;
constexpr int SPI_MISO = 8;
constexpr int SPI_MOSI = 9;
constexpr int SD_CS = 21;
constexpr int PERIPH_EN = 41;

// XIAO ESP32S3 Sense camera slot GPIO assignment from Seeed Wiki.
constexpr int CAM_XCLK = 10;
constexpr int CAM_SIOD = 40;
constexpr int CAM_SIOC = 39;
constexpr int CAM_Y9 = 48;
constexpr int CAM_Y8 = 11;
constexpr int CAM_Y7 = 12;
constexpr int CAM_Y6 = 14;
constexpr int CAM_Y5 = 16;
constexpr int CAM_Y4 = 18;
constexpr int CAM_Y3 = 17;
constexpr int CAM_Y2 = 15;
constexpr int CAM_VSYNC = 38;
constexpr int CAM_HREF = 47;
constexpr int CAM_PCLK = 13;
constexpr int CAM_PWDN = -1;
constexpr int CAM_RESET = -1;
}  // namespace pins

constexpr int LCD_WIDTH = 240;
constexpr int LCD_HEIGHT = 284;
constexpr int LCD_OFFSET_X = 0;
constexpr int LCD_OFFSET_Y = 0;
constexpr uint8_t LCD_PRODUCT_ROTATION = 0;

// Keep the camera frame 4:3 for FOV measurement. VGA is a good first bring-up
// point: large enough for pixel measurement, small enough for live conversion.
constexpr framesize_t CAMERA_FRAME_SIZE = FRAMESIZE_VGA;  // 640x480
constexpr pixformat_t CAMERA_PIXEL_FORMAT = PIXFORMAT_RGB565;
constexpr int JPEG_QUALITY_UNUSED_FOR_RGB565 = 12;

class VideoSightLcd : public lgfx::LGFX_Device {
  lgfx::Bus_SPI bus_;
  lgfx::Panel_ST7789 panel_;

 public:
  VideoSightLcd() {
    {
      auto cfg = bus_.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = true;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = pins::SPI_SCK;
      cfg.pin_mosi = pins::SPI_MOSI;
      cfg.pin_miso = -1;
      cfg.pin_dc = pins::LCD_DC;
      bus_.config(cfg);
      panel_.setBus(&bus_);
    }

    {
      auto cfg = panel_.config();
      cfg.pin_cs = pins::LCD_CS;
      cfg.pin_rst = pins::LCD_RST;
      cfg.pin_busy = -1;
      cfg.memory_width = 240;
      cfg.memory_height = 320;
      cfg.panel_width = LCD_WIDTH;
      cfg.panel_height = LCD_HEIGHT;
      cfg.offset_x = LCD_OFFSET_X;
      cfg.offset_y = LCD_OFFSET_Y;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = false;
      cfg.invert = true;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;
      panel_.config(cfg);
    }

    setPanel(&panel_);
  }
};

VideoSightLcd lcd;
uint16_t *lcdLine = nullptr;
uint32_t captureIndex = 0;
uint32_t lastButtonChangeMs = 0;
bool lastButtonLevel = HIGH;
bool stableButtonLevel = HIGH;
bool sdReady = false;
bool cameraReady = false;

uint16_t readRgb565(const uint8_t *p) {
  // esp_cameraのRGB565バッファは、この環境ではLCDへ渡す16bit値として
  // low byte, high byte の順に読む必要があった。逆順に読むと緑/紫系の
  // 派手な色化けになる。
  return (static_cast<uint16_t>(p[1]) << 8) | p[0];
}

void rgb565ToRgb888(uint16_t c, uint8_t &r, uint8_t &g, uint8_t &b) {
  r = ((c >> 11) & 0x1F) * 255 / 31;
  g = ((c >> 5) & 0x3F) * 255 / 63;
  b = (c & 0x1F) * 255 / 31;
}

void enablePeripheralRailAndBacklight() {
  pinMode(pins::PERIPH_EN, OUTPUT);
  digitalWrite(pins::PERIPH_EN, HIGH);
  delay(100);

  pinMode(pins::LCD_BL, OUTPUT);
  digitalWrite(pins::LCD_BL, HIGH);
}

bool initLcd() {
  pinMode(pins::LCD_CS, OUTPUT);
  digitalWrite(pins::LCD_CS, HIGH);

  if (!lcd.init()) {
    Serial.println("lcd.init() failed");
    return false;
  }

  lcd.setRotation(LCD_PRODUCT_ROTATION);
  lcd.setBrightness(255);
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setCursor(24, 24);
  lcd.print("VIDEO SIGHT camera test");
  return true;
}

bool initSd(bool remount = false) {
  if (remount) {
    SD.end();
    delay(20);
  }

  pinMode(pins::SD_CS, OUTPUT);
  pinMode(pins::LCD_CS, OUTPUT);
  digitalWrite(pins::SD_CS, HIGH);
  digitalWrite(pins::LCD_CS, HIGH);

  // LCD and SD share SCK/MOSI.  SD also needs MISO, so configure the Arduino
  // SPI object explicitly before SD.begin(); otherwise SD.begin(21) may use an
  // unsuitable default pin map after the LCD bus has been initialized.
  SPI.begin(pins::SPI_SCK, pins::SPI_MISO, pins::SPI_MOSI, pins::SD_CS);

  if (!SD.begin(pins::SD_CS, SPI, 4000000)) {
    Serial.println("SD.begin(21, SPI, 4MHz) failed");
    digitalWrite(pins::SD_CS, HIGH);
    return false;
  }

  if (SD.cardType() == CARD_NONE) {
    Serial.println("No SD card attached");
    digitalWrite(pins::SD_CS, HIGH);
    return false;
  }

  if (!SD.exists("/fov")) {
    SD.mkdir("/fov");
  }

  digitalWrite(pins::SD_CS, HIGH);
  Serial.println("SD ready: /fov");
  return true;
}

bool initCamera() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = pins::CAM_Y2;
  config.pin_d1 = pins::CAM_Y3;
  config.pin_d2 = pins::CAM_Y4;
  config.pin_d3 = pins::CAM_Y5;
  config.pin_d4 = pins::CAM_Y6;
  config.pin_d5 = pins::CAM_Y7;
  config.pin_d6 = pins::CAM_Y8;
  config.pin_d7 = pins::CAM_Y9;
  config.pin_xclk = pins::CAM_XCLK;
  config.pin_pclk = pins::CAM_PCLK;
  config.pin_vsync = pins::CAM_VSYNC;
  config.pin_href = pins::CAM_HREF;
  config.pin_sccb_sda = pins::CAM_SIOD;
  config.pin_sccb_scl = pins::CAM_SIOC;
  config.pin_pwdn = pins::CAM_PWDN;
  config.pin_reset = pins::CAM_RESET;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = CAMERA_PIXEL_FORMAT;
  config.frame_size = CAMERA_FRAME_SIZE;
  config.jpeg_quality = JPEG_QUALITY_UNUSED_FOR_RGB565;
  config.fb_count = psramFound() ? 2 : 1;
  config.fb_location = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("esp_camera_init failed: 0x%x\n", err);
    return false;
  }

  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor) {
    sensor->set_framesize(sensor, CAMERA_FRAME_SIZE);
    // 実機確認で上下反転していたため、OV3660側で垂直反転を補正する。
    sensor->set_vflip(sensor, 1);
    sensor->set_hmirror(sensor, 0);
  }

  Serial.printf("Camera ready: psram=%s fb_count=%d\n", psramFound() ? "yes" : "no",
                config.fb_count);
  return true;
}

void drawStatus(const char *message, uint16_t color = TFT_WHITE) {
  // Avoid rounded corners: the first visible characters can be clipped near x=0/y=0.
  constexpr int x = 24;
  constexpr int y = 24;
  lcd.fillRect(x - 4, y - 4, LCD_WIDTH - x, 18, TFT_BLACK);
  lcd.setTextColor(color, TFT_BLACK);
  lcd.setCursor(x, y);
  lcd.print(message);
}

void drawFrameToLcd(const camera_fb_t *fb) {
  if (!fb || fb->format != PIXFORMAT_RGB565 || !lcdLine) {
    return;
  }

  const int srcW = fb->width;
  const int srcH = fb->height;
  const int cropW = (srcH * LCD_WIDTH) / LCD_HEIGHT;
  const int cropX = (srcW - cropW) / 2;

  lcd.startWrite();
  for (int y = 0; y < LCD_HEIGHT; ++y) {
    const int srcY = (y * srcH) / LCD_HEIGHT;
    const uint8_t *srcRow = fb->buf + (srcY * srcW * 2);
    for (int x = 0; x < LCD_WIDTH; ++x) {
      const int srcX = cropX + (x * cropW) / LCD_WIDTH;
      lcdLine[x] = readRgb565(srcRow + srcX * 2);
    }
    lcd.pushImage(0, y, LCD_WIDTH, 1, lcdLine);
  }
  lcd.endWrite();

  // Simple safe-area and capture guide overlay. It is drawn after the frame so
  // the saved SD image remains raw camera data, not overlayed display data.
  lcd.drawRect(20, 20, LCD_WIDTH - 40, LCD_HEIGHT - 40, TFT_DARKGREY);
  lcd.drawFastHLine(0, LCD_HEIGHT / 2, LCD_WIDTH, TFT_DARKGREY);
  lcd.drawFastVLine(LCD_WIDTH / 2, 0, LCD_HEIGHT, TFT_DARKGREY);
}

bool writeBmp24FromRgb565Frame(const camera_fb_t *fb, const char *path) {
  if (!fb || fb->format != PIXFORMAT_RGB565) {
    Serial.println("save failed: frame is not RGB565");
    return false;
  }

  digitalWrite(pins::LCD_CS, HIGH);
  digitalWrite(pins::SD_CS, HIGH);

  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    Serial.printf("failed to open %s\n", path);
    digitalWrite(pins::SD_CS, HIGH);
    return false;
  }

  const uint32_t width = fb->width;
  const uint32_t height = fb->height;
  const uint32_t rowSize = ((width * 3 + 3) / 4) * 4;
  const uint32_t pixelDataSize = rowSize * height;
  const uint32_t fileSize = 54 + pixelDataSize;

  uint8_t header[54] = {};
  header[0] = 'B';
  header[1] = 'M';
  header[2] = fileSize & 0xFF;
  header[3] = (fileSize >> 8) & 0xFF;
  header[4] = (fileSize >> 16) & 0xFF;
  header[5] = (fileSize >> 24) & 0xFF;
  header[10] = 54;
  header[14] = 40;
  header[18] = width & 0xFF;
  header[19] = (width >> 8) & 0xFF;
  header[20] = (width >> 16) & 0xFF;
  header[21] = (width >> 24) & 0xFF;
  header[22] = height & 0xFF;
  header[23] = (height >> 8) & 0xFF;
  header[24] = (height >> 16) & 0xFF;
  header[25] = (height >> 24) & 0xFF;
  header[26] = 1;
  header[28] = 24;
  header[34] = pixelDataSize & 0xFF;
  header[35] = (pixelDataSize >> 8) & 0xFF;
  header[36] = (pixelDataSize >> 16) & 0xFF;
  header[37] = (pixelDataSize >> 24) & 0xFF;

  if (file.write(header, sizeof(header)) != sizeof(header)) {
    file.close();
    digitalWrite(pins::SD_CS, HIGH);
    return false;
  }

  uint8_t *row = static_cast<uint8_t *>(malloc(rowSize));
  if (!row) {
    file.close();
    digitalWrite(pins::SD_CS, HIGH);
    Serial.println("save failed: row malloc");
    return false;
  }

  const uint32_t padding = rowSize - width * 3;
  for (int32_t y = height - 1; y >= 0; --y) {
    const uint8_t *src = fb->buf + y * width * 2;
    uint8_t *dst = row;
    for (uint32_t x = 0; x < width; ++x) {
      uint8_t r, g, b;
      rgb565ToRgb888(readRgb565(src + x * 2), r, g, b);
      *dst++ = b;
      *dst++ = g;
      *dst++ = r;
    }
    for (uint32_t i = 0; i < padding; ++i) {
      *dst++ = 0;
    }
    if (file.write(row, rowSize) != rowSize) {
      free(row);
      file.close();
      digitalWrite(pins::SD_CS, HIGH);
      Serial.println("save failed: write row");
      return false;
    }
  }

  free(row);
  file.close();
  digitalWrite(pins::SD_CS, HIGH);
  return true;
}

bool saveCurrentFrameToSd(const camera_fb_t *fb) {
  if (!sdReady) {
    Serial.println("SD not ready; retrying SD init before save");
    drawStatus("Retry SD...", TFT_YELLOW);
    sdReady = initSd(true);
  }

  if (!sdReady) {
    drawStatus("SD not ready", TFT_RED);
    return false;
  }

  char path[48];
  snprintf(path, sizeof(path), "/fov/fov_%04lu_%ux%u.bmp",
           static_cast<unsigned long>(captureIndex++), fb->width, fb->height);

  Serial.printf("Saving %s ...\n", path);
  drawStatus("Saving BMP...", TFT_YELLOW);

  const uint32_t start = millis();
  const bool ok = writeBmp24FromRgb565Frame(fb, path);
  const uint32_t elapsed = millis() - start;

  if (ok) {
    Serial.printf("Saved %s in %lu ms\n", path, static_cast<unsigned long>(elapsed));
    drawStatus("Saved to /fov", TFT_GREEN);
  } else {
    Serial.printf("Save failed: %s\n", path);
    drawStatus("Save failed", TFT_RED);
  }
  return ok;
}

bool wakeButtonPressedEvent() {
  const bool level = digitalRead(pins::WAKE_BUTTON);
  const uint32_t now = millis();

  if (level != lastButtonLevel) {
    lastButtonLevel = level;
    lastButtonChangeMs = now;
  }

  if ((now - lastButtonChangeMs) < 40) {
    return false;
  }

  if (level != stableButtonLevel) {
    stableButtonLevel = level;
    if (stableButtonLevel == LOW) {
      return true;
    }
  }

  return false;
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("========================================");
  Serial.println("VIDEO SIGHT - camera live view / FOV capture");
  Serial.println("Wake button: save current RGB565 camera frame as BMP to /fov");
  Serial.println("LCD: center crop left/right to 240x284 portrait display");
  Serial.println("========================================");
  Serial.printf("PSRAM found: %s size=%u\n", psramFound() ? "yes" : "no",
                static_cast<unsigned>(ESP.getPsramSize()));

  pinMode(pins::WAKE_BUTTON, INPUT_PULLUP);
  enablePeripheralRailAndBacklight();

  lcdLine = static_cast<uint16_t *>(heap_caps_malloc(LCD_WIDTH * sizeof(uint16_t),
                                                      MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
  if (!lcdLine) {
    lcdLine = static_cast<uint16_t *>(malloc(LCD_WIDTH * sizeof(uint16_t)));
  }

  const bool lcdReady = initLcd();
  sdReady = initSd();
  cameraReady = initCamera();

  if (!lcdLine) {
    Serial.println("lcdLine allocation failed");
    if (lcdReady) drawStatus("line buffer failed", TFT_RED);
  } else if (!cameraReady) {
    if (lcdReady) drawStatus("camera failed", TFT_RED);
  } else if (!sdReady) {
    if (lcdReady) drawStatus("SD failed", TFT_RED);
  } else {
    if (lcdReady) drawStatus("Live: D0 saves BMP", TFT_GREEN);
  }
}

void loop() {
  if (!cameraReady || !lcdLine) {
    delay(500);
    return;
  }

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("camera capture failed");
    drawStatus("capture failed", TFT_RED);
    delay(100);
    return;
  }

  drawFrameToLcd(fb);

  if (wakeButtonPressedEvent()) {
    saveCurrentFrameToSd(fb);
  }

  esp_camera_fb_return(fb);
}
