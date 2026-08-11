# Owie

[![Gitpod Ready-to-Code](https://img.shields.io/badge/Gitpod-Ready--to--Code-blue?logo=gitpod)](https://gitpod.io/#https://github.com/lolwheel/Owie)

This project unlocks battery expansion possibilities on otherwise locked OneWheels, adds WiFi-based monitoring of various battery health signals such as individual cell voltages, current, etc.

# Disclaimer

The authors and contributors of this project are in no way affiliated with Future Motion Inc. OneWheel, OneWheel XR, OneWheel Pint, OneWheel Plus, etc are registered trademarks of Future Motion Inc.

This is a hobby project for its contributors and comes with absolutely no guarantees of any sort. **Messing with your OneWheel in any way voids its warranty and could potentially lead to property loss, injuries or even death.** Don't be silly and use this project at your own risk.

# Features

- Unlocks battery expansion capabilities on Pints and XRs.
- Displays correct battery percentage in the official OneWheel app.
- Defeats BMS <-> Controller pairing and allows you to use any Pint or XR BMS in your board.
- Shows various stats about your battery on a web page through WiFi - Voltage, current, individual cell voltages and more.
- Supports future firmware updates via WiFi - no need to reopen your board.
- Adds password protection to your board

# Installing Owie into your board

## Prerequisites:

- Have essential soldering skills and tools: Soldering iron, some 22 gauge or otherwise thin wires, fish tape or isolating tape.
- Be comfortable with opening your board's battery enclosure.
  - For the PINT you require a somewhat exotic Torx 5-point security bit, size TS20. [Amazon link](https://www.amazon.com/gp/product/B07TC79LVH)
  - For the XR+ you will need a 3/32" Allen key. [Amazon link](https://www.amazon.com/dp/B0000CBJE1)
- **ESP32-C3 module** — this firmware targets the ESP32-C3 (not the original ESP8266 Wemos D1 Mini). Two common boards work:
  - **ESP32-C3 Super Mini** — recommended for installation inside the OneWheel. It is about the same size as the old Wemos D1 Mini and fits well in the battery enclosure. Look for a board **without** a bulky ceramic WiFi antenna if space is tight. Common on AliExpress and Amazon.
  - **ESP32-C3-DevKitM-1** — Espressif's official dev kit; fine for bench testing and first-time USB flashing before you install the module in the board.
  - Both boards use the **same GPIO numbers** in firmware; only the silkscreen labels differ. See [docs/esp32-c3-wiring.md](docs/esp32-c3-wiring.md) for the full pinout.

## Flashing Owie for the first time

1. Download the latest [`firmware.bin`](https://github.com/lolwheel/Owie/releases/latest/download/firmware.bin) from this repo's Releases (or build with `pio run -e esp32-c3`).
2. Connect the ESP32-C3 to your computer over **USB** (USB-C on most Super Mini boards) and flash the firmware. Options:
   - **PlatformIO:** `pio run -e esp32-c3 -t upload`
   - **ESP Web Tools:** follow the instructions on the ESP Web Tools page [here](https://ow-breaker.github.io/) if a build is hosted for ESP32-C3.
3. After flashing, the module should boot normally. Once installed in the board and powered from the BMS, you can connect with the NexusOwie companion app over BLE to verify telemetry (the board does not need to be wired to the BMS for a basic USB bench test).

## Installation into the board:

Follow this step-by-step installation video made by one of the community members — the BMS harness prep is the same even though the video shows a Wemos D1 Mini: https://www.youtube.com/watch?v=HhKdwnYUbA0

Or follow these instructions below (ESP32-C3 / Super Mini wiring):

1. Flash NexusOwie firmware onto your ESP32-C3 module as instructed above (USB on the Super Mini or DevKit).
2. I highly recommend physically removing or covering the **BOOT/RESET** button on the module so it cannot be pressed accidentally inside the OneWheel enclosure.
3. Disassemble your board and open the battery enclosure.
4. Disconnect all wires from BMS, strictly in the following order:
   1. Battery balance lead — the leftmost connector (24 wires) on the BMS.
   2. Battery main lead — an XT60 connector on the rightmost side of the BMS.
   3. All the other wires to the BMS; the order here doesn't matter.
5. Prepare your ESP32-C3 module and BMS:
   1. Tin **GPIO1** (TX), **GPIO3** (RX), **GPIO4**, **GPIO5**, **3.3V**, and **GND** on the module. On an ESP32-C3 Super Mini these correspond to **GPIO1**, **D1/GPIO3**, **D2/GPIO4**, **GPIO5**, **3V3**, and **GND** on the pinout diagram below.
   2. **Required on-board jumper:** solder a short wire on the module connecting **GPIO1 (TX) → GPIO4**. The firmware monitors GPIO4 and drives the inverted RS485 B line on GPIO5 — the same trick the original Wemos install used (TX → D2), without a MAX485 chip.
   3. Solder power pickup wires to the BMS. The JWFFM chip installation video demonstrates this well — [YouTube: Power pickup from BMS](https://youtu.be/kSWicH8hUFo?t=1028). Power the ESP32-C3 from **3.3 V**, not 5 V.
   4. Cut the **WHITE** and **GREEN** wires from the three-wire BMS↔main-board connector about 3/4 of an inch from the connector. Wrap the **GREEN** stub **toward the BMS** in insulating tape — it is not used. Tin the other three wire ends.
      Again, the JWFFM install video has a good demonstration: [YouTube: Cutting GREEN and WHITE wires](https://youtu.be/kSWicH8hUFo?t=453)

   ESP32-C3 Super Mini pinout — NexusOwie uses **GPIO1** (TX), **GPIO3** (RX), **GPIO4** (jumper from TX), **GPIO5** (RS485 B line), **3V3**, and **GND**:

   <img src="docs/img/esp32-c3-super-mini-pinout.png?raw=true" height="360px">

6. Connect the harness wires to your ESP32-C3 module (soldering to the bottom of a Super Mini often works best):

   | OneWheel wire | Direction | ESP32-C3 pin |
   |---|---|---|
   | **GND** (middle wire on BMS 5-pin connector) | BMS → module | **GND** |
   | **3.3 V standby** (from BMS pickup) | BMS → module | **3.3V** |
   | **WHITE** | BMS → module (receive) | **GPIO3** (RX) |
   | **WHITE** | Module → main board (transmit) | **GPIO1** (TX) |
   | **GREEN** | Module → main board (RS485 B line) | **GPIO5** |

   Do **not** use GPIO0, GPIO2, or GPIO9 for custom wiring — they are boot/strapping pins on ESP32-C3.

7. Insulate the module thoroughly (fish tape or electrical tape on top and bottom) so no solder joints can short against the BMS or enclosure.

For a diagram, DevKit pin reference, and Wemos→ESP32-C3 mapping, see **[docs/esp32-c3-wiring.md](docs/esp32-c3-wiring.md)**.

DONE!

## Troubleshooting:

### Board reporting battery at 1% after install

If after installing OWIE into your board it reports that your battery is at 1% even though it shouldn't, plug your board into a charger. This problem occurs because the BMS goes through a state reset and doesn't know the status of the battery, and plugging the board into a charger corrects this issue by forcing the BMS (and controller potentially) to do a state check.

# Board Locking functionality

TL;DR: You can immobilize your board by quickly power cycling the board. Once immobilized, you unlock the board by logging into Owie WiFi, tapping a button and power cycling the board. Keep reading for details.

**WARNING:** Arming your board for parking **will** disable the emergency recovery mode (2 restarts), so if you forget your network password, the only way to recover is to reflash via USB. The normal OTA update mode will still be functional as normal (see below for OTA instructions). Disarming the board will restore the emergency recovery mode.

Use these instructions if you want to be able to 'park' your OneWheel using the power button sequence. The park functionality comes by interrupting all communication between the BMS and the controller, thus causing an error 16.

This functionality can be removed quite easily by someone motivated enough and with enough knowledge; all that's required is to open up the board, remove Owie and solder the wires back together, or to reflash it via USB.

## Setup

1. Set an Owie network password in `Settings`.
1. Tap the `Arm` button in `Settings`. This arms your board so you can put it into 'park'.

## Parking your board

When you need to park your board, turn it on and then off in less than 5 seconds.

## Un-parking your board

Use these instructions to un-park your board so you can go ride.

1. Power on your board normally. Ignore the error 16 (that's how the board gets parked).
1. Connect to your password protected Owie network.
1. On the status screen, click the `Unlock` button.
1. Then as the button will remind you, restart your board to get rid of the error 16.

# Updating Owie

Use these instructions to update your Owie installation over WiFi (OTA).

## Using OTA

These instructions will work so long as you can connect to your regular `Owie-XXXX` network.

1. Download the latest `firmware.bin` from the Releases tab.
1. Once you have your hands on a firmware.bin file, copy that binary onto your flashing device of choice (desktop, laptop, phone). Some phones might not let you select the binary, thus you will need to use a computer.
1. Bring that device close to your board and ensure that your OneWheel has at least a few percent of battery left in the tank.
1. Connect to your normal Owie network `Owie-XXXX` and navigate to your normal Owie IP (192.168.4.1).
1. You should see the Owie menu load as normal.
1. Click "Settings" button, hit the "Browse" button in the Firmware section of the page and select the `firmware.bin`.
1. The page will look unresponsive during the file upload, do not refresh it.
1. Once the file is uploaded you will see a success message. DO NOT CUT POWER TO OWIE until it's WiFi is back on. Doing otherwise will brick your Owie and you'll have to re-flash it via USB.
1. Connect to the normal Owie network, and check that your update has worked.
1. Enjoy.

## Recovering an Owie flash

These instructions are for if you somehow manage to bungle flashing your wemos OTA.
They are the last step you can reasonably take before having to remove the chip from your board and flash it using a USB cable.

1. Follow the instructions above for how to build Owie from source using gitpod (or grab a release binary if they're available).
1. Once you have your hands on a firmware.bin file, copy that binary onto your flashing device of choice (desktop, laptop, phone). Some phones might not let you select the binary, thus you will need to use a computer.
1. Bring that device close to your board, and ensure that your OneWheel has at least a few percent of battery left in the tank (don't have it plugged in though).
1. Power cycle the OneWheel 2 times (reboot it) in less than 3 seconds. Keeping your app connected can be useful here as once Owie makes it into recovery mode, your board will report an error 16 (don't worry, this is supposed to happen).
   1. On XR's your headlights will come on as normal, but after a few seconds they will dim and then totally turn off, followed by your power button light flashing rapidly to indicate that error 16.
1. Connect to the WiFi network named _Owie-Recovery_ and navigate to your normal Owie IP (192.168.4.1).
1. You should see the firmware upload dialog. Point the file selector to the `firmware.bin` and wait for it to upload.
1. Either check for your normal Owie WiFi network to come back online, or in the app you should see battery percentage being reported again. Once either of these things occur (preferably both), you can restart your board to reset the error 16.
1. Connect to the normal Owie network, and check that your update has stuck.
1. Enjoy.

# [For posterity's sake] Things I've found during the development:

The BMS (Battery Management System) board, located in the battery side of the OneWheel, communicates with the main board via [RS485](https://en.wikipedia.org/wiki/RS-485) protocol. Details that I've managed to discover so far:

- The communication is unidirectional. The data flows only from BMS to MB. It's 115200 baud, standard 8N1 framing.
- The RS485 bus signaling voltage seems to be 3.3v. This makes it possible to read the bus voltages directly via ESP8266 and doesn't require 5v -> 3.3v level shifting.
- Both ends of the RS485 bus are terminated properly - 120-ohm resistors across the A and B lines and pullup / pulldown resistors A and B lines correspondingly.

## Doing away with MAX485 drivers

Technically, we'd need to use RS485 drivers such as MAX485 to intercept and retransmit bits on the line, however so far it seems like we can do away with them:

### Receiving RS485 directly via hardware UART:

The A (high) line of the RS485 bus coming from the BMS can be read directly via hardware UART with a little care.

When the line isn't driven by the BMS transmitter, it hovers around 3.3v/2 = 1.6v due to the terminating resistors on the BMS side. 1.6v isn't a defined logic level for a GPIO pin so chances are we'll read spurious data. However, if we turn on the pullup resistor of the UART RX pin to which the A line is connected, the bus idle voltage gets pulled up to right above 3 volts, which is more than 0.75\*VDD necessary for a logic 1 input, mentioned in ESP32 datasheet. This way the A wire from the BMS can be fed directly into the UART Rx pin and its logical state will read 1 at line idle, just as we want for UART communication.

## Transmitting RS485 without the driver

Transmitting the data to the MB requires us to signal on two wires. The A wire of the RS485 can be connected directly to the output of a hardware UART. The B line must be inverse of the A line as RS485 uses differential signaling.
I've achieved this by simply attaching a pin change interrupt to the UART TX pin and bitbanging the inverse value of this pin to the B line of the RS485.

# Communication protocol

The data frames sent by BMS are of the following general format:

1. Preamble - 3 bytes, fixed: `FF 55 AA`
1. Message type - 1 byte. I've so far observed all values between `0` and `0xD`, inclusive, except `1` and `0x10`
1. Message body - variable length but fixed based on the message type above.
1. Checksum - last two bytes of the frame - simply sum of all of the bytes in the frame, including the preamble.

## Message types:

I've isolated all message parsing code in `src/lib/bms/packet_parses.cpp`, the code should be self-explanatory.
