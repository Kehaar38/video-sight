#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <FS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "esp_camera.h"

// ===== LCD pin assignment (from docs/videosight_hardware_config_for_xiao_coding.md) =====
static constexpr int PIN_LCD_RST = 44;    // D7 / GPI44
static constexpr int PIN_LCD_BL = 43;    // D6 / GPIO43
static constexpr int PIN_LCD_DC = 42;    // D11 / GPIO42
static constexpr int PIN_LCD_CS = 41;    // D12 / GPIO41
static constexpr int PIN_SPI_SCK = 7;    // D8 / GPIO7
static constexpr int PIN_SPI_MOSI = 9;   // D10 / GPIO9
static constexpr int PIN_SPI_MISO = 8;   // D9 / GPIO8
static constexpr int PIN_SD_CS = 21;     // on-board SD CS

// ===== LCD geometry =====
static constexpr int LCD_W = 240;
static constexpr int LCD_H = 280;
static constexpr int VIDEO_W = 240;
static constexpr int VIDEO_H = 240;
static constexpr int VIDEO_X = 0;
static constexpr int VIDEO_Y = (LCD_H - VIDEO_H) / 2;  // 20
static constexpr int LCD_ROTATION = 2;                  // 0..3 (2 = 180deg)

SPIClass lcdSpi(FSPI);
Adafruit_ST7789 tft(&lcdSpi, PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RST);

class Rgb565Overlay : public Adafruit_GFX {
 public:
  Rgb565Overlay(int16_t w, int16_t h) : Adafruit_GFX(w, h) {}

  void setBuffer(uint16_t *buf) { buf_ = buf; }

  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (!buf_) return;
    if (x < 0 || y < 0 || x >= width() || y >= height()) return;
    buf_[(size_t)y * (size_t)width() + (size_t)x] = color;
  }

 private:
  uint16_t *buf_ = nullptr;
};

// ===== XIAO ESP32S3 Sense camera pin map (OV2640) =====
// If your camera module revision is different and init fails,
// update these pins to the board's camera pinout.
static constexpr int CAM_PWDN = -1;
static constexpr int CAM_RESET = -1;
static constexpr int CAM_XCLK = 10;
static constexpr int CAM_SIOD = 40;
static constexpr int CAM_SIOC = 39;

static constexpr int CAM_Y9 = 48;
static constexpr int CAM_Y8 = 11;
static constexpr int CAM_Y7 = 12;
static constexpr int CAM_Y6 = 14;
static constexpr int CAM_Y5 = 16;
static constexpr int CAM_Y4 = 18;
static constexpr int CAM_Y3 = 17;
static constexpr int CAM_Y2 = 15;
static constexpr int CAM_VSYNC = 38;
static constexpr int CAM_HREF = 47;
static constexpr int CAM_PCLK = 13;
static constexpr int CAM_HMIRROR = 1;  // 0: normal, 1: mirrored
static constexpr int CAM_VFLIP = 0;    // 0: normal, 1: flipped vertically

// ===== I2C InputDevice =====
static constexpr int PIN_I2C_SDA = 5;                 // D4 / GPIO5
static constexpr int PIN_I2C_SCL = 6;                 // D5 / GPIO6
static constexpr uint8_t INPUT_I2C_ADDR = 0x12;
static constexpr uint8_t INPUT_BIT_FRONT = 2;         // old "RIGHT" button bit

static bool g_frontPressed = false;
static bool g_prevFrontPressed = false;
static bool g_sdReady = false;
static Rgb565Overlay g_overlay(VIDEO_W, VIDEO_H);

static void writeLe16(File &f, uint16_t v) {
  f.write((uint8_t)(v & 0xFF));
  f.write((uint8_t)((v >> 8) & 0xFF));
}

static void writeLe32(File &f, uint32_t v) {
  f.write((uint8_t)(v & 0xFF));
  f.write((uint8_t)((v >> 8) & 0xFF));
  f.write((uint8_t)((v >> 16) & 0xFF));
  f.write((uint8_t)((v >> 24) & 0xFF));
}

static bool saveRgb565AsBmp(const uint8_t *rgb565, int w, int h) {
  if (!g_sdReady) {
    return false;
  }

  char path[32];
  int idx = 1;
  do {
    snprintf(path, sizeof(path), "/snap_%04d.bmp", idx++);
  } while (SD.exists(path) && idx < 10000);

  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open snapshot file");
    return false;
  }

  const uint32_t pixelBytes = (uint32_t)w * (uint32_t)h * 2U;
  const uint32_t headerBytes = 14U + 40U + 12U;  // BMP + DIB + RGB565 masks
  const uint32_t fileSize = headerBytes + pixelBytes;

  // BMP file header (14 bytes)
  file.write('B');
  file.write('M');
  writeLe32(file, fileSize);
  writeLe16(file, 0);
  writeLe16(file, 0);
  writeLe32(file, headerBytes);

  // BITMAPINFOHEADER (40 bytes)
  writeLe32(file, 40);               // biSize
  writeLe32(file, (uint32_t)w);      // biWidth
  writeLe32(file, (uint32_t)(-h));   // biHeight (top-down)
  writeLe16(file, 1);                // biPlanes
  writeLe16(file, 16);               // biBitCount
  writeLe32(file, 3);                // biCompression = BI_BITFIELDS
  writeLe32(file, pixelBytes);       // biSizeImage
  writeLe32(file, 2835);             // biXPelsPerMeter
  writeLe32(file, 2835);             // biYPelsPerMeter
  writeLe32(file, 0);                // biClrUsed
  writeLe32(file, 0);                // biClrImportant

  // RGB565 masks (12 bytes)
  writeLe32(file, 0xF800);           // red mask
  writeLe32(file, 0x07E0);           // green mask
  writeLe32(file, 0x001F);           // blue mask

  // Pixel data
  const size_t written = file.write(rgb565, pixelBytes);
  file.close();
  if (written != pixelBytes) {
    Serial.println("Snapshot write size mismatch");
    return false;
  }

  Serial.printf("SNAP saved: %s (%lu bytes)\n", path, (unsigned long)fileSize);
  return true;
}

static void updateInputDeviceState() {
  g_prevFrontPressed = g_frontPressed;

  const int req = Wire.requestFrom((int)INPUT_I2C_ADDR, 2);
  if (req < 2) {
    return;
  }

  const int8_t encDelta = (int8_t)Wire.read();
  (void)encDelta;  // currently unused in this sample

  const uint8_t b1 = (uint8_t)Wire.read();
  const uint8_t buttons = (uint8_t)(b1 & 0x1F);  // lower 5 bits = button states
  g_frontPressed = (buttons & (1U << INPUT_BIT_FRONT)) != 0;
}

static bool initCamera() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = CAM_Y2;
  config.pin_d1 = CAM_Y3;
  config.pin_d2 = CAM_Y4;
  config.pin_d3 = CAM_Y5;
  config.pin_d4 = CAM_Y6;
  config.pin_d5 = CAM_Y7;
  config.pin_d6 = CAM_Y8;
  config.pin_d7 = CAM_Y9;
  config.pin_xclk = CAM_XCLK;
  config.pin_pclk = CAM_PCLK;
  config.pin_vsync = CAM_VSYNC;
  config.pin_href = CAM_HREF;
  config.pin_sccb_sda = CAM_SIOD;
  config.pin_sccb_scl = CAM_SIOC;
  config.pin_pwdn = CAM_PWDN;
  config.pin_reset = CAM_RESET;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_240X240;
  config.pixel_format = PIXFORMAT_RGB565;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 2;

  const esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("esp_camera_init failed: 0x%x\n", err);
    return false;
  }

  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor != nullptr) {
    sensor->set_vflip(sensor, CAM_VFLIP);
    sensor->set_hmirror(sensor, CAM_HMIRROR);
    sensor->set_brightness(sensor, 0);
    sensor->set_saturation(sensor, 0);
  }
  return true;
}

static void initLcd() {
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, LOW);

  lcdSpi.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_LCD_CS);
  tft.init(240, 280);
  tft.setRotation(LCD_ROTATION);
  tft.cp437(true);  // enable CP437 symbols (arrow chars)
  tft.fillScreen(ST77XX_BLACK);

  digitalWrite(PIN_LCD_BL, HIGH);
}

static bool initSdCard() {
  if (!SD.begin(PIN_SD_CS, lcdSpi)) {
    Serial.println("SD.begin failed");
    return false;
  }
  uint64_t cardSizeMb = SD.cardSize() / (1024ULL * 1024ULL);
  Serial.printf("SD ready: %llu MB\n", cardSizeMb);
  return true;
}

static void drawMainModeGuide(Adafruit_GFX &gfx) {
  // MainMode guide:
  // [▲▼]BRT
  // [F▶]SNAP
  // [F_HOLD▶]REC
  // [◀B]MENU
  const int x = 4;
  const int y = 4;
  const uint16_t c = ST77XX_WHITE;

  // 背景透過にするため、塗りつぶしは行わず文字と図形だけ重ね描きする。
  gfx.setTextColor(c);
  gfx.setTextSize(1);

  // 1行目: [▲▼]BRT
  gfx.setCursor(x + 2, y + 2);
  gfx.printf("[%c%c]BRT", 0x1E, 0x1F);  // ▲▼

  // 2行目: [F▶]SNAP
  gfx.setCursor(x + 2, y + 12);
  gfx.printf("[F%c]SNAP", 0x10);  // ▶

  // 3行目: [F_HOLD▶]REC
  gfx.setCursor(x + 2, y + 22);
  gfx.printf("[F_HOLD%c]REC", 0x10);  // ▶

  // 4行目: [◀B]MENU
  gfx.setCursor(x + 2, y + 32);
  gfx.printf("[%cB]MENU", 0x11);  // ◀
}

static void drawStatus(const char *msg, uint16_t color) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(color);
  tft.setTextSize(2);
  tft.setCursor(10, 130);
  tft.print(msg);
}

void setup() {
  Serial.begin(115200);
  delay(300);

  initLcd();
  g_overlay.cp437(true);
  drawStatus("Init camera...", ST77XX_YELLOW);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 100000);

  if (!initCamera()) {
    drawStatus("Camera init NG", ST77XX_RED);
    return;
  }

  g_sdReady = initSdCard();
  if (!g_sdReady) {
    Serial.println("SNAP disabled: SD not available");
  }

  drawStatus("Camera init OK", ST77XX_GREEN);
  delay(500);
  tft.fillScreen(ST77XX_BLACK);
}

void loop() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (fb == nullptr) {
    Serial.println("esp_camera_fb_get failed");
    return;
  }

  if (fb->format == PIXFORMAT_RGB565 && fb->width == VIDEO_W && fb->height == VIDEO_H) {
    // XIAO ESP32S3 Sense camera framebuffer is often byte-swapped for RGB565
    // relative to Adafruit_ST7789 expectations. Swap each pixel bytes before draw.
    for (size_t i = 0; i + 1 < fb->len; i += 2) {
      const uint8_t hi = fb->buf[i];
      fb->buf[i] = fb->buf[i + 1];
      fb->buf[i + 1] = hi;
    }
    // ガイドをメモリ上のフレームに重畳してから、LCDへ1回だけ転送する
    g_overlay.setBuffer(reinterpret_cast<uint16_t *>(fb->buf));
    drawMainModeGuide(g_overlay);
    tft.drawRGBBitmap(VIDEO_X, VIDEO_Y, reinterpret_cast<uint16_t *>(fb->buf), VIDEO_W, VIDEO_H);

    updateInputDeviceState();
    const bool frontRisingEdge = (!g_prevFrontPressed && g_frontPressed);
    if (frontRisingEdge) {
      const bool ok = saveRgb565AsBmp(fb->buf, VIDEO_W, VIDEO_H);
      if (!ok) {
        Serial.println("SNAP failed");
      }
    }
  } else {
    static uint32_t lastWarn = 0;
    const uint32_t now = millis();
    if (now - lastWarn > 1000) {
      lastWarn = now;
      Serial.printf("Unexpected frame format/size: fmt=%d w=%d h=%d\n",
                    static_cast<int>(fb->format), fb->width, fb->height);
    }
  }

  esp_camera_fb_return(fb);
}
