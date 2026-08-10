#include <Arduino.h>

#include "battery_fuel_gauge.h"
#include "bms_relay.h"
#include "network.h"
#include "packet.h"
#include "settings.h"
#include "task_queue.h"

// ESP32-C3-DevKitM-1's onboard LED is GPIO8 (may be a plain LED or, on some
// boards, an addressable WS2812 that won't visibly respond to digitalWrite).
#ifndef LED_BUILTIN
#define LED_BUILTIN 8
#endif

// UART RX is connected to the *BMS* White line
// UART TX is connected to the *MB* White line
// TX_INPUT_PIN must be soldered to the UART TX
#define TX_INPUT_PIN 4
// Connected to the MB B line
#define TX_INVERSE_OUT_PIN 5
// Hardware UART0 pins (same logical wiring as the Wemos D1 Mini).
#define BMS_UART_RX 3
#define BMS_UART_TX 1

#if CONFIG_IDF_TARGET_ESP32C3
#include "soc/gpio_reg.h"
#endif

namespace {

// Emulate the RS485 B line by bitbanging the inverse of the TX A line.
// ESP32 only keeps one interrupt handler per pin, so a single CHANGE
// handler replaces the separate RISING/FALLING handlers used on ESP8266.
// On ESP32-C3, digitalRead()/digitalWrite() are slow enough (relative to the
// ~8.7us bit period at 115200 baud) to blur the B line's edges against A;
// direct GPIO register access keeps the two in tight sync.
void IRAM_ATTR txPinChangeInterrupt() {
#if CONFIG_IDF_TARGET_ESP32C3
  const uint32_t level = (REG_READ(GPIO_IN_REG) >> TX_INPUT_PIN) & 1U;
  if (level) {
    REG_WRITE(GPIO_OUT_W1TC_REG, (1U << TX_INVERSE_OUT_PIN));
  } else {
    REG_WRITE(GPIO_OUT_W1TS_REG, (1U << TX_INVERSE_OUT_PIN));
  }
#else
  digitalWrite(TX_INVERSE_OUT_PIN, digitalRead(TX_INPUT_PIN) ? 0 : 1);
#endif
}

}  // namespace

BmsRelay *relay;

void bms_setup() {
  relay = new BmsRelay([]() { return Serial.read(); },
                       [](uint8_t b) {
                         // This if statement is what implements locking.
                         if (!Settings->is_locked) {
                           Serial.write(b);
                         }
                       },
                       millis);
  Serial.begin(115200, SERIAL_8N1, BMS_UART_RX, BMS_UART_TX);
  // Give the UART driver more slack to hold received bytes if loop() is
  // briefly delayed by WiFi/task-queue work, instead of overrunning.
  Serial.setRxBufferSize(1024);

  // The B line idle is 0
  digitalWrite(TX_INVERSE_OUT_PIN, 0);
  pinMode(TX_INVERSE_OUT_PIN, OUTPUT);

  pinMode(TX_INPUT_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(TX_INPUT_PIN), txPinChangeInterrupt,
                  CHANGE);

  relay->addReceivedPacketCallback([](BmsRelay *, Packet *packet) {
    static uint8_t ledState = 0;
    digitalWrite(LED_BUILTIN, ledState);
    ledState = 1 - ledState;
    streamBMSPacket(packet->start(), packet->len());
  });
  relay->setUnknownDataCallback([](uint8_t b) {
    static std::vector<uint8_t> unknownData = {0};
    if (unknownData.size() > 128) {
      return;
    }
    unknownData.push_back(b);
    streamBMSPacket(&unknownData[0], unknownData.size());
  });

  if (Settings->has_battery_state) {
    FuelGaugeState gaugeState;
    gaugeState.bottomMilliampSeconds =
        Settings->battery_state.bottom_milliamp_seconds;
    gaugeState.currentMilliampSeconds =
        Settings->battery_state.current_milliamp_seconds;
    gaugeState.bottomSoc = Settings->battery_state.bottom_soc;
    gaugeState.topSoc = Settings->battery_state.top_soc;
    relay->getBatteryFuelGauge().restoreState(gaugeState);
  }

  // relay->setPowerOffCallback([]() {
  //   Settings->graceful_shutdown_count++;
  //   const FuelGaugeState &gaugeState = relay->getBatteryFuelGauge().getState();

  //   Settings->has_battery_state = true;
  //   Settings->battery_state.bottom_milliamp_seconds =
  //       gaugeState.bottomMilliampSeconds;
  //   Settings->battery_state.current_milliamp_seconds =
  //       gaugeState.currentMilliampSeconds;
  //   Settings->battery_state.bottom_soc = gaugeState.bottomSoc;
  //   Settings->battery_state.top_soc = gaugeState.topSoc;
  //   saveSettings();
  // });

  relay->setBMSSerialOverride(0xFFABCDEF);

  setupWifi();
  setupWebServer(relay);
}

void bms_process_uart() {
  if (relay != nullptr) {
    relay->loop();
  }
}
