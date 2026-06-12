#include <Arduino.h>
#include <LovyanGFX.hpp>

// VIDEO SIGHT LCD bring-up / resolution check.
// Target module: Waveshare 1.83inch LCD Module Rev2 / ST7789P / 240x284.
//
// Wiring from docs/hardware/parts_and_pinout.md:
//   LCD_RST  D1 / GPIO2
//   LCD_DC   D2 / GPIO3
//   LCD_BL   D6 / GPIO43
//   LCD_CS   D7 / GPIO44
//   SPI_SCK  D8 / GPIO7
//   SPI_MISO D9 / GPIO8  (LCD does not use MISO)
//   SPI_MOSI D10 / GPIO9
//   PERIPH_EN D12 / GPIO41 (TPS22919 peripheral rail enable)

namespace pins {
constexpr int LCD_RST = 2;
constexpr int LCD_DC = 3;
constexpr int LCD_BL = 43;
constexpr int LCD_CS = 44;
constexpr int SPI_SCK = 7;
constexpr int SPI_MOSI = 9;
constexpr int PERIPH_EN = 41;
}  // namespace pins

// Confirmed on VIDEO SIGHT test hardware:
//   visible area: 240 x 284
//   product orientation: rotation 0
//   offset: 0,0
// The physical LCD has rounded corners; do not place important UI elements in
// the extreme corners.
constexpr int LCD_WIDTH = 240;
constexpr int LCD_HEIGHT = 284;
constexpr int LCD_OFFSET_X = 0;
constexpr int LCD_OFFSET_Y = 0;
constexpr uint8_t LCD_PRODUCT_ROTATION = 0;

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

uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
  return lcd.color565(r, g, b);
}

void enablePeripheralRailAndBacklight() {
  pinMode(pins::PERIPH_EN, OUTPUT);
  digitalWrite(pins::PERIPH_EN, HIGH);
  delay(100);

  pinMode(pins::LCD_BL, OUTPUT);
  digitalWrite(pins::LCD_BL, HIGH);
}

void drawCornerLabel(int x, int y, const char *label, uint16_t color) {
  lcd.fillRect(x, y, 28, 28, color);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setCursor(x + 2, y + 31);
  lcd.print(label);
}

void drawResolutionPattern(uint8_t rotation) {
  lcd.setRotation(rotation);
  const int w = lcd.width();
  const int h = lcd.height();

  lcd.fillScreen(TFT_BLACK);

  // One-pixel outer border. If any side is missing, the configured resolution
  // or ST7789 offset is probably wrong.
  lcd.drawRect(0, 0, w, h, TFT_WHITE);
  lcd.drawRect(1, 1, w - 2, h - 2, TFT_DARKGREY);

  // Colored corner blocks make rotation and edge clipping easy to identify.
  drawCornerLabel(4, 4, "TL", TFT_RED);
  drawCornerLabel(w - 32, 4, "TR", TFT_GREEN);
  drawCornerLabel(4, h - 48, "BL", TFT_BLUE);
  drawCornerLabel(w - 32, h - 48, "BR", TFT_ORANGE);

  // Center crosshair and 20-pixel grid ticks for visible-area confirmation.
  const int cx = w / 2;
  const int cy = h / 2;
  lcd.drawFastHLine(0, cy, w, TFT_CYAN);
  lcd.drawFastVLine(cx, 0, h, TFT_CYAN);

  for (int x = 0; x < w; x += 20) {
    lcd.drawFastVLine(x, 0, 8, TFT_YELLOW);
    lcd.drawFastVLine(x, h - 8, 8, TFT_YELLOW);
  }
  for (int y = 0; y < h; y += 20) {
    lcd.drawFastHLine(0, y, 8, TFT_YELLOW);
    lcd.drawFastHLine(w - 8, y, 8, TFT_YELLOW);
  }

  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextSize(1);
  lcd.setCursor(38, 8);
  lcd.print("VIDEO SIGHT LCD TEST");
  lcd.setCursor(38, 22);
  lcd.printf("rotation=%u", rotation);
  lcd.setCursor(38, 36);
  lcd.printf("visible=%dx%d", w, h);
  lcd.setCursor(38, 50);
  lcd.printf("panel=%dx%d", LCD_WIDTH, LCD_HEIGHT);
  lcd.setCursor(38, 64);
  lcd.printf("offset=%d,%d", LCD_OFFSET_X, LCD_OFFSET_Y);

  lcd.setCursor(20, cy + 10);
  lcd.print("Check full white border");
  lcd.setCursor(20, cy + 24);
  lcd.print("and all 4 corner blocks");

  Serial.printf("rotation=%u width=%d height=%d offset=(%d,%d)\n", rotation, w, h,
                LCD_OFFSET_X, LCD_OFFSET_Y);
}

void drawColorCycleFrame(uint16_t color, const char *name) {
  lcd.fillScreen(color);
  lcd.setTextColor(TFT_WHITE, color);
  lcd.setTextSize(2);
  lcd.setCursor(20, 20);
  lcd.print(name);
  lcd.setTextSize(1);
  lcd.setCursor(20, 52);
  lcd.print("Color fill test");
  Serial.print("color fill: ");
  Serial.println(name);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("========================================");
  Serial.println("VIDEO SIGHT - LCD resolution test");
  Serial.println("Waveshare 1.83inch LCD Rev2 / ST7789P");
  Serial.println("Expected visible resolution: 240x284");
  Serial.println("========================================");

  enablePeripheralRailAndBacklight();

  if (!lcd.init()) {
    Serial.println("lcd.init() failed");
    return;
  }

  lcd.setBrightness(255);
  drawResolutionPattern(LCD_PRODUCT_ROTATION);
}

void loop() {
  static uint32_t last = 0;
  static uint8_t step = 0;

  if (millis() - last < 3500) {
    delay(10);
    return;
  }
  last = millis();

  switch (step % 8) {
    case 0:
      drawResolutionPattern(0);
      break;
    case 1:
      drawResolutionPattern(1);
      break;
    case 2:
      drawResolutionPattern(2);
      break;
    case 3:
      drawResolutionPattern(3);
      break;
    case 4:
      drawColorCycleFrame(TFT_RED, "RED");
      break;
    case 5:
      drawColorCycleFrame(TFT_GREEN, "GREEN");
      break;
    case 6:
      drawColorCycleFrame(TFT_BLUE, "BLUE");
      break;
    case 7:
      drawColorCycleFrame(TFT_WHITE, "WHITE");
      break;
  }

  step++;
}
