// PeripheralMCU (ATmega328P, 8MHz)
// PD0: UART RX <- EventNode1
// PD2: EVENT input <- EventNode1
// PB2: EVENT output -> VideoSight (active LOW while queue not empty)
// PC4: I2C SDA -> VideoSight
// PC5: I2C SCL -> VideoSight

#include <Arduino.h>
#include <Wire.h>

static const uint8_t PIN_NODE1_EVENT_IN = 2;   // PD2 / INT0
static const uint8_t PIN_VS_EVENT_OUT   = 10;  // PB2

static const uint8_t I2C_ADDR = 0x32;
static const uint8_t FRAME_LEN = 11;
static const uint8_t FRAME_HEADER = 0xA5;

static const uint8_t CMD_GET_COUNT = 0x00;
static const uint8_t CMD_GET_EVENT = 0x01;
static const uint8_t CMD_POP_EVENT = 0x02;

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

static const uint8_t QUEUE_SIZE = 8;
volatile bool g_nodeEventHint = false;

EventFrame g_queue[QUEUE_SIZE];
volatile uint8_t g_qHead = 0;
volatile uint8_t g_qTail = 0;
volatile uint8_t g_qCount = 0;

volatile uint8_t g_lastI2cCmd = CMD_GET_COUNT;

void updateVsEventLine() {
  digitalWrite(PIN_VS_EVENT_OUT, (g_qCount > 0) ? LOW : HIGH);
}

bool queuePush(const EventFrame &ev) {
  if (g_qCount >= QUEUE_SIZE) {
    return false;
  }
  g_queue[g_qTail] = ev;
  g_qTail = (g_qTail + 1) % QUEUE_SIZE;
  g_qCount++;
  updateVsEventLine();
  return true;
}

bool queuePeek(EventFrame &ev) {
  if (g_qCount == 0) {
    return false;
  }
  ev = g_queue[g_qHead];
  return true;
}

bool queuePop() {
  if (g_qCount == 0) {
    return false;
  }
  g_qHead = (g_qHead + 1) % QUEUE_SIZE;
  g_qCount--;
  updateVsEventLine();
  return true;
}

void onNodeEventInterrupt() {
  g_nodeEventHint = true;
}

bool readOneFrameFromSerial(EventFrame &out) {
  static uint8_t buf[FRAME_LEN];
  static uint8_t idx = 0;

  while (Serial.available() > 0) {
    uint8_t b = (uint8_t)Serial.read();

    if (idx == 0) {
      if (b != FRAME_HEADER) {
        continue;
      }
      buf[idx++] = b;
      continue;
    }

    buf[idx++] = b;

    if (idx == FRAME_LEN) {
      idx = 0;

      uint8_t csum = 0;
      for (uint8_t i = 0; i < FRAME_LEN - 1; ++i) {
        csum ^= buf[i];
      }
      if (csum != buf[FRAME_LEN - 1]) {
        return false;
      }

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
  }

  return false;
}

void onI2cReceive(int count) {
  if (count <= 0) {
    return;
  }

  uint8_t cmd = (uint8_t)Wire.read();
  g_lastI2cCmd = cmd;

  if (cmd == CMD_POP_EVENT) {
    queuePop();
  }

  while (Wire.available()) {
    (void)Wire.read();
  }
}

void onI2cRequest() {
  if (g_lastI2cCmd == CMD_GET_COUNT) {
    Wire.write((uint8_t)g_qCount);
    return;
  }

  if (g_lastI2cCmd == CMD_GET_EVENT) {
    EventFrame ev{};
    if (!queuePeek(ev)) {
      const uint8_t zeros[9] = {0};
      Wire.write(zeros, sizeof(zeros));
      return;
    }

    uint8_t out[9];
    out[0] = ev.version;
    out[1] = ev.eventId;
    out[2] = ev.eventType;
    out[3] = ev.nodeType;
    out[4] = ev.tsL;
    out[5] = ev.tsH;
    out[6] = ev.meta0;
    out[7] = ev.meta1;
    out[8] = ev.flags;
    Wire.write(out, sizeof(out));
    return;
  }

  // default
  Wire.write((uint8_t)0);
}

void setup() {
  pinMode(PIN_NODE1_EVENT_IN, INPUT_PULLUP);
  pinMode(PIN_VS_EVENT_OUT, OUTPUT);
  digitalWrite(PIN_VS_EVENT_OUT, HIGH);

  Serial.begin(9600);

  attachInterrupt(digitalPinToInterrupt(PIN_NODE1_EVENT_IN), onNodeEventInterrupt, FALLING);

  Wire.begin(I2C_ADDR);
  Wire.onReceive(onI2cReceive);
  Wire.onRequest(onI2cRequest);
}

void loop() {
  EventFrame ev;
  while (readOneFrameFromSerial(ev)) {
    queuePush(ev);
  }

  // This flag is just a hint/wakeup. Parsing is driven by UART bytes.
  if (g_nodeEventHint) {
    g_nodeEventHint = false;
  }
}