// EventNode1 (ATtiny85, 8MHz, 3.3V)
// EVENT省略版
//
// PB0: UART TX -> PeripheralMCU
// PB1: 未使用（前回は EVENT出力）
// PB2: タクトスイッチ入力
// PB3: 未使用（前回は UART RX）
// PB4: デバッグLED（任意）
//
// 動作:
// - スイッチ変化を検知
// - イベント発生時にUARTフレームを即送信
//
// 前回片側UART版との差分:
// - EVENT線を使わない
// - setEventLine() 削除
// - EVENT_HOLD_MS 削除
// - PB1 は未使用

#include <Arduino.h>
#include <SoftwareSerial.h>

// ------------------------------
// ピン定義
// ------------------------------
static const uint8_t PIN_UART_TX   = PB0;
static const uint8_t PIN_UNUSED_1  = PB1;  // 前回は EVENT
static const uint8_t PIN_SWITCH    = PB2;
static const uint8_t PIN_UNUSED_2  = PB3;  // 前回は UART RX
static const uint8_t PIN_LED       = PB4;

// ------------------------------
// 固定値
// ------------------------------
static const uint32_t UART_BAUD = 9600;

static const uint8_t FRAME_HEADER  = 0xA5;
static const uint8_t FRAME_VERSION = 0x01;

// EventNode1 の種別値
static const uint8_t NODE_TYPE_VALUE = 0x11;

// イベント種別
static const uint8_t EVENT_TYPE_SWITCH_PRESSED  = 0x02;
static const uint8_t EVENT_TYPE_SWITCH_RELEASED = 0x03;

// デバウンス関連
static const uint16_t DEBOUNCE_MS        = 40;
static const uint16_t RETRIGGER_GUARD_MS = 60;

// SoftwareSerial
// RX は未使用だが、コンストラクタ都合でダミー指定
SoftwareSerial nodeSerial(PIN_UNUSED_2, PIN_UART_TX);

// ------------------------------
// イベント送信
// ------------------------------
uint8_t g_eventIdCounter = 0;

// フレーム送信
void sendEventFrame(uint8_t eventType, uint8_t meta0, uint8_t meta1, uint8_t flags) {
  uint16_t ts = (uint16_t)millis();

  uint8_t buf[11];
  buf[0]  = FRAME_HEADER;
  buf[1]  = FRAME_VERSION;
  buf[2]  = g_eventIdCounter++;
  buf[3]  = eventType;
  buf[4]  = NODE_TYPE_VALUE;
  buf[5]  = (uint8_t)(ts & 0xFF);
  buf[6]  = (uint8_t)((ts >> 8) & 0xFF);
  buf[7]  = meta0;
  buf[8]  = meta1;
  buf[9]  = flags;

  uint8_t checksum = 0;
  for (uint8_t i = 0; i < 10; ++i) {
    checksum ^= buf[i];
  }
  buf[10] = checksum;

  for (uint8_t i = 0; i < sizeof(buf); ++i) {
    nodeSerial.write(buf[i]);
  }
  nodeSerial.flush();
}

void setup() {
  pinMode(PIN_SWITCH, INPUT_PULLUP);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  nodeSerial.begin(UART_BAUD);
}

void loop() {
  static bool debouncedState = true;    // true=離されている
  static bool lastRawState   = true;
  static uint32_t lastChangeAt = 0;
  static uint32_t lastAcceptedEventAt = 0;

  bool raw = digitalRead(PIN_SWITCH);

  if (raw != lastRawState) {
    lastRawState = raw;
    lastChangeAt = millis();
  }

  if ((millis() - lastChangeAt) >= DEBOUNCE_MS && raw != debouncedState) {
    if ((millis() - lastAcceptedEventAt) >= RETRIGGER_GUARD_MS) {
      debouncedState = raw;
      lastAcceptedEventAt = millis();

      digitalWrite(PIN_LED, !digitalRead(PIN_LED));

      if (!debouncedState) {
        // 押下
        sendEventFrame(EVENT_TYPE_SWITCH_PRESSED, 0x01, 0x00, 0x00);
      } else {
        // 解放
        sendEventFrame(EVENT_TYPE_SWITCH_RELEASED, 0x00, 0x00, 0x00);
      }
    } else {
      debouncedState = raw;
    }
  }
}
