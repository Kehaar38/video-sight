// EventNode1 (ATtiny85, 8MHz)
// PB0: UART TX -> PeripheralMCU
// PB1: EVENT output -> PeripheralMCU (active LOW pulse)
// PB2: Tact switch input (to GND, use INPUT_PULLUP)

#include <Arduino.h>

static const uint8_t PIN_UART_TX   = PB0;
static const uint8_t PIN_EVENT_OUT = PB1;
static const uint8_t PIN_SWITCH    = PB2;

static const uint32_t UART_BAUD = 9600;
static const uint16_t UART_BIT_US = 1000000UL / UART_BAUD;

static const uint8_t FRAME_HEADER = 0xA5;
static const uint8_t FRAME_VERSION = 0x01;
static const uint8_t NODE_TYPE_REMOTE_SWITCH_ATTINY85 = 0x02;

static const uint8_t EVENT_TYPE_SWITCH_PRESSED  = 0x02;
static const uint8_t EVENT_TYPE_SWITCH_RELEASED = 0x03;

static const uint16_t DEBOUNCE_MS = 20;
static const uint16_t EVENT_HOLD_MS = 20;

uint8_t g_eventId = 0;

void uartTxByte(uint8_t b) {
  // 8N1, idle HIGH
  digitalWrite(PIN_UART_TX, HIGH);
  delayMicroseconds(UART_BIT_US);

  // start bit
  digitalWrite(PIN_UART_TX, LOW);
  delayMicroseconds(UART_BIT_US);

  // data bits LSB first
  for (uint8_t i = 0; i < 8; ++i) {
    digitalWrite(PIN_UART_TX, (b & 0x01) ? HIGH : LOW);
    delayMicroseconds(UART_BIT_US);
    b >>= 1;
  }

  // stop bit
  digitalWrite(PIN_UART_TX, HIGH);
  delayMicroseconds(UART_BIT_US);
}

void uartTxFrame(uint8_t eventType, uint8_t meta0, uint8_t meta1, uint8_t flags) {
  const uint16_t ts = (uint16_t)millis();

  uint8_t frame[11];
  frame[0]  = FRAME_HEADER;
  frame[1]  = FRAME_VERSION;
  frame[2]  = g_eventId++;
  frame[3]  = eventType;
  frame[4]  = NODE_TYPE_REMOTE_SWITCH_ATTINY85;
  frame[5]  = (uint8_t)(ts & 0xFF);
  frame[6]  = (uint8_t)((ts >> 8) & 0xFF);
  frame[7]  = meta0;
  frame[8]  = meta1;
  frame[9]  = flags;

  uint8_t checksum = 0;
  for (uint8_t i = 0; i < 10; ++i) {
    checksum ^= frame[i];
  }
  frame[10] = checksum;

  // Notify first, then transmit while EVENT is active.
  digitalWrite(PIN_EVENT_OUT, LOW);
  delayMicroseconds(UART_BIT_US * 2);

  for (uint8_t i = 0; i < sizeof(frame); ++i) {
    uartTxByte(frame[i]);
  }

  delay(EVENT_HOLD_MS);
  digitalWrite(PIN_EVENT_OUT, HIGH);
}

void setup() {
  pinMode(PIN_UART_TX, OUTPUT);
  digitalWrite(PIN_UART_TX, HIGH);

  pinMode(PIN_EVENT_OUT, OUTPUT);
  digitalWrite(PIN_EVENT_OUT, HIGH);

  pinMode(PIN_SWITCH, INPUT_PULLUP);
}

void loop() {
  static bool stableState = true;       // true = released (pull-up)
  static bool lastRead    = true;
  static uint32_t lastChg = 0;

  const bool raw = digitalRead(PIN_SWITCH);

  if (raw != lastRead) {
    lastRead = raw;
    lastChg = millis();
  }

  if ((millis() - lastChg) >= DEBOUNCE_MS && raw != stableState) {
    stableState = raw;

    if (!stableState) {
      // pressed
      uartTxFrame(EVENT_TYPE_SWITCH_PRESSED, 0x01, 0x00, 0x00);
    } else {
      // released
      uartTxFrame(EVENT_TYPE_SWITCH_RELEASED, 0x00, 0x00, 0x00);
    }
  }
}