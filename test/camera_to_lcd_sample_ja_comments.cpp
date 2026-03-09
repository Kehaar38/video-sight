#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "esp_camera.h"

// ===== LCDのピン割り当て（docs/videosight_hardware_config_for_xiao_coding.mdより） =====
static constexpr int PIN_LCD_RST = 2;    // D1 / GPIO2
static constexpr int PIN_LCD_BL = 43;    // D6 / GPIO43
static constexpr int PIN_LCD_DC = 42;    // D11 / GPIO42
static constexpr int PIN_LCD_CS = 41;    // D12 / GPIO41
static constexpr int PIN_SPI_SCK = 7;    // D8 / GPIO7
static constexpr int PIN_SPI_MOSI = 9;   // D10 / GPIO9
static constexpr int PIN_SPI_MISO = 8;   // D9 / GPIO8

// ===== LCDの表示サイズ =====
static constexpr int LCD_W = 240;
static constexpr int LCD_H = 280;
static constexpr int VIDEO_W = 240;
static constexpr int VIDEO_H = 240;
static constexpr int VIDEO_X = 0;
static constexpr int VIDEO_Y = (LCD_H - VIDEO_H) / 2;  // 20
static constexpr int LCD_ROTATION = 2;                  // 0..3（2 = 180度回転）

SPIClass lcdSpi(FSPI);
Adafruit_ST7789 tft(&lcdSpi, PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RST);

// ===== XIAO ESP32S3 Sense カメラのピンマップ（OV2640） =====
// カメラモジュールのリビジョンが異なり初期化に失敗する場合は、
// このピン定義を使用中モジュールのピンマップに合わせて変更すること。
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
static constexpr int CAM_HMIRROR = 1;  // 0: 通常, 1: 左右反転
static constexpr int CAM_VFLIP = 0;    // 0: 通常, 1: 上下反転

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
  tft.fillScreen(ST77XX_BLACK);

  digitalWrite(PIN_LCD_BL, HIGH);
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
  drawStatus("Init camera...", ST77XX_YELLOW);

  if (!initCamera()) {
    drawStatus("Camera init NG", ST77XX_RED);
    return;
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
    // XIAO ESP32S3 Sense のカメラフレームバッファは、
    // Adafruit_ST7789 が期待する RGB565 のバイト順と異なる場合がある。
    // そのため描画前に各ピクセルの2バイトを入れ替える。
    for (size_t i = 0; i + 1 < fb->len; i += 2) {
      const uint8_t hi = fb->buf[i];
      fb->buf[i] = fb->buf[i + 1];
      fb->buf[i + 1] = hi;
    }
    tft.drawRGBBitmap(VIDEO_X, VIDEO_Y, reinterpret_cast<uint16_t *>(fb->buf), VIDEO_W, VIDEO_H);
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
