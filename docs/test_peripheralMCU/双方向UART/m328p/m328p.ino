// PeripheralMCU (ATmega328P, 8MHz, 3.3V)
//
// EventNode1:
//   PD0 <- EventNode1 TX
//   PD1 -> EventNode1 RX
//   PD2 <- EventNode1 EVENT
//
// UI:
//   PD4 <- BTN_UP
//   PD5 <- BTN_DOWN
//   PD6 <- BTN_FRONT
//   PD7 <- BTN_BACK
//   PB0 <- ENC_A
//   PB1 <- ENC_B
//
// VideoSight:
//   PB2 -> EVENT out
//   PC4 -> SDA
//   PC5 -> SCL

#include <Arduino.h>
#include <Wire.h>

// ------------------------------
// ピン定義
// ------------------------------
static const uint8_t PIN_NODE1_EVENT   = 2;   // PD2
static const uint8_t PIN_VS_EVENT_OUT  = 10;  // PB2

static const uint8_t PIN_BTN_UP    = 4;  // PD4
static const uint8_t PIN_BTN_DOWN  = 5;  // PD5
static const uint8_t PIN_BTN_FRONT = 6;  // PD6
static const uint8_t PIN_BTN_BACK  = 7;  // PD7

static const uint8_t PIN_ENC_A = 8;  // PB0
static const uint8_t PIN_ENC_B = 9;  // PB1

// ------------------------------
// 通信定数
// ------------------------------
static const uint8_t I2C_ADDR = 0x32;
static const uint32_t UART_BAUD = 9600;

static const uint8_t CMD_HEADER      = 0x55;
static const uint8_t CMD_GET_EVENT   = 0x12;
static const uint8_t CMD_CLEAR_EVENT = 0x13;

static const uint8_t FRAME_HEADER = 0xA5;
static const uint8_t FRAME_LEN = 11;

// I2C レジスタ
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

static const uint16_t NODE_REPLY_TIMEOUT_MS = 80;
static const uint16_t BUTTON_DEBOUNCE_MS    = 20;

// ------------------------------
// エンコーダ設定
// InputDevice と同じ考え方
// ------------------------------

// 1クリックあたりの有効遷移数
static const int8_t ENC_STEPS_PER_NOTCH = 2;

// 回転方向の符号反転
// 向きが逆なら false にしてください
static const bool ENC_DIR_REVERSE = true;

// ------------------------------
// 外部イベントフレーム
// ------------------------------
struct EventFrame {
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

// ------------------------------
// 外部イベントキュー
// ------------------------------
static const uint8_t QUEUE_SIZE = 16;
EventFrame g_queue[QUEUE_SIZE];
volatile uint8_t g_qHead = 0;
volatile uint8_t g_qTail = 0;
volatile uint8_t g_qCount = 0;

// ------------------------------
// I2C レジスタ状態
// ------------------------------
volatile uint8_t g_regPointer = 0;

volatile uint8_t g_status = 0;
volatile uint8_t g_buttonState = 0;

// 未読エンコーダ差分累積
volatile int8_t g_encDelta = 0;

// クリック換算前の途中経過
volatile int8_t g_qstepAcc = 0;

// 前回AB状態
volatile uint8_t g_prevAB = 0;

// STATUS bit
static const uint8_t STATUS_BUTTON_PENDING   = 0x01;
static const uint8_t STATUS_ENCODER_PENDING  = 0x02;
static const uint8_t STATUS_EXTERNAL_PENDING = 0x04;

// ------------------------------
// 内部関数
// ------------------------------
void updateVsEventLine() {
  bool pending =
    (g_status & STATUS_BUTTON_PENDING) ||
    (g_status & STATUS_ENCODER_PENDING) ||
    (g_status & STATUS_EXTERNAL_PENDING);

  digitalWrite(PIN_VS_EVENT_OUT, pending ? LOW : HIGH);
}

bool queuePush(const EventFrame &ev) {
  if (g_qCount >= QUEUE_SIZE) {
    return false;
  }
  g_queue[g_qTail] = ev;
  g_qTail = (g_qTail + 1) % QUEUE_SIZE;
  g_qCount++;

  g_status |= STATUS_EXTERNAL_PENDING;
  updateVsEventLine();
  return true;
}

bool queuePeek(EventFrame &ev) {
  if (g_qCount == 0) return false;
  ev = g_queue[g_qHead];
  return true;
}

bool queuePop() {
  if (g_qCount == 0) return false;

  g_qHead = (g_qHead + 1) % QUEUE_SIZE;
  g_qCount--;

  if (g_qCount == 0) {
    g_status &= ~STATUS_EXTERNAL_PENDING;
  }
  updateVsEventLine();
  return true;
}

// ------------------------------
// EventNode1 コマンド送信
// ------------------------------
void sendCommandHardware(uint8_t cmd) {
  uint8_t csum = CMD_HEADER ^ cmd;
  Serial.write(CMD_HEADER);
  Serial.write(cmd);
  Serial.write(csum);
  Serial.flush();
}

// ------------------------------
// 外部イベントフレーム解析
// ------------------------------
bool parseFrame(const uint8_t *buf, EventFrame &out, uint8_t srcPort) {
  if (buf[0] != FRAME_HEADER) return false;

  uint8_t csum = 0;
  for (uint8_t i = 0; i < FRAME_LEN - 1; ++i) {
    csum ^= buf[i];
  }
  if (csum != buf[FRAME_LEN - 1]) return false;

  out.srcPort   = srcPort;
  out.version   = buf[1];
  out.eventId   = buf[2];
  out.eventType = buf[3];
  out.nodeType  = buf[4];
  out.tsL       = buf[5];
  out.tsH       = buf[6];
  out.meta0     = buf[7];
  out.meta1     = buf[8];
  out.flags     = buf[9];
  return true;
}

bool readFrameHardware(EventFrame &out, uint8_t srcPort) {
  uint8_t buf[FRAME_LEN];
  uint8_t idx = 0;
  uint32_t start = millis();

  while ((millis() - start) < NODE_REPLY_TIMEOUT_MS) {
    while (Serial.available() > 0) {
      uint8_t b = (uint8_t)Serial.read();

      if (idx == 0) {
        if (b != FRAME_HEADER) {
          continue;
        }
      }

      buf[idx++] = b;
      if (idx == FRAME_LEN) {
        return parseFrame(buf, out, srcPort);
      }
    }
  }
  return false;
}

// ------------------------------
// EventNode1 サービス
// ------------------------------
bool serviceNode1() {
  if (digitalRead(PIN_NODE1_EVENT) != LOW) {
    return false;
  }

  while (Serial.available() > 0) {
    Serial.read();
  }

  sendCommandHardware(CMD_GET_EVENT);

  EventFrame ev{};
  if (!readFrameHardware(ev, 1)) {
    return false;
  }

  queuePush(ev);
  sendCommandHardware(CMD_CLEAR_EVENT);
  return true;
}

// ------------------------------
// ボタン読み取り
// ------------------------------
uint8_t readRawButtons() {
  uint8_t state = 0;
  if (digitalRead(PIN_BTN_UP)    == LOW) state |= 0x01;
  if (digitalRead(PIN_BTN_DOWN)  == LOW) state |= 0x02;
  if (digitalRead(PIN_BTN_FRONT) == LOW) state |= 0x04;
  if (digitalRead(PIN_BTN_BACK)  == LOW) state |= 0x08;
  return state;
}

void serviceButtons() {
  static uint8_t lastRaw = 0;
  static uint8_t debounced = 0;
  static uint32_t lastChangeAt = 0;

  uint8_t raw = readRawButtons();

  if (raw != lastRaw) {
    lastRaw = raw;
    lastChangeAt = millis();
  }

  if ((millis() - lastChangeAt) >= BUTTON_DEBOUNCE_MS && raw != debounced) {
    debounced = raw;
    g_buttonState = debounced;
    g_status |= STATUS_BUTTON_PENDING;
    updateVsEventLine();
  }
}

// ------------------------------
// エンコーダ関連
// InputDevice と同じ考え方
// ------------------------------
uint8_t readAB() {
  uint8_t a = digitalRead(PIN_ENC_A) ? 1 : 0;
  uint8_t b = digitalRead(PIN_ENC_B) ? 1 : 0;
  return (uint8_t)((a << 1) | b);
}

// A/B の状態遷移から +1/-1 のクォドラチャステップを生成
int8_t quadratureStep(uint8_t prevAB, uint8_t currAB) {
  const uint8_t t = (uint8_t)((prevAB << 2) | currAB);

  // InputDevice と同じテーブル
  static const int8_t stepTable[16] = {
     0, -1, +1,  0,
    +1,  0,  0, -1,
    -1,  0,  0, +1,
     0, +1, -1,  0
  };

  int8_t s = stepTable[t];

  if (ENC_DIR_REVERSE) {
    s = (int8_t)-s;
  }

  return s;
}

void addEncoderDelta(int8_t delta) {
  int16_t tmp = (int16_t)g_encDelta + delta;
  if (tmp > 127) tmp = 127;
  if (tmp < -128) tmp = -128;
  g_encDelta = (int8_t)tmp;
  g_status |= STATUS_ENCODER_PENDING;
  updateVsEventLine();
}

void serviceEncoder() {
  uint8_t curr = readAB();
  uint8_t prev = g_prevAB;

  if (curr == prev) {
    return;
  }

  g_prevAB = curr;

  int8_t qstep = quadratureStep(prev, curr);
  if (qstep == 0) {
    return;
  }

  // まずクォドラチャ生ステップを蓄積
  g_qstepAcc = (int8_t)(g_qstepAcc + qstep);

  // 1クリック分に到達したら ±1 だけ加算
  if (ENC_STEPS_PER_NOTCH > 0) {
    while (g_qstepAcc >= ENC_STEPS_PER_NOTCH) {
      g_qstepAcc -= ENC_STEPS_PER_NOTCH;
      addEncoderDelta(+1);
    }
    while (g_qstepAcc <= -ENC_STEPS_PER_NOTCH) {
      g_qstepAcc += ENC_STEPS_PER_NOTCH;
      addEncoderDelta(-1);
    }
  }
}

// ------------------------------
// I2C
// ------------------------------
void onI2cReceive(int count) {
  if (count <= 0) return;

  // 最初の1バイトはレジスタ番号
  g_regPointer = (uint8_t)Wire.read();
  count--;

  // 書き込み付きの場合
  if (count > 0) {
    if (g_regPointer == REG_ACK_CLEAR) {
      uint8_t ack = (uint8_t)Wire.read();

      if (ack & 0x01) {
        g_status &= ~STATUS_BUTTON_PENDING;
      }
      if (ack & 0x02) {
        g_encDelta = 0;
        // 途中経過 g_qstepAcc は保持する
        g_status &= ~STATUS_ENCODER_PENDING;
      }
      if (ack & 0x04) {
        queuePop();
      }

      updateVsEventLine();
    }

    while (Wire.available()) {
      (void)Wire.read();
    }
  }
}

void onI2cRequest() {
  uint8_t value = 0;
  EventFrame ev{};

  switch (g_regPointer) {
    case REG_STATUS:
      value = g_status;
      break;

    case REG_BUTTON_STATE:
      value = g_buttonState;
      break;

    case REG_ENC_DELTA:
      value = (uint8_t)g_encDelta;
      break;

    case REG_EVENT_COUNT:
      value = g_qCount;
      break;

    case REG_SRC_PORT:
      if (queuePeek(ev)) value = ev.srcPort;
      break;

    case REG_VERSION:
      if (queuePeek(ev)) value = ev.version;
      break;

    case REG_EVENT_ID:
      if (queuePeek(ev)) value = ev.eventId;
      break;

    case REG_EVENT_TYPE:
      if (queuePeek(ev)) value = ev.eventType;
      break;

    case REG_NODE_TYPE:
      if (queuePeek(ev)) value = ev.nodeType;
      break;

    case REG_TS_L:
      if (queuePeek(ev)) value = ev.tsL;
      break;

    case REG_TS_H:
      if (queuePeek(ev)) value = ev.tsH;
      break;

    case REG_META0:
      if (queuePeek(ev)) value = ev.meta0;
      break;

    case REG_META1:
      if (queuePeek(ev)) value = ev.meta1;
      break;

    case REG_FLAGS:
      if (queuePeek(ev)) value = ev.flags;
      break;

    default:
      value = 0;
      break;
  }

  Wire.write(value);
}

void setup() {
  // EventNode
  pinMode(PIN_NODE1_EVENT, INPUT_PULLUP);

  // ボタン
  pinMode(PIN_BTN_UP, INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  pinMode(PIN_BTN_FRONT, INPUT_PULLUP);
  pinMode(PIN_BTN_BACK, INPUT_PULLUP);

  // エンコーダ
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  g_prevAB = readAB();

  // VideoSight へのイベント線
  pinMode(PIN_VS_EVENT_OUT, OUTPUT);
  digitalWrite(PIN_VS_EVENT_OUT, HIGH);

  Serial.begin(UART_BAUD);

  Wire.begin(I2C_ADDR);
  Wire.onReceive(onI2cReceive);
  Wire.onRequest(onI2cRequest);

  // 初期状態
  g_buttonState = readRawButtons();
}

void loop() {
  // UI
  serviceButtons();
  serviceEncoder();

  // EventNode1
  if (digitalRead(PIN_NODE1_EVENT) == LOW) {
    serviceNode1();
  }
}