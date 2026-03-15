// Virtual VideoSight (XIAO ESP32, 3.3V)
//
// D2  <- Peripheral EVENT
// D4  -> I2C SDA -> Peripheral
// D5  -> I2C SCL -> Peripheral
// D1  -> LCD DC
// D3  -> LCD CS
// D6  -> LCD BL
// D7  -> LCD RST
// D8  -> LCD SCK
// D10 -> LCD MOSI
//
// LCD: 1.69" 240x280 ST7789V2
//
// 表示方針
// ・通常時は TestDevice 風に BUTTONS / ENC_TOTAL を表示
// ・外部イベント受信時のみ 1秒間イベント画面を表示
// ・複数イベントはキューに積んで順番表示

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

static const uint8_t I2C_ADDR_PERIPHERAL = 0x32;

static const int PIN_EVENT = D2;
static const int PIN_SDA   = D4;
static const int PIN_SCL   = D5;

static const int PIN_LCD_DC   = D1;
static const int PIN_LCD_CS   = D3;
static const int PIN_LCD_BL   = D6;
static const int PIN_LCD_RST  = D7;
static const int PIN_LCD_SCK  = D8;
static const int PIN_LCD_MOSI = D10;

// 上端の角丸欠け対策
static const int Y_BASE = 28;

// Peripheral レジスタ
static const uint8_t REG_STATUS       = 0x00;
static const uint8_t REG_BUTTON_STATE = 0x01;
static const uint8_t REG_ENC_DELTA    = 0x02;
static const uint8_t REG_EVENT_COUNT  = 0x03;
static const uint8_t REG_SRC_PORT     = 0x04;
static const uint8_t REG_VERSION      = 0x05;
static const uint8_t REG_EVENT_ID     = 0x06;
static const uint8_t REG_EVENT_TYPE   = 0x07;
static const uint8_t REG_NODE_TYPE    = 0x08;
static const uint8_t REG_TS_L         = 0x09;
static const uint8_t REG_TS_H         = 0x0A;
static const uint8_t REG_META0        = 0x0B;
static const uint8_t REG_META1        = 0x0C;
static const uint8_t REG_FLAGS        = 0x0D;
static const uint8_t REG_ACK_CLEAR    = 0x10;

// STATUS bit
static const uint8_t STATUS_BUTTON_PENDING   = 0x01;
static const uint8_t STATUS_ENCODER_PENDING  = 0x02;
static const uint8_t STATUS_EXTERNAL_PENDING = 0x04;

SPIClass lcdSPI(FSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&lcdSPI, PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RST);

// ----------------------------------------
// 外部イベント表示キュー
// ----------------------------------------
struct DisplayItem {
  uint8_t srcPort;
  uint8_t version;
  uint8_t eventId;
  uint8_t eventType;
  uint8_t nodeType;
  uint8_t tsL;
  uint8_t tsH;
  uint8_t meta0;
  uint8_t meta1;
  uint8_t flags;
};

static const uint8_t DISPLAY_QUEUE_SIZE = 16;
DisplayItem g_dispQueue[DISPLAY_QUEUE_SIZE];
uint8_t g_dispHead = 0;
uint8_t g_dispTail = 0;
uint8_t g_dispCount = 0;

// ----------------------------------------
// 通常表示用状態
// ----------------------------------------
volatile bool g_peripheralIrqPending = false;

// 現在のボタン状態（ビット表示用）
uint8_t g_currentButtons = 0;

// エンコーダ差分累計値
int32_t g_encTotal = 0;

// 画面の再描画制御
enum ScreenMode {
  SCREEN_NORMAL,
  SCREEN_EVENT
};

ScreenMode g_screenMode = SCREEN_NORMAL;
bool g_screenDirty = true;

// ----------------------------------------
// キュー操作
// ----------------------------------------
bool queuePush(const DisplayItem &item) {
  if (g_dispCount >= DISPLAY_QUEUE_SIZE) return false;
  g_dispQueue[g_dispTail] = item;
  g_dispTail = (g_dispTail + 1) % DISPLAY_QUEUE_SIZE;
  g_dispCount++;
  return true;
}

bool queuePeek(DisplayItem &item) {
  if (g_dispCount == 0) return false;
  item = g_dispQueue[g_dispHead];
  return true;
}

bool queuePop() {
  if (g_dispCount == 0) return false;
  g_dispHead = (g_dispHead + 1) % DISPLAY_QUEUE_SIZE;
  g_dispCount--;
  return true;
}

// ----------------------------------------
// 割り込み
// ----------------------------------------
void IRAM_ATTR onPeripheralEvent() {
  g_peripheralIrqPending = true;
}

// ----------------------------------------
// I2C ヘルパ
// ----------------------------------------
bool writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(I2C_ADDR_PERIPHERAL);
  Wire.write(reg);
  Wire.write(value);
  return (Wire.endTransmission() == 0);
}

bool setReadReg(uint8_t reg) {
  Wire.beginTransmission(I2C_ADDR_PERIPHERAL);
  Wire.write(reg);
  return (Wire.endTransmission(false) == 0); // Repeated Start
}

uint8_t readReg8(uint8_t reg) {
  if (!setReadReg(reg)) return 0;
  if (Wire.requestFrom((int)I2C_ADDR_PERIPHERAL, 1) != 1) return 0;
  return (uint8_t)Wire.read();
}

// ----------------------------------------
// 表示用ヘルパ
// TestDevice の表示形式に合わせる
// ----------------------------------------
void makeButtonsString(char out[8], uint8_t b4) {
  // 表示例: [ - U F - ] ではなく固定長の [ - U F B ]
  // 並びは今回の4ボタン構成に合わせて:
  // bit0=UP, bit1=DOWN, bit2=FRONT, bit3=BACK
  out[0] = '[';
  out[1] = (b4 & 0x01) ? 'U' : '-';
  out[2] = (b4 & 0x02) ? 'D' : '-';
  out[3] = (b4 & 0x04) ? 'F' : '-';
  out[4] = (b4 & 0x08) ? 'B' : '-';
  out[5] = ']';
  out[6] = '\0';
}

void drawCenteredText(int16_t y, const String &text, uint8_t textSize) {
  tft.setTextSize(textSize);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);

  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  int16_t x = (240 - (int16_t)w) / 2;
  if (x < 0) x = 0;
  tft.setCursor(x, y);
  tft.print(text);
}

// ----------------------------------------
// 通常画面描画
// TestDevice 風
// ----------------------------------------
void drawNormalScreen() {
  char btnStr[8];
  makeButtonsString(btnStr, g_currentButtons);

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);

  // 見出し
  tft.setTextSize(2);
  tft.setCursor(10, Y_BASE);
  tft.println("BUTTONS");

  // ボタンビット一括表示
  tft.setCursor(20, Y_BASE + 28);
  tft.println(btnStr);

  // エンコーダ累計
  tft.setCursor(10, Y_BASE + 72);
  tft.println("ENC_TOTAL");

  // 累計値は大きく中央表示
  drawCenteredText(Y_BASE + 112, String(g_encTotal), 4);
}

// ----------------------------------------
// 外部イベント画面描画
// ----------------------------------------
const char* eventTypeToStr(uint8_t t) {
  switch (t) {
    case 0x02: return "SW PRESSED";
    case 0x03: return "SW RELEASED";
    default:   return "UNKNOWN";
  }
}

void drawEventScreen(const DisplayItem &it) {
  uint16_t ts = (uint16_t)it.tsL | ((uint16_t)it.tsH << 8);

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(2);

  int y = Y_BASE;
  tft.setCursor(8, y);  tft.println("EventNode"); y += 28;
  tft.setCursor(8, y);  tft.print("Port: "); tft.println(it.srcPort); y += 28;
  tft.setCursor(8, y);  tft.print("Node: "); tft.println(it.nodeType, HEX); y += 28;
  tft.setCursor(8, y);  tft.println(eventTypeToStr(it.eventType)); y += 28;
  tft.setCursor(8, y);  tft.print("ID: "); tft.println(it.eventId); y += 28;
  tft.setCursor(8, y);  tft.print("TS: "); tft.println(ts);
}

// ----------------------------------------
// Peripheral から状態を回収
// ----------------------------------------
void fetchPeripheralEvents() {
  uint8_t status = readReg8(REG_STATUS);

  // ボタン状態
  if (status & STATUS_BUTTON_PENDING) {
    g_currentButtons = readReg8(REG_BUTTON_STATE);
    writeReg(REG_ACK_CLEAR, 0x01);
    g_screenDirty = true;
  }

  // エンコーダ差分
  if (status & STATUS_ENCODER_PENDING) {
    int8_t delta = (int8_t)readReg8(REG_ENC_DELTA);
    g_encTotal += (int32_t)delta;
    writeReg(REG_ACK_CLEAR, 0x02);
    g_screenDirty = true;
  }

  // 外部イベント
  while (readReg8(REG_EVENT_COUNT) > 0) {
    DisplayItem item{};
    item.srcPort   = readReg8(REG_SRC_PORT);
    item.version   = readReg8(REG_VERSION);
    item.eventId   = readReg8(REG_EVENT_ID);
    item.eventType = readReg8(REG_EVENT_TYPE);
    item.nodeType  = readReg8(REG_NODE_TYPE);
    item.tsL       = readReg8(REG_TS_L);
    item.tsH       = readReg8(REG_TS_H);
    item.meta0     = readReg8(REG_META0);
    item.meta1     = readReg8(REG_META1);
    item.flags     = readReg8(REG_FLAGS);

    queuePush(item);
    writeReg(REG_ACK_CLEAR, 0x04);
  }
}

void setup() {
  pinMode(PIN_EVENT, INPUT_PULLUP);
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH);

  Wire.begin(PIN_SDA, PIN_SCL);

  lcdSPI.begin(PIN_LCD_SCK, -1, PIN_LCD_MOSI, PIN_LCD_CS);
  tft.init(240, 280);
  tft.setRotation(2);   // 画面反転
  tft.fillScreen(ST77XX_BLACK);

  attachInterrupt(digitalPinToInterrupt(PIN_EVENT), onPeripheralEvent, FALLING);

  g_screenMode = SCREEN_NORMAL;
  g_screenDirty = true;
  g_peripheralIrqPending = true;
}

void loop() {
  // Peripheral に変化があれば回収
  if (g_peripheralIrqPending) {
    g_peripheralIrqPending = false;
    fetchPeripheralEvents();
  }

  static bool showingEvent = false;
  static uint32_t showStartedAt = 0;
  static DisplayItem currentEvent{};

  // 外部イベントがなければ通常画面
  if (!showingEvent) {
    if (queuePeek(currentEvent)) {
      showingEvent = true;
      showStartedAt = millis();
      g_screenMode = SCREEN_EVENT;
      g_screenDirty = true;
    } else {
      if (g_screenMode != SCREEN_NORMAL) {
        g_screenMode = SCREEN_NORMAL;
        g_screenDirty = true;
      }
    }
  } else {
    if ((millis() - showStartedAt) >= 1000) {
      queuePop();
      showingEvent = false;
      if (g_dispCount == 0) {
        g_screenMode = SCREEN_NORMAL;
        g_screenDirty = true;
      }
    }
  }

  // 必要時のみ再描画
  if (g_screenDirty) {
    g_screenDirty = false;
    if (g_screenMode == SCREEN_NORMAL) {
      drawNormalScreen();
    } else {
      drawEventScreen(currentEvent);
    }
  }
}