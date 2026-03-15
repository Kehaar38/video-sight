// EventNode
// Remote Switch (ATtiny85, 8MHz, 3.3V)
// PB0: UART TX -> PeripheralMCU
// PB1: EVENT出力 -> PeripheralMCU（アクティブLOW）
// PB2: タクトスイッチ入力
// PB3: UART RX <- PeripheralMCU
// PB4: デバッグLED（任意）

#include <Arduino.h>
#include <SoftwareSerial.h>

// ------------------------------
// ピン定義
// ------------------------------
static const uint8_t PIN_UART_TX   = PB0;
static const uint8_t PIN_EVENT_OUT = PB1;
static const uint8_t PIN_SWITCH    = PB2;
static const uint8_t PIN_UART_RX   = PB3;
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

// Peripheral -> EventNode コマンド
static const uint8_t CMD_HEADER      = 0x55;
static const uint8_t CMD_GET_EVENT   = 0x12;
static const uint8_t CMD_CLEAR_EVENT = 0x13;

// デバウンス関連
static const uint16_t DEBOUNCE_MS        = 40;
static const uint16_t RETRIGGER_GUARD_MS = 60;

// SoftwareSerial
SoftwareSerial nodeSerial(PIN_UART_RX, PIN_UART_TX);

// ------------------------------
// イベントフレーム
// ------------------------------
struct EventFrame {
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

// ------------------------------
// グローバル状態
// ------------------------------
volatile bool g_eventPending = false;
EventFrame g_pendingEvent{};
uint8_t g_eventIdCounter = 0;

// ------------------------------
// EVENT線制御
// ------------------------------
void setEventLine(bool pending) {
  // pending=true の間だけ LOW にする
  digitalWrite(PIN_EVENT_OUT, pending ? LOW : HIGH);
}

// ------------------------------
// イベント保持
// ------------------------------
void prepareEvent(uint8_t eventType, uint8_t meta0, uint8_t meta1, uint8_t flags) {
  uint16_t ts = (uint16_t)millis();

  g_pendingEvent.version   = FRAME_VERSION;
  g_pendingEvent.eventId   = g_eventIdCounter++;
  g_pendingEvent.eventType = eventType;
  g_pendingEvent.nodeType  = NODE_TYPE_VALUE;
  g_pendingEvent.tsL       = (uint8_t)(ts & 0xFF);
  g_pendingEvent.tsH       = (uint8_t)((ts >> 8) & 0xFF);
  g_pendingEvent.meta0     = meta0;
  g_pendingEvent.meta1     = meta1;
  g_pendingEvent.flags     = flags;

  g_eventPending = true;
  setEventLine(true);
}

// ------------------------------
// 保持中イベント送信
// ------------------------------
void sendPendingEventFrame() {
  uint8_t buf[11];
  buf[0]  = FRAME_HEADER;
  buf[1]  = g_pendingEvent.version;
  buf[2]  = g_pendingEvent.eventId;
  buf[3]  = g_pendingEvent.eventType;
  buf[4]  = g_pendingEvent.nodeType;
  buf[5]  = g_pendingEvent.tsL;
  buf[6]  = g_pendingEvent.tsH;
  buf[7]  = g_pendingEvent.meta0;
  buf[8]  = g_pendingEvent.meta1;
  buf[9]  = g_pendingEvent.flags;

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

// ------------------------------
// コマンド処理
// ------------------------------
void handleCommand() {
  static uint8_t cmdBuf[3];
  static uint8_t idx = 0;

  while (nodeSerial.available() > 0) {
    uint8_t b = (uint8_t)nodeSerial.read();

    if (idx == 0) {
      if (b != CMD_HEADER) {
        continue;
      }
      cmdBuf[idx++] = b;
      continue;
    }

    cmdBuf[idx++] = b;

    if (idx == 3) {
      idx = 0;

      uint8_t csum = cmdBuf[0] ^ cmdBuf[1];
      if (csum != cmdBuf[2]) {
        continue;
      }

      uint8_t cmd = cmdBuf[1];

      if (cmd == CMD_GET_EVENT) {
        if (g_eventPending) {
          sendPendingEventFrame();
        }
      } else if (cmd == CMD_CLEAR_EVENT) {
        g_eventPending = false;
        setEventLine(false);
      }
    }
  }
}

void setup() {
  pinMode(PIN_EVENT_OUT, OUTPUT);
  setEventLine(false);

  pinMode(PIN_SWITCH, INPUT_PULLUP);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  nodeSerial.begin(UART_BAUD);
}

void loop() {
  // まずUARTコマンド処理
  handleCommand();

  // スイッチ監視
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
        prepareEvent(EVENT_TYPE_SWITCH_PRESSED, 0x01, 0x00, 0x00);
      } else {
        // 解放
        prepareEvent(EVENT_TYPE_SWITCH_RELEASED, 0x00, 0x00, 0x00);
      }
    } else {
      debouncedState = raw;
    }
  }
}