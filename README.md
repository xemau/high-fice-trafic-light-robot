# High-Five Traffic-Light Robot

PlatformIO / Arduino / C++17 firmware for the existing **ESP32-WROOM-32D development board**. The robot displays red for 3 seconds, yellow for 1 second, then waits on green for a debounced high-five. A high-five starts `/MP3/0001.mp3` and a rotating RGB animation for 10 seconds, then stops audio and returns to red.

The ESP32 controls a **YX5200 Mini MP3 module** over UART2; the YX5200 decodes the audio. Its DAC outputs feed one PAM8610 amplifier channel and the existing 4 Ω speaker. No additional microcontroller or replacement MP3 module is needed.

## Status and verification

The ESP32 firmware builds, and 42 native tests pass across six suites. A clean Apple Clang coverage run measured **100% core line coverage and 97.4% branch coverage**. Reproduce these results with the commands below; generated reports are ignored by Git.

Physical commissioning is still required: no ESP32 USB device was available during implementation. Tests verify application behavior and protocol handling, not actual sound, wiring, switch mechanics, supply stability, or this particular YX5200 module's compatibility. Follow the staged bring-up checklist before installing the electronics in cardboard.

## Hardware BOM

| Quantity | Component |
| --- | --- |
| 1 | Existing ESP32-WROOM-32D development board with USB programming and a 5V input |
| 1 each | YX5200 Mini MP3 module and microSD card (FAT16/FAT32, up to 32 GB) |
| 1 | PAM8610 stereo amplifier board, sold as 2×15 W |
| 1 | 4 Ω / 15 W / 75 mm speaker |
| 1 | Regulated, enclosed 12 V / 3 A wall adapter |
| 1 each | 5.5×2.1 mm female DC jack/pigtail, DC-rated rocker switch, LM2596 adjustable buck converter |
| 1 | RAMPS 1.4 mechanical endstop module |
| 1 | WS2812B strip/module, initially three addressable pixels |
| 2 | Approximately 1 kΩ resistors for analog mono summing |
| 1 | 330–470 Ω resistor in the WS2812 data line |
| 1 | Recommended 470–1000 µF electrolytic capacitor, rated at least 10 V, for the LED supply |
| 1 | Recommended 74AHCT-family 3.3 V → 5 V buffer, e.g. 74AHCT125, with 100 nF local decoupling |
| As needed | Wire, solder, heat-shrink, proper distribution connectors, optional perfboard and removable connectors |

No Arduino Uno, second ESP32, DFPlayer-branded replacement, DY-SV5W, different speaker, battery pack, or finished-installation breadboard is required.

## Power and wiring

**Never apply 12 V to the ESP32, YX5200, endstop, or WS2812. Adjust the disconnected LM2596 to 5.0 V with a multimeter before connecting any 5 V devices.** The 5V connection below is the carrier board's regulated-input pin, not the WROOM module's 3.3 V supply pin. Confirm the carrier's labels and pinout; WROOM-32D identifies the module, not the carrier's USB power circuit.

```text
230 V wall outlet
     |
enclosed regulated 12 V / 3 A adapter
     |
female DC jack +12 V (verify polarity)
     |
DC-rated rocker switch (interrupt positive, never ground)
     |
switched +12 V --------+--------------------------+
                      |                          |
                  PAM8610 VCC                LM2596 IN+
                                                 |
                                            adjusted 5.0 V
                                                 |
                                            LM2596 OUT+
                                                 |
                         +-----------------------+-------------------+
                         |                       |                   |
                    ESP32 5V                 YX5200 VCC           WS2812 5V

PSU negative -------- COMMON GROUND DISTRIBUTION BUS
                         |-- PAM8610 power GND
                         |-- LM2596 IN- and OUT- (non-isolated converter)
                         |-- ESP32 GND
                         |-- YX5200 GND / amplifier input ground reference
                         |-- WS2812 GND
                         `-- Endstop GND
```

Use a soldered/perfboard ground bus, suitable connectors, or WAGO-style distribution. Run separate power/return branches to the amplifier and buck converter; keep heavy speaker/amplifier return currents out of the sensor and audio-signal return wiring. Do not daisy-chain device power or route amplifier power through a breadboard. Insulate joints and mount the amplifier/buck so they can dissipate heat away from cardboard.

### Complete connection table

GPIO numbers are ESP32 GPIO labels, **not physical header positions**. Check silk-screen labels on the actual endstop and amplifier; connector orientation varies.

| From | To | Notes |
| --- | --- | --- |
| PSU +12 V via DC jack | Rocker switch input | Verify polarity and switch's DC rating |
| Rocker switch output | PAM8610 VCC and LM2596 IN+ | Both receive switched +12 V |
| PSU negative | Common GND bus | Positive supply is switched; ground stays common |
| GND bus | PAM8610 power GND; LM2596 IN− and OUT− | Buck input/output grounds share the same system |
| LM2596 OUT+ (5.0 V) | ESP32 carrier 5V; YX5200 VCC; WS2812 5V | Separate branches; never the ESP32 3V3 pin |
| GND bus | ESP32 GND; YX5200 GND; WS2812 GND; endstop GND | Connect before applying signals |
| ESP32 GPIO17 / UART2 TX | YX5200 RX | ESP32 transmits; optional ~1 kΩ series resistor if UART noise warrants it |
| YX5200 TX | ESP32 GPIO16 / UART2 RX | ESP32 receives, 9600 baud, 8N1; confirm module TX is 3.3 V logic |
| ESP32 3V3 | Endstop VCC | Keep signal within ESP32's 3.3 V logic range |
| Endstop SIGNAL | ESP32 GPIO27 | Active low; firmware enables input pull-up |
| ESP32 GPIO18 | AHCT buffer input | Recommended for reliable WS2812 logic levels |
| 5 V / GND bus | AHCT buffer VCC / GND | For 74AHCT125, enable used channel by taking its active-low OE to GND; terminate unused inputs per datasheet |
| Buffer output | 330–470 Ω resistor → first WS2812 DIN | Keep short; DIN, not DOUT. Initial short-wire direct GPIO18 → resistor → DIN may be tested |
| WS2812 DOUT | Next WS2812 DIN | Only needed when chaining separate modules |
| LED 5 V / GND | Capacitor + / − respectively | 470–1000 µF close to LEDs; observe polarity |
| YX5200 DAC_L | First 1 kΩ resistor → mono sum node | Line audio, **not SPK output** |
| YX5200 DAC_R | Second 1 kΩ resistor → same mono sum node | Never short DAC_L and DAC_R together |
| Mono sum node | PAM8610 left audio input | Use the board's line-input terminal/jack, not a speaker output |
| YX5200 GND | PAM8610 input GND | Common signal reference, short wiring |
| PAM8610 L+ | Speaker + | Use one amplifier channel only |
| PAM8610 L− | Speaker − | Both speaker terminals are driven; neither goes to GND |
| PAM8610 R+ / R− | Leave unconnected | Do not bridge channels; terminate unused input only as board instructions specify |

```text
YX5200 DAC_L ---- 1kΩ ----+
                         +---- PAM8610 left LINE input
YX5200 DAC_R ---- 1kΩ ----+
YX5200 GND ------------------- PAM8610 input GND

PAM8610 L+ ------------------- speaker +
PAM8610 L- ------------------- speaker -   (NOT ground)
```

Do not connect YX5200 SPK+/SPK− into the PAM8610. Do not ground L−/speaker−, join amplifier outputs, or use an earth-grounded oscilloscope clip on a speaker output. The PAM8610 has bridge outputs. Its manufacturer's datasheet includes 4 Ω load characteristics; the board's advertised “15 W” is not a guaranteed clean-output rating. Start with the amplifier gain low and firmware volume 12, then check distortion and temperature under load. Keep the specified 12 V architecture.

Avoid powering the ESP32 from USB and external 5 V simultaneously until the exact carrier's USB/5V power path has been verified. For basic programming, turn off 12 V and use USB to power only the ESP32; disconnect the external 5 V feed to the board if its backfeed behavior is unknown. Keep unpowered peripheral signal connections disconnected to avoid phantom powering. For serial diagnostics with the external supply on, use a verified power arrangement or a suitable USB data connection that isolates host VBUS while retaining the signals/ground required by the carrier. Do not assume all “data-only” cables provide this arrangement.

## Build, upload, and monitor

Install Python 3.10+ and a host C++ compiler (Xcode command line tools on macOS; GCC/G++ on Linux). From the repository root:

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r requirements-dev.txt
pio run -e esp32dev
pio device list
pio run -e esp32dev -t upload --upload-port /dev/cu.YOUR_ESP32_PORT
pio device monitor --port /dev/cu.YOUR_ESP32_PORT --baud 115200
```

Replace the port with the actual USB UART device (`/dev/ttyUSB0` is a common Linux example; `COM3` is a Windows example). Power down/prepare wiring before upload. If auto-reset fails, hold BOOT while upload connects, then release it. The selected `esp32dev` profile targets a classic ESP32 with 4 MB flash and no PSRAM, suitable for the WROOM-32D; confirm the actual carrier if upload/reset settings need adjustment.

The dependencies are pinned in `platformio.ini`: Espressif32 6.10.0, Arduino ESP32 2.0.17 via that platform, and Adafruit NeoPixel 1.12.5. PlatformIO stores tools locally in `.pio-core/` and builds in `.pio/`. Firmware binaries are under `.pio/build/esp32dev/`. No hardware upload occurs when running native tests.

Open the monitor and press EN/reset to see the boot menu. Send a number **followed by Enter** within 5 seconds. Use a serial terminal that sends LF or CRLF; both are supported. Without a completed selection, the firmware starts FULL automatically and does not wait for a serial connection. With PlatformIO monitor, `--filter send_on_enter` is useful for line editing. Exit the monitor with Ctrl+C before uploading.

## Diagnostic modes

Reboot to select a different mode. A digit sent after selection is rejected; modes do not silently change during operation. Commands are lowercase, one per line, up to 63 characters. Oversized or non-text lines are discarded in full, with one error; backspace and CRLF work. `help` and `status` work in every active mode.

| Selection | Mode | Initialized hardware | Operation and commands |
| --- | --- | --- | --- |
| `1` | FULL | Sensor, LEDs, UART2 | Automatic red → yellow → green → high-five → reward → red |
| `2` | LIGHTS TEST | LEDs only | Automatically cycles red, yellow, green, off every second. `red`, `yellow`, `green`, `off` hold a lamp. `cycle` restarts cycling. `dance` rotates RGB across **every** configured pixel |
| `3` | AUDIO TEST | UART2 only | `play 1`, `play 2`, `stop`, `pause`, `resume`, `volume 15`, `volume 20`, `next`, `previous`, `retry` |
| `4` | SENSOR TEST | GPIO27 only | Prints stable PRESSED/RELEASED transitions and a HIGH FIVE EVENT once per debounced press; no per-loop spam |
| `5` | SEQUENCE TEST | None of the physical sensor/LED/audio devices | Runs the real RobotController with virtual hardware. Send `highfive` at green; inspect state and simulated audio logs |

FULL and SEQUENCE also accept `status`, `play <track>`, `volume <0-30>`, `lights red`, `lights yellow`, `lights green`, `lights off`, `simulate highfive`, `reset`, and `help`, plus the audio controls above. Manual lamp commands persist until the next state transition; they do not change the state machine. `reset` stops pending reward playback and returns to red. In sequence mode, audio acceptance and lamp operations are simulated.

Boot-held switches are displayed as pressed but generate no high-five until released and pressed again. A press during red/yellow/reward is consumed and ignored by the state machine. A switch held into green cannot trigger a later reward automatically. Mount the switch inside the hand/arm so a high-five moves the hand enough to actuate it, without transferring the full impact into the switch or exposed wires.

### Audio initialization and errors

`begin()` starts asynchronous initialization, not a blocking success check. After a 3-second module settling period, the driver sends stop, select TF/SD, conservative volume, DAC enable, and a volume query, at least 200 ms apart. `[OK] YX5200` requires a valid checksummed volume reply matching the configured startup volume. A reply timeout produces `[ERROR] YX5200 initialization failed`; UART failure, SD removal, module errors, and runtime disconnects are also logged. FULL continues processing LEDs, sensor, timing, and diagnostics if audio fails.

Runtime commands go into a fixed eight-entry queue and return success **only for acceptance**, not audible playback. Invalid volumes/tracks, failed/not-ready audio, and queue overflow return failure. Commands do not wait for ACKs. The driver parses error/finish notifications and polls status every 5 seconds with a 1.5-second response timeout. `status` reports cached driver health; there is no claim that the speaker is connected or that a file exists. Repair wiring/card issues and send `retry` to restart initialization. Commands received before audio is ready are rejected rather than replayed unexpectedly later. Stop discards pending playback and takes priority over health polling at the next permitted transmit slot.

## SD-card layout and YX5200 compatibility

Use a FAT16/FAT32 microSD card, up to 32 GB, with this exact layout:

```text
SD root/
└── MP3/
    ├── 0001.mp3   # reward music
    ├── 0002.mp3
    └── ...
```

`play 1` uses command **0x12 (MP3-folder addressing)** for `/MP3/0001.mp3`, not FAT copy-order track selection. Valid filenames are four decimal digits, 0001–9999. For bring-up, use a short known-good MP3 and just these two files; remove macOS `._` resource-fork files from the card if present. `next`/`previous` send the module's native navigation commands: their order depends on the module/card enumeration and is not a guaranteed numeric ordering within MP3. Use `play N` when deterministic addressing matters.

The driver directly implements the YX5200/DFPlayer-compatible 10-byte protocol at 9600 baud, 8N1. DFRobot documentation is used as a **protocol reference**, not as a requirement to replace the existing YX5200. Module variants can differ; AUDIO TEST verifies the actual module. The checksum is the 16-bit two's complement of bytes 1–6 (version through parameter-low). For volume 15, the frame is `7E FF 06 06 00 00 0F FE E6 EF`. Some older manual examples contain inconsistent checksums; this implementation follows the algorithm and tests complete frames and corrupted/fragmented replies.

## Configuration and architecture

All tunable defaults are in [`include/Config.h`](include/Config.h): GPIOs, UART number/baud, serial baud, LED count/groups/brightness/color order, animation/cycle timings, state durations, debounce, reward track, volume, boot selection, UART pacing/timeouts/queue capacity, and serial buffer limits.

| Setting | Default |
| --- | --- |
| ESP32 UART2 RX / TX | GPIO16 ← YX5200 TX / GPIO17 → YX5200 RX |
| WS2812 data / high-five input | GPIO18 / GPIO27 |
| Serial / MP3 UART baud | 115200 / 9600 |
| LED groups | Pixel 0 red, 1 yellow, 2 green |
| LED brightness / frame interval | 48/255 / 80 ms |
| Red / yellow / reward | 3000 / 1000 / 10000 ms |
| Green | Wait indefinitely |
| Debounce | 30 ms |
| Reward track / initial volume | 1 / 12 (supported volume range 0–30) |
| Boot selection timeout / default | 5000 ms / FULL |

To use twelve pixels, change `LedCount` to `12` and `Lamps` to `{{{0, 4}, {4, 4}, {8, 4}}}`. RobotController needs no changes. Layout validation rejects empty, overlapping, or out-of-bounds groups. Reassess supply current and wiring when adding pixels; low firmware brightness is not a substitute for a supply/wiring design that tolerates startup or faults.

To compile with another default mode, change the `DEFAULT_APP_MODE` fallback in Config.h or extend the ESP32 build flags in `platformio.ini`:

```ini
build_flags = ${env.build_flags} -DDEFAULT_APP_MODE=2
```

Modes are numbered 1–5 as in the menu. A serial selection still overrides the compile-time default. Do not use ESP32 flash-connected GPIO6–11 or change to boot-strapping pins without checking the carrier/module documentation.

```text
include/interfaces/     Clock, sensor, audio, traffic light, logger, and electrical IO contracts
include/core/          Pure C++ application and driver declarations
src/core/              State machine, debounce, pixels, UART protocol, parser, diagnostics
include/hardware/      ESP32 adapter declarations
src/hardware/          Arduino GPIO/millis/UART/NeoPixel/serial access
src/main.cpp           Owns and connects components; Arduino setup()/loop() only
test/                  Six native Unity suites and fake IO
scripts/               Native compiler/coverage integration
```

Core code uses injected interfaces, fixed buffers and bounded queues, with no Arduino calls, exceptions, or dynamic allocation. Hardware startup may allocate the NeoPixel/UART buffers. Every duration check uses unsigned subtraction; service the loop regularly (well within the 32-bit millis wrap period, about 49.7 days). Long loop stalls advance at most one state per update, preserving a visible interval for each state. Animation skips missed frames without an unbounded catch-up loop.

There are no application `delay()` calls or busy waits. Serial input, UART receive, and log transmission each have per-loop budgets. Logs use a bounded buffer, drop excess whole messages, and report overflow when the buffer drains. NeoPixel output still has the short physical transmission time required by WS2812 (roughly 30 µs per pixel plus latch/driver overhead); it is not a 10-second blocking animation. Keep LED counts appropriate for the required input latency.

## Automated tests and coverage

With the development environment active:

```sh
pio test -e native
pio test -e native -f test_robot_controller
python scripts/coverage.py
```

`coverage.py` cleans **only the native build**, runs all six suites, and creates `coverage/index.html`, `coverage/summary.json`, and `coverage/cobertura.xml`. It fails below **90% line or 90% branch coverage**. It measures `src/core/` and `include/core/`, including protocol and diagnostic implementations; Arduino adapters, third-party libraries, and the fakes/test assertions are outside the denominator. It does not exclude untested core branches to improve the score.

On Linux, the script uses GCC's `gcov`; on macOS it uses Apple Clang and `xcrun llvm-cov gcov`. If using a different compiler version, set `GCOV` to the matching coverage executable, e.g. `GCOV=gcov-14 python scripts/coverage.py`. Compiler versions can produce slightly different branch totals. Do not merge counters from different source versions or compilers; the script cleans them first.

The tests cover state transitions, −1/exact/+1 timing boundaries and rollover; early/held/repeated high-fives; actual debounce bounce sequences; grouped LED mapping/off/dance; fixed UART frames, fragmented/corrupt input, bounded RX, command pacing/queue overflow, initialization/runtime failures, volume limits and stop priority; malformed serial input, boot defaults, and strict mode isolation. An integration test runs the real controller, diagnostics, sensor, LED mapping and audio driver together through fake electrical IO.

GitHub Actions runs native tests/coverage on Linux and macOS and builds the ESP32 firmware on Linux for pushes and pull requests. It publishes coverage reports and firmware binaries as workflow artifacts.

## Hardware bring-up checklist

Make connections with power off. Use one stage at a time; a missing MP3 module must not prevent sensor or light tests. Do not connect the whole robot just to test one component.

1. **Set LM2596 to 5.0 V.** Leave the ESP32, MP3 and LEDs disconnected. Apply the 12 V supply, verify jack polarity, set/measure buck output, then turn power off. Check the output again under load later.
2. **Power ESP32 only.** Use USB with the external feed isolated as described above. Build/upload, open 115200-baud monitor, reset, verify the menu and boot timeout. Resolve USB/external power isolation before live externally powered tests.
3. **SENSOR TEST (`4`).** Connect only endstop 3V3/GND/SIGNAL. Verify RELEASED → PRESSED + exactly one HIGH FIVE EVENT → RELEASED. Hold it for several seconds: no repeated events. Tap/bounce it and tune debounce only if needed.
4. **LIGHTS TEST (`2`).** Add the separately wired LED supply, common ground, resistor, capacitor, and preferably level buffer. Check each lamp, off, automatic cycle and dance. All configured pixels should show red, green, and blue during dance. Sensor/MP3 are not required.
5. **YX5200 AUDIO TEST (`3`).** Insert the prepared card while unpowered; connect 5V/GND/UART. Leave amplifier/speaker disconnected initially. Wait for verified `[OK] YX5200`, then try `play 1`, `pause`, `resume`, `stop`, `volume 12`, and `status`. UART success alone does not establish audible output.
6. **PAM8610 and speaker.** Power off, add resistor-summed DAC line audio to the left input and speaker across L+/L−. Power the amplifier from switched 12 V. Start its gain low. Repeat AUDIO TEST, listen for clean sound, and measure 5 V/12 V under playback load. Check for hot components or reset/brownout behavior. Never rewire speaker outputs while energized.
7. **SEQUENCE TEST (`5`).** Verify red (3 s), yellow (1 s), green (indefinite), then `highfive` → reward (10 s) → red. This mode requires no physical peripheral and does not play actual audio or drive LEDs.
8. **FULL (`1`).** Connect the tested components and use the physical hand. Verify music and dance start once on green, presses during red/yellow do not queue a reward, a held switch never retriggers, and a new press works on the next cycle. Disconnect/fix audio with power off and repeat to verify diagnostics and recovery. Test normal operation without a serial monitor.

Record results, module markings/carrier model, measured voltage under load, and any configuration changes in [`docs/hardware-validation.md`](docs/hardware-validation.md). Do not mark electrical/audio checks passed based on unit tests.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| ESP32 will not boot/upload | USB data cable and correct port; stable 5 V/3.3 V; common ground; carrier 5V versus 3V3 labels; no 12 V exposure; BOOT/EN procedure; no added loads on flash/strapping pins. Disconnect peripherals to isolate the fault |
| YX5200 not responding | AUDIO TEST; GPIO17 TX → module RX and GPIO16 RX ← module TX; module power/ground and 3.3 V UART levels; 9600 8N1; valid card; wait for initialization; inspect timeout/error log, then `retry`. Clones may require longer startup/command timings or have protocol differences |
| No speaker audio | A queued command is not proof of playback. Verify `/MP3/0001.mp3`, valid MP3, volume, amplifier 12 V supply/gain/mute state, DAC_L/R resistor sum to line input, input ground, and speaker across one channel's +/−. Leave SPK outputs unused |
| Audio distortion | Lower MP3 volume and amplifier gain, check clipped source audio, resistor sum, supply sag, loose connections and board temperature. Do not expect clean continuous 15 W from the marketing label |
| WS2812 does not respond | Correct DIN direction, configured count/pin, LED 5 V and common ground, series resistor location, buffer enable/wiring. Use a 74AHCT buffer instead of assuming 3.3 V data is accepted |
| Wrong LED colors | Verify physical pixel ordering and Config::Lamps. Config::LedColorOrder defaults to WS2812B GRB byte order; change it to match the actual LED part if channels are swapped. A pure RGB test distinguishes channel order from group mapping |
| Endstop always pressed/released | Check the module's labeled VCC/GND/SIGNAL and switch actuator; power at 3.3 V; measure released HIGH/pressed LOW; inspect connector reversal and switch contact selection. A disconnected input typically reads released through the pull-up |
| ESP32 resets when audio gets loud | Measure 12 V and 5 V under load; inspect wire/connector resistance, ground distribution, buck thermal/current limits, amplifier gain and short circuits. Keep amplifier current off breadboards and ESP32 supply wiring |
| Sensor or LEDs work but audio reports failure | Expected isolation: use the individual modes, repair audio, then `retry`. A sensor/LED initialization log verifies software setup, not external wiring |
| Commands do nothing | Finish boot selection with Enter before timeout; check mode, lowercase syntax and `help`. Wait for audio ready. Reboot to change modes. Watch queue/full/input-overflow errors |

## References

- [Espressif ESP32-WROOM-32D/32U datasheet](https://documentation.espressif.com/esp32-wroom-32d_esp32-wroom-32u_datasheet_en.html): module pinout and electrical limits.
- [PlatformIO esp32dev board](https://docs.platformio.org/en/stable/boards/espressif32/esp32dev.html): board target, build/upload configuration.
- [DFRobot protocol implementation](https://github.com/DFRobot/DFRobotDFPlayerMini/blob/master/DFRobotDFPlayerMini.cpp) and [module reference](https://wiki.dfrobot.com/dfr0299/docs/20905): compatible UART command IDs, MP3-folder addressing and checksum algorithm. No DFRobot library is used by this firmware.
- [Diodes PAM8610 datasheet](https://www.diodes.com/datasheet/download/PAM8610.pdf): supply, bridge outputs and load/power characteristics.
- [gcovr documentation](https://www.gcovr.com/en/8.3/): coverage commands and compiler-specific data handling.
