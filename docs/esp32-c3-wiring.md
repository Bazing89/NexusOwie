# ESP32-C3 Wiring Guide (NexusOwie)

This document describes how to wire a **OneWheel BMS** to NexusOwie firmware running on an **ESP32-C3**.

Firmware target (PlatformIO):

- **Environment:** `esp32-c3`
- **Board:** `esp32-c3-devkitm-1` (see `platformio.ini`)
- **Pin definitions:** `src/bms_main.cpp`

The BMS bus is **115200 baud, 8N1**, unidirectional from BMS → main board. NexusOwie sits in the middle: it reads BMS traffic, may modify packets, and forwards data to the OneWheel controller (main board / MB).

---

## Required GPIO connections

These are the pins the firmware uses today:

| GPIO | Firmware name | Direction | Connect to |
|------|---------------|-----------|------------|
| **GPIO3** | `BMS_UART_RX` | Input | BMS **WHITE** wire (stub toward BMS) |
| **GPIO1** | `BMS_UART_TX` | Output | Main board **WHITE** wire (RS485 A line) |
| **GPIO4** | `TX_INPUT_PIN` | Input | **Jumper from GPIO1 (TX)** — see below |
| **GPIO5** | `TX_INVERSE_OUT_PIN` | Output | Main board **GREEN** wire (RS485 B line, inverted) |
| **3.3V** | — | Power in | BMS standby / 3.3V pickup (see power notes) |
| **GND** | — | Ground | BMS ground |

### On-board jumper (required)

Solder a short wire on the module connecting:

```text
GPIO1 (UART TX)  ──►  GPIO4 (TX monitor)
```

The firmware uses a pin-change interrupt on **GPIO4** to bit-bang the **inverse** of the TX line onto **GPIO5**. That drives the RS485 **B** line without a MAX485 chip — this is the ESP32-C3 equivalent of the old Wemos D1 Mini trick (solder **TX → D2**, i.e. GPIO4).

---

## OneWheel harness wiring

After cutting the BMS ↔ main board 3-wire harness (see the install instructions in the main [README](../README.md)):

| Wire color | Direction | ESP32-C3 pin |
|------------|-----------|--------------|
| **WHITE** | BMS → module (receive) | **GPIO3** (RX) |
| **WHITE** | Module → main board (transmit) | **GPIO1** (TX) |
| **GREEN** | Module → main board (B line) | **GPIO5** |
| **GREEN** | Stub toward BMS | **Not used** — insulate and tuck away |
| **GND** | BMS ground | **GND** |
| **Power** | BMS standby rail | **3.3V** (see power notes) |

### Signal flow (simplified)

```text
BMS ──WHITE──► GPIO3 (RX)     firmware reads BMS frames
                    │
                    ▼
              NexusOwie relay
                    │
                    ├──► GPIO1 (TX) ──WHITE──► Main board (A line)
                    │
                    └──► GPIO4 monitors TX
                              │
                              ▼ (inverted in ISR)
                         GPIO5 ──GREEN──► Main board (B line)
```

---

## Wemos D1 Mini → ESP32-C3 mapping

The **logical** wiring is unchanged from the original Wemos D1 Mini install — the BMS UART already lived on GPIO1/GPIO3 (hardware UART0) on the D1 Mini too. Only the module's physical pin silkscreen labels differ:

| Wemos D1 Mini label | GPIO | ESP32-C3 |
|---------------------|------|----------|
| RX | GPIO3 | **GPIO3** |
| TX | GPIO1 | **GPIO1** |
| D1 | GPIO5 | **GPIO5** |
| D2 | GPIO4 | **GPIO4** (jumper from TX) |
| GND | GND | **GND** |
| 5V | 5V | **3.3V** (see below) |

---

## ESP32-C3-DevKitM-1 reference

When testing on the official dev kit before installing inside a OneWheel:

| DevKit silkscreen | GPIO | NexusOwie use |
|--------------------|------|---------------|
| IO1 | GPIO1 | BMS UART TX |
| IO3 | GPIO3 | BMS UART RX |
| IO4 | GPIO4 | TX monitor (jumper from IO1) |
| IO5 | GPIO5 | RS485 B line out |
| IO8 | GPIO8 | On-board LED (status blink) |
| USB | GPIO18/19 | Flashing only (not used for BMS UART) |

**Do not use these for custom wiring** — they are strapping / boot pins on ESP32-C3:

- GPIO0, GPIO2, GPIO9

Note: the DevKitM-1's onboard LED on GPIO8 is an addressable WS2812 on some board revisions, which won't visibly blink from a plain `digitalWrite()`. Other common "ESP32-C3 SuperMini"-style boards wire a plain LED to GPIO8 instead. Either way this only affects the diagnostic heartbeat blink, not BMS relay behavior.

---

## Power

- The RS485 bus I/O is **3.3 V** logic; ESP32-C3 GPIOs are 3.3 V tolerant and match the OneWheel BMS bus.
- Power the module from a **stable 3.3 V** source. Many installs tap the BMS standby rail; follow community install videos for your board generation (Pint / XR).
- The DevKitM-1 can be powered over USB for bench testing; inside the OneWheel, use your BMS pickup wiring instead.
- Do **not** feed 5 V into a 3.3 V-only module unless your specific board has an onboard regulator and you know its input limit.

---

## Firmware notes

- BMS UART uses **hardware UART0** via `Serial`, with explicit pins GPIO3 (RX) / GPIO1 (TX) passed to `Serial.begin()`.
- The firmware builds with `-DARDUINO_USB_CDC_ON_BOOT=0` so the native USB-CDC port doesn't compete with UART0 for the `Serial` symbol.
