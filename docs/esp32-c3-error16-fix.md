# ESP32-C3 Error 16 Fix (NexusOwie)

## Symptom

After porting NexusOwie from the ESP8266 (Wemos D1 Mini) to ESP32-C3 (see `docs/esp32-c3-wiring.md`), the WiFi web UI worked correctly — the app showed live BMS telemetry (cell voltages, current, SOC, etc.) — but the OneWheel's main board reported **Error 16** (BMS↔main-board communication failure), something the original Wemos D1 Mini firmware never did on the same hardware.

Since telemetry (BMS → module, over hardware UART RX) worked fine, the break was specifically in the module → main board forwarding path (the software-generated RS485 B line).

## Root cause

Two things changed silently when the RS485 bit-bang relay moved from ESP8266 to ESP32-C3:

1. **The B-line interrupt handler got slower relative to the bit period.** The B line is generated in software: a GPIO-change interrupt on `TX_INPUT_PIN` (GPIO4, jumpered from the UART TX pin) reads the current TX level and writes its inverse onto `TX_INVERSE_OUT_PIN` (GPIO5), which drives the main board's B wire. At 115200 baud each bit is only ~8.7µs wide. `digitalRead()`/`digitalWrite()` on the ESP32 Arduino core carry enough call overhead (pin validity checks, mode lookups, etc.) to eat a meaningful fraction of that window, so the B line's edges drift out of alignment with the A line (which is driven directly by UART hardware and unaffected). The main board's RS485 receiver needs A and B to be clean complements to decode frames reliably; once they drift, frames get corrupted or dropped, which the main board's firmware surfaces as Error 16.

2. **The relay's UART processing was just another task in the cooperative `TaskQueue`.** It ran interleaved with DNS polling and other background work (`TaskQueue.postRecurringTask([]() { relay->loop(); })`) rather than being guaranteed to run first, every loop iteration. On ESP32-C3's single core, that's more scheduling latency than the ESP8266's simpler main loop had, adding jitter to how promptly received BMS bytes get relayed onward.

Both of these were previously root-caused (and fixed) for BLE-based sibling branches of this repo (see commit `d05bf3b` on the `esp32c3` branch, and `docs/esp32-error16-fixes.md`/`docs/esp32-wiring.md` on the `esp32` branch for the classic-ESP32 investigation). This fix ports the two *non-BLE-specific* parts of that work onto this branch, which has no BLE.

## Fix

### 1. Direct GPIO register access in the B-line ISR (`src/bms_main.cpp`)

```cpp
#if CONFIG_IDF_TARGET_ESP32C3
#include "soc/gpio_reg.h"
#endif

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
```

Reads the TX pin's level straight out of `GPIO_IN_REG` and toggles the B-line pin via the set/clear registers (`GPIO_OUT_W1TS_REG` / `GPIO_OUT_W1TC_REG`) instead of going through the Arduino pin-abstraction functions. The `#if CONFIG_IDF_TARGET_ESP32C3` guard keeps the portable `digitalRead`/`digitalWrite` path for any other target (native tests don't touch this file at all, since it's board-specific `src/`, not `lib/bms/`).

### 2. Run the relay outside the cooperative task queue (`src/main.cpp`, `src/bms_main.cpp`, `include/bms_main.h`)

`bms_main.cpp` now exposes `bms_process_uart()` instead of registering `relay->loop()` as a `TaskQueue` recurring task:

```cpp
// bms_main.cpp
void bms_process_uart() {
  if (relay != nullptr) {
    relay->loop();
  }
}
```

`main.cpp`'s `loop()` calls it directly, first, before anything else:

```cpp
void loop() {
  // RS485 BMS<->main board relay is safety-critical and must never be
  // delayed behind WiFi/background work, so it runs first, every iteration,
  // outside the cooperative TaskQueue.
  bms_process_uart();
  TaskQueue.process();
}
```

This guarantees the relay gets CPU time every single loop iteration, deterministically, rather than being one of several tasks a round-robin scheduler works through.

### 3. Larger UART RX buffer (`src/bms_main.cpp`)

```cpp
Serial.begin(115200, SERIAL_8N1, BMS_UART_RX, BMS_UART_TX);
Serial.setRxBufferSize(1024);
```

Gives the hardware UART driver more room to hold received bytes if `loop()` is ever briefly delayed, instead of overrunning and dropping data.

## What was deliberately *not* changed

The original fix commit (`d05bf3b`) also included a `TaskQueueType::process()` change (`>` → `>=` when checking if a timed task is due) and several BLE-notify-deferral changes. The BLE changes don't apply here (no BLE on this branch). The `TaskQueueType` change was **not** adopted — it would make a task scheduled with e.g. a 1ms delay fire one tick early, which breaks `test/test_task_queue_type/task_queue_type_test.cpp`'s `testOneshotTaskExecutionOrder` (it explicitly asserts nothing has executed yet at `millis == scheduledTime`). That change wasn't identified as load-bearing for Error 16, so it was left out rather than changing scheduler semantics and the test that pins them.

## Verification

- `pio run -e esp32-c3` / `pio run -e ota` both build clean.
- Flashed to hardware: Error 16 no longer occurs during normal (unlocked) operation — confirmed by the user on real OneWheel hardware.

Note: intentional Error 16 from board-parking (`Settings->is_locked`) is unrelated and expected — see the README's "Board Locking functionality" section.
