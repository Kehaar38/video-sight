// Virtual VideoSight (XIAO ESP32)
// D2: EVENT input <- PeripheralMCU
// D4: I2C SDA -> OLED + PeripheralMCU
// D5: I2C SCL -> OLED + PeripheralMCU

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static const uint8_t I2C_ADDR_OLED = 0x3C;
static const uint8_t I2C_ADDR_PERIPHERAL = 0x32;
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

static const int PIN_EVENT = D2;
static const int PIN_SDA   = D4;
static const int PIN_SCL   = D5;

static const uint8_t CMD_GET_COUNT = 0x00;
static const uint8_t CMD_GET_EVENT = 0x01;
static const uint8_t CMD_POP_EVENT = 0x02;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

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

static const uint8_t DISPLAY_QUEUE_SIZE = 16;
EventFrame g_dispQueue[DISPLAY_QUEUE_SIZE];
uint8_t g_dispHead = 0;
uint8_t g_dispTail = 0;
uint8_t g_dispCount = 0;

volatile bool g_peripheralIrqPending = false;

bool queuePush(const EventFrame &ev) {
  if (g_dispCount >= DISPLAY_QUEUE_SIZE) {
    return false;
  }
  g_dispQueue[g_dispTail] = ev;
  g_dispTail = (g_dispTail + 1) % DISPLAY_QUEUE_SIZE;
  g_dispCount++;
  return true;
}

bool queuePeek(EventFrame &ev) {
  if (g_dispCount == 0) {
    return false;
  }
  ev = g_dispQueue[g_dispHead];
  return true;
}

bool queuePop() {
  if (g_dispCount == 0) {
    return false;
  }
  g_dispHead = (g_dispHead + 1) % DISPLAY_QUEUE_SIZE;
  g_dispCount--;
  return true;
}

void IRAM_ATTR onPeripheralEvent() {
  g_peripheralIrqPending = true;
}

bool peripheralWriteCmd(uint8_t cmd) {
  Wire.beginTransmission(I2C_ADDR_PERIPHERAL);
  Wire.write(cmd);
  return (Wire.endTransmission() == 0);
}

uint8_t peripheralGetCount() {
  if (!peripheralWriteCmd(CMD_GET_COUNT)) {
    return 0;
  }

  if (Wire.requestFrom((int)I2C_ADDR_PERIPHERAL, 1) != 1) {
    return 0;
  }

  return (uint8_t)Wire.read();
}

bool peripheralGetEvent(EventFrame &ev) {
  if (!peripheralWriteCmd(CMD_GET_EVENT)) {
    return false;
  }

  const int need = 9;
  if (Wire.requestFrom((int)I2C_ADDR_PERIPHERAL, need) != need) {
    return false;
  }

  ev.version   = (uint8_t)Wire.read();
  ev.eventId   = (uint8_t)Wire.read();
  ev.eventType = (uint8_t)Wire.read();
  ev.nodeType  = (uint8_t)Wire.read();
  ev.tsL       = (uint8_t)Wire.read();
  ev.tsH       = (uint8_t)Wire.read();
  ev.meta0     = (uint8_t)Wire.read();
  ev.meta1     = (uint8_t)Wire.read();
  ev.flags     = (uint8_t)Wire.read();
  return true;
}

bool peripheralPopEvent() {
  return peripheralWriteCmd(CMD_POP_EVENT);
}

const char* eventTypeToStr(uint8_t t) {
  switch (t) {
    case 0x01: return "shot_detected";
    case 0x02: return "sw_pressed";
    case 0x03: return "sw_released";
    case 0x04: return "sensor_event";
    default:   return "unknown";
  }
}

const char* nodeTypeToStr(uint8_t n) {
  switch (n) {
    case 0x01: return "tracer_attiny85";
    case 0x02: return "remote_sw_attiny85";
    default:   return "unknown_node";
  }
}

void fetchAllPeripheralEvents() {
  while (true) {
    uint8_t count = peripheralGetCount();
    if (count == 0) {
      break;
    }

    EventFrame ev{};
    if (!peripheralGetEvent(ev)) {
      break;
    }

    queuePush(ev);
    if (!peripheralPopEvent()) {
      break;
    }
  }
}

void drawEvent(const EventFrame &ev) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  const uint16_t ts = (uint16_t)ev.tsL | ((uint16_t)ev.tsH << 8);

  display.println("Virtual VideoSight");
  display.println("----------------");
  display.print("Node: ");
  display.println(nodeTypeToStr(ev.nodeType));
  display.print("Type: ");
  display.println(eventTypeToStr(ev.eventType));
  display.print("ID  : ");
  display.println(ev.eventId);
  display.print("TS  : ");
  display.println(ts);
  display.print("M0  : ");
  display.print(ev.meta0);
  display.print("  M1: ");
  display.println(ev.meta1);

  display.display();
}

void drawWaiting() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Virtual VideoSight");
  display.println("----------------");
  display.println("Waiting event...");
  display.display();
}

void setup() {
  pinMode(PIN_EVENT, INPUT_PULLUP);

  Wire.begin(PIN_SDA, PIN_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDR_OLED)) {
    while (true) {
      delay(1000);
    }
  }

  attachInterrupt(digitalPinToInterrupt(PIN_EVENT), onPeripheralEvent, FALLING);

  g_peripheralIrqPending = true;
  drawWaiting();
}

void loop() {
  if (g_peripheralIrqPending) {
    g_peripheralIrqPending = false;
    fetchAllPeripheralEvents();
  }

  static bool showing = false;
  static uint32_t showStartedAt = 0;
  static EventFrame current{};

  if (!showing) {
    if (queuePeek(current)) {
      drawEvent(current);
      showStartedAt = millis();
      showing = true;
    }
  } else {
    if ((millis() - showStartedAt) >= 1000) {
      queuePop();
      showing = false;

      if (g_dispCount == 0) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Virtual VideoSight");
        display.println("----------------");
        display.println("Waiting event...");
        display.display();
      }
    }
  }
}