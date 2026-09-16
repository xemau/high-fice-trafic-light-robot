# High-Five Traffic-Light Robot

PlatformIO / Arduino / C++17 firmware for the existing **ESP32-WROOM-32D development board** and **three standard LEDs: one red, one yellow, one green**. The robot displays red for 3 seconds, yellow for 1 second, then waits on green for a debounced high-five. A high-five starts `/MP3/0001.mp3` and a three-LED chase for 30 seconds, then stops audio and returns to red.

The automatic red phase remains 3 seconds, below the 5-second maximum. The 30-second reward timer starts when the high-five triggers the reward, not when audible playback begins. At the deadline, firmware ends the chase and queues an audio stop; actual sound timing includes UART/module latency. This is a reward-phase limit, not a universal audio timeout: manual AUDIO TEST playback and boot/error sounds have no added 30-second cutoff.

The ESP32 controls a **YX5200 Mini MP3 module** over UART2; the YX5200 decodes the audio. Its DAC outputs feed one PAM8610 amplifier channel and the existing 4 Ω speaker. No additional microcontroller or replacement MP3 module is needed.

## Status and verification

The ESP32 firmware builds, and 46 native tests pass across six suites, plus 11 host tests for SD preparation and audio conversion. A clean Apple Clang coverage run measured **100% core line coverage and 97.7% branch coverage**. Reproduce these results with the commands below; generated reports are ignored by Git.

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
| 1 each | Standard low-current red, yellow and green LEDs |
| 2 | Approximately 1 kΩ resistors for analog mono summing |
| 3 | LED current-limiting resistors, initially 330 Ω each; confirm against each LED's forward voltage/current rating |
| As needed | Wire, solder, heat-shrink, proper distribution connectors, optional perfboard and removable connectors |

No Arduino Uno, second ESP32, DFPlayer-branded replacement, DY-SV5W, different speaker, battery pack, or finished-installation breadboard is required.

## Power and wiring

**Never apply 12 V to the ESP32, YX5200, endstop, or LEDs. Adjust the disconnected LM2596 to 5.0 V with a multimeter before connecting any 5 V devices.** The 5V connection below is the carrier board's regulated-input pin, not the WROOM module's 3.3 V supply pin. LEDs use the ESP32's 3.3 V GPIO outputs through individual resistors, not the 5 V bus. Confirm the carrier's labels and pinout; WROOM-32D identifies the module, not the carrier's USB power circuit.

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
                         +-----------------------+
                         |                       |
                    ESP32 5V                 YX5200 VCC

PSU negative -------- COMMON GROUND DISTRIBUTION BUS
                         |-- PAM8610 power GND
                         |-- LM2596 IN- and OUT- (non-isolated converter)
                         |-- ESP32 GND
                         |-- YX5200 GND / amplifier input ground reference
                         |-- Red, yellow and green LED cathodes
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
| LM2596 OUT+ (5.0 V) | ESP32 carrier 5V; YX5200 VCC | Separate branches; never the ESP32 3V3 pin |
| GND bus | ESP32 GND; YX5200 GND; LED cathodes; endstop GND | Connect before applying signals |
| ESP32 GPIO17 / UART2 TX | YX5200 RX | ESP32 transmits; optional ~1 kΩ series resistor if UART noise warrants it |
| YX5200 TX | ESP32 GPIO16 / UART2 RX | ESP32 receives, 9600 baud, 8N1; confirm module TX is 3.3 V logic |
| ESP32 3V3 | Endstop VCC | Keep signal within ESP32's 3.3 V logic range |
| Endstop SIGNAL | ESP32 GPIO27 | Active low; firmware enables input pull-up |
| ESP32 GPIO18 | 330 Ω resistor → red LED anode (+) | Active high; cathode (−) to GND |
| ESP32 GPIO19 | 330 Ω resistor → yellow LED anode (+) | Active high; cathode (−) to GND |
| ESP32 GPIO23 | 330 Ω resistor → green LED anode (+) | Active high; cathode (−) to GND |
| YX5200 DAC_L | First 1 kΩ resistor → mono sum node | Line audio, **not SPK output** |
| YX5200 DAC_R | Second 1 kΩ resistor → same mono sum node | Never short DAC_L and DAC_R together |
| Mono sum node | PAM8610 left audio input | Use the board's line-input terminal/jack, not a speaker output |
| YX5200 GND | PAM8610 input GND | Common signal reference, short wiring |
| PAM8610 L+ | Speaker + | Use one amplifier channel only |
| PAM8610 L− | Speaker − | Both speaker terminals are driven; neither goes to GND |
| PAM8610 R+ / R− | Leave unconnected | Do not bridge channels; terminate unused input only as board instructions specify |

Each LED needs its **own** series resistor. For an ordinary LED with a 2.0 V forward voltage, 330 Ω gives approximately `(3.3 - 2.0) / 330 = 4 mA`; actual current depends on its forward voltage and GPIO output voltage. Select LEDs that are visible at a few milliamps. High-power LEDs or 5 V/12 V lamp modules need suitable external drivers and must not be connected directly to GPIOs. No addressable-strip level shifter or separate LED supply is used.

```text
GPIO18 ---- 330Ω ---- red LED anode     cathode ---- GND
GPIO19 ---- 330Ω ---- yellow LED anode  cathode ---- GND
GPIO23 ---- 330Ω ---- green LED anode   cathode ---- GND

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

The dependencies are pinned in `platformio.ini`: Espressif32 6.10.0 and Arduino ESP32 2.0.17 via that platform. LEDs use Arduino GPIO output directly, with no LED library dependency. PlatformIO stores tools locally in `.pio-core/` and builds in `.pio/`. Firmware binaries are under `.pio/build/esp32dev/`. No hardware upload occurs when running native tests.

Open the monitor and press EN/reset to see the boot menu. Send a number **followed by Enter** within 5 seconds. Use a serial terminal that sends LF or CRLF; both are supported. Without a completed selection, the firmware starts FULL automatically and does not wait for a serial connection. With PlatformIO monitor, `--filter send_on_enter` is useful for line editing. Exit the monitor with Ctrl+C before uploading.

## Diagnostic modes

Reboot to select a different mode. A digit sent after selection is rejected; modes do not silently change during operation. Commands are lowercase, one per line, up to 63 characters. Oversized or non-text lines are discarded in full, with one error; backspace and CRLF work. `help` and `status` work in every active mode.

| Selection | Mode | Initialized hardware | Operation and commands |
| --- | --- | --- | --- |
| `1` | FULL | Sensor, LEDs, UART2 | Automatic red → yellow → green → high-five → reward → red |
| `2` | LIGHTS TEST | LEDs only | Automatically cycles red, yellow, green, off every second. `red`, `yellow`, `green`, `off` hold a lamp. `cycle` restarts cycling. `dance` chases red → yellow → green, one LED at a time every 80 ms |
| `3` | AUDIO TEST | UART2 only | `play 1`, `play 2`, `boot`, `error`, `stop`, `pause`, `resume`, `volume 15`, `volume 20`, `next`, `previous`, `retry` |
| `4` | SENSOR TEST | GPIO27 only | Prints stable PRESSED/RELEASED transitions and a HIGH FIVE EVENT once per debounced press; no per-loop spam |
| `5` | SEQUENCE TEST | None of the physical sensor/LED/audio devices | Runs the real RobotController with virtual hardware. Send `highfive` at green; inspect state and simulated audio logs |

FULL and SEQUENCE also accept `status`, `play <track>`, `volume <0-30>`, `lights red`, `lights yellow`, `lights green`, `lights off`, `simulate highfive`, `reset`, and `help`, plus the audio controls above. Manual lamp commands persist until the next state transition; they do not change the state machine. `reset` stops pending reward playback and returns to red. In sequence mode, audio acceptance and lamp operations are simulated.

Boot-held switches are displayed as pressed but generate no high-five until released and pressed again. A press during red/yellow/reward is consumed and ignored by the state machine. A switch held into green cannot trigger a later reward automatically. Mount the switch inside the hand/arm so a high-five moves the hand enough to actuate it, without transferring the full impact into the switch or exposed wires.

### Audio initialization and errors

`begin()` starts asynchronous initialization, not a blocking success check. After a 3-second module settling period, the driver sends stop, select TF/SD, conservative volume, DAC enable, and a volume query, at least 200 ms apart. `[OK] YX5200` requires a valid checksummed volume reply matching the configured startup volume. A reply timeout produces `[ERROR] YX5200 initialization failed`; UART failure, SD removal, module errors, and runtime disconnects are also logged. FULL continues processing LEDs, sensor, timing, and diagnostics if audio fails.

Runtime commands go into a fixed eight-entry queue and return success **only for acceptance**, not audible playback. Invalid volumes/tracks, failed/not-ready audio, and queue overflow return failure. Commands do not wait for ACKs. The driver parses error/finish notifications and polls status every 5 seconds with a 1.5-second response timeout. Missing-file/out-of-range replies (module errors 5/6) after initialization keep the link usable and raise a one-shot error event; other module errors fail the driver. `status` reports cached driver health; there is no claim that the speaker is connected or that a file exists. Repair wiring/card issues and send `retry` to restart initialization. Commands received before audio is ready are rejected rather than replayed unexpectedly later. Stop discards pending playback and takes priority over health polling at the next permitted transmit slot.

FULL plays the boot sound once after audio becomes ready. A late startup never interrupts an active reward. Failed LED/sensor initialization, recoverable missing-track errors, and rejected diagnostic commands request the error sound when audio is available. AUDIO TEST accepts `boot` and `error` explicitly but does not play a boot cue automatically. An absent/broken MP3 module cannot emit an error sound; Serial remains the fallback. Failure to find the error cue itself is logged without recursively requesting it.

## SD-card layout and YX5200 compatibility

Use a FAT32 microSD card up to 32 GB with an MBR partition table. Run the importer with your **original filenames**; no manual renaming is needed:

```sh
python scripts/prepare_sd.py --output /Volumes/ROBOT \
  --boot /path/to/boot.mp3 --error /path/to/error.mp3 \
  --music '/path/to/My favourite song.wav' /path/to/more-music/
```

The importer accepts MP3, WAV, FLAC, M4A, OGG, WMA and other sources supported by the installed FFmpeg decoder. It converts them to metadata-free 44.1 kHz stereo, 128 kbps CBR MP3, checks that each output decodes, assigns IDs, and verifies copied hashes. It does not claim support for every possible codec, encrypted/DRM audio, or corrupt files. Unsupported sources fail before the destination is modified. It does not format disks.

The YX5200 still requires numeric addressing for deterministic playback; it cannot open `/system/boot.mp3` by pathname through UART. The importer handles that hardware constraint, retaining original system files under `/system` and creating playback copies automatically:

```text
SD root/
├── system/
│   ├── boot.mp3       # original supplied file
│   └── error.mp3      # original supplied file
├── MP3/
│   ├── 0001.mp3       # first imported music file: high-five reward
│   ├── ...
│   ├── 2998.mp3       # prepared boot sound
│   └── 2999.mp3       # prepared error sound
└── audio-manifest.json # original names, IDs, file sizes and SHA-256 hashes
```

`play 1` uses command **0x12 (MP3-folder addressing)** for `/MP3/0001.mp3`, not FAT copy-order track selection. Config.h reserves tracks 2998/2999 for boot/error and leaves 1–2997 for imported music. Explicit input order determines music IDs; folders are scanned in sorted path order. The first music file is the reward. `next`/`previous` use native card enumeration, which can include the original system copies and is not guaranteed to follow numeric ordering. Use `play N`, `boot`, or `error` for deterministic selection. The importer removes AppleDouble `._` metadata sidecars for generated files after copying. macOS can recreate them while the volume is mounted; safely eject the card after preparation.

To update an already prepared card, repeat the import command with `--replace` and the **entire desired music collection**. Replacement verifies the existing manifest and hashes, preserves unrelated files, refuses collisions with unowned files, and removes obsolete generated tracks. Keep the card attached until verification completes; conversion occurs first, but copying multiple files is not a filesystem transaction. Retain source audio on the laptop so an interrupted write can be rebuilt. Do not drag arbitrary audio directly into MP3 and expect format conversion or ID assignment to happen on the module.

For the current card, `high-enough.wav` is converted to reward track 1. The original source remains on the laptop. The 30-second reward limit is enforced by firmware, so changing that duration does not require trimming the audio or preparing the card again. See [the SD preparation record](docs/sd-card-preparation.md) for the prepared files.

The driver directly implements the YX5200/DFPlayer-compatible 10-byte protocol at 9600 baud, 8N1. DFRobot documentation is used as a **protocol reference**, not as a requirement to replace the existing YX5200. Module variants can differ; AUDIO TEST verifies the actual module. The checksum is the 16-bit two's complement of bytes 1–6 (version through parameter-low). For volume 15, the frame is `7E FF 06 06 00 00 0F FE E6 EF`. Some older manual examples contain inconsistent checksums; this implementation follows the algorithm and tests complete frames and corrupted/fragmented replies.

## Configuration and architecture

All tunable defaults are in [`include/Config.h`](include/Config.h): the three LED GPIOs, sensor/UART GPIOs, UART number/baud, serial baud, animation/cycle timings, state durations, debounce, reward track, volume, boot selection, UART pacing/timeouts/queue capacity, and serial buffer limits.

| Setting | Default |
| --- | --- |
| ESP32 UART2 RX / TX | GPIO16 ← YX5200 TX / GPIO17 → YX5200 RX |
| Red / yellow / green LED outputs | GPIO18 / GPIO19 / GPIO23 |
| High-five input | GPIO27 |
| Serial / MP3 UART baud | 115200 / 9600 |
| LED drive / chase frame interval | Active-high on/off / 80 ms |
| Red / yellow / reward | 3000 / 1000 / 30000 ms |
| Green | Wait indefinitely |
| Debounce | 30 ms |
| Reward / boot / error track | 1 / 2998 / 2999 |
| Initial volume | 12 (supported volume range 0–30) |
| Boot selection timeout / default | 5000 ms / FULL |

To move an LED, change `Config::Pins::LedRed`, `LedYellow`, or `LedGreen` and its matching wire. Keep three distinct, output-capable GPIOs that do not conflict with the sensor, UART, flash or carrier hardware. RobotController needs no changes. LED brightness is set by the LED/resistor combination; no PWM or color-order setting is used. LED initialization sets all outputs low, and each transition turns off the previous lamp before enabling the next.

To compile with another default mode, change the `DEFAULT_APP_MODE` fallback in Config.h or extend the ESP32 build flags in `platformio.ini`:

```ini
build_flags = ${env.build_flags} -DDEFAULT_APP_MODE=2
```

Modes are numbered 1–5 as in the menu. A serial selection still overrides the compile-time default. Do not use ESP32 flash-connected GPIO6–11 or change to boot-strapping pins without checking the carrier/module documentation.

```text
include/interfaces/     Clock, sensor, audio, traffic light, logger, and electrical IO contracts
include/core/          Pure C++ application and driver declarations
src/core/              State machine, debounce, LED chase, UART protocol, parser, diagnostics
include/hardware/      ESP32 adapter declarations
src/hardware/          Arduino GPIO/millis/UART/serial access
src/main.cpp           Owns and connects components; Arduino setup()/loop() only
test/                  Six native Unity suites and fake IO
scripts/               Native compiler/coverage integration
```

Core code uses injected interfaces, fixed buffers and bounded queues, with no Arduino calls, exceptions, or dynamic allocation. Hardware startup may allocate UART buffers. Every duration check uses unsigned subtraction; service the loop regularly (well within the 32-bit millis wrap period, about 49.7 days). Long loop stalls advance at most one state per update, preserving a visible interval for each state. Animation skips missed frames without an unbounded catch-up loop.

There are no application `delay()` calls or busy waits. Serial input, UART receive, and log transmission each have per-loop budgets. Logs use a bounded buffer, drop excess whole messages, and report overflow when the buffer drains. Each LED frame uses only GPIO writes; the 30-second reward animation does not block sensor or audio updates.

## Automated tests and coverage

With the development environment active:

```sh
pio test -e native
pio test -e native -f test_robot_controller
python scripts/coverage.py
python -m unittest discover -s test_host -v
```

`coverage.py` cleans **only the native build**, runs all six suites, and creates `coverage/index.html`, `coverage/summary.json`, and `coverage/cobertura.xml`. It fails below **90% line or 90% branch coverage**. It measures `src/core/` and `include/core/`, including protocol and diagnostic implementations; Arduino adapters, third-party libraries, and the fakes/test assertions are outside the denominator. It does not exclude untested core branches to improve the score.

On Linux, the script uses GCC's `gcov`; on macOS it uses Apple Clang and `xcrun llvm-cov gcov`. If using a different compiler version, set `GCOV` to the matching coverage executable, e.g. `GCOV=gcov-14 python scripts/coverage.py`. Compiler versions can produce slightly different branch totals. Do not merge counters from different source versions or compilers; the script cleans them first.

The tests cover state transitions, −1/exact/+1 timing boundaries and rollover; early/held/repeated high-fives; actual debounce bounce sequences; individual LED selection/off/chase, animation cancellation and restart; fixed UART frames, fragmented/corrupt input, bounded RX, command pacing/queue overflow, initialization/runtime failures, volume limits and stop priority; malformed serial input, boot defaults, and strict mode isolation. An integration test runs the real controller, diagnostics, sensor, LED selection and audio driver together through fake electrical IO.

GitHub Actions runs native tests/coverage and host importer/conversion tests on Linux and macOS and builds the ESP32 firmware on Linux for pushes and pull requests. It publishes coverage reports and firmware binaries as workflow artifacts. The importer tests include real WAV, MP3, FLAC and M4A conversion, arbitrary/Unicode filenames, read-back hashes, corrupt input, and safe library replacement.

## Hardware bring-up checklist

Make connections with power off. Use one stage at a time; a missing MP3 module must not prevent sensor or light tests. Do not connect the whole robot just to test one component.

1. **Set LM2596 to 5.0 V.** Leave the ESP32, MP3 and LEDs disconnected. Apply the 12 V supply, verify jack polarity, set/measure buck output, then turn power off. Check the output again under load later.
2. **Power ESP32 only.** Use USB with the external feed isolated as described above. Build/upload, open 115200-baud monitor, reset, verify the menu and boot timeout. Resolve USB/external power isolation before live externally powered tests.
3. **SENSOR TEST (`4`).** Connect only endstop 3V3/GND/SIGNAL. Verify RELEASED → PRESSED + exactly one HIGH FIVE EVENT → RELEASED. Hold it for several seconds: no repeated events. Tap/bounce it and tune debounce only if needed.
4. **LIGHTS TEST (`2`).** Connect GPIO18/19/23 through one resistor each to the red/yellow/green LED anodes; connect their cathodes to GND. Check each lamp, off, automatic cycle and `dance` (one lamp lit at a time in a repeating red → yellow → green chase). Sensor/MP3 are not required; LEDs can be tested with the ESP32 powered by USB alone.
5. **YX5200 AUDIO TEST (`3`).** Insert the prepared card while unpowered; connect 5V/GND/UART. Leave amplifier/speaker disconnected initially. Wait for verified `[OK] YX5200`, then try `play 1`, `pause`, `resume`, `stop`, `volume 12`, and `status`. UART success alone does not establish audible output.
6. **PAM8610 and speaker.** Power off, add resistor-summed DAC line audio to the left input and speaker across L+/L−. Power the amplifier from switched 12 V. Start its gain low. Repeat AUDIO TEST, listen for clean sound, and measure 5 V/12 V under playback load. Check for hot components or reset/brownout behavior. Never rewire speaker outputs while energized.
7. **SEQUENCE TEST (`5`).** Verify red (3 s), yellow (1 s), green (indefinite), then `highfive` → reward (30 s) → red. This mode requires no physical peripheral and does not play actual audio or drive LEDs.
8. **FULL (`1`).** Connect the tested components and use the physical hand. Verify music and dance start once on green, presses during red/yellow do not queue a reward, a held switch never retriggers, and a new press works on the next cycle. Disconnect/fix audio with power off and repeat to verify diagnostics and recovery. Test normal operation without a serial monitor.

Record results, module markings/carrier model, measured voltage under load, and any configuration changes in [`docs/hardware-validation.md`](docs/hardware-validation.md). Do not mark electrical/audio checks passed based on unit tests.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| ESP32 will not boot/upload | USB data cable and correct port; stable 5 V/3.3 V; common ground; carrier 5V versus 3V3 labels; no 12 V exposure; BOOT/EN procedure; no added loads on flash/strapping pins. Disconnect peripherals to isolate the fault |
| YX5200 not responding | AUDIO TEST; GPIO17 TX → module RX and GPIO16 RX ← module TX; module power/ground and 3.3 V UART levels; 9600 8N1; valid card; wait for initialization; inspect timeout/error log, then `retry`. Clones may require longer startup/command timings or have protocol differences |
| No speaker audio | A queued command is not proof of playback. Verify `/MP3/0001.mp3`, valid MP3, volume, amplifier 12 V supply/gain/mute state, DAC_L/R resistor sum to line input, input ground, and speaker across one channel's +/−. Leave SPK outputs unused |
| Audio distortion | Lower MP3 volume and amplifier gain, check clipped source audio, resistor sum, supply sag, loose connections and board temperature. Do not expect clean continuous 15 W from the marketing label |
| LED stays dark | LIGHTS TEST; correct GPIO, individual series resistor, LED polarity and common GND. Check the LED's forward voltage/current requirements; never bypass its resistor or apply 5 V to the GPIO |
| Wrong lamp lights | Check red → GPIO18, yellow → GPIO19, green → GPIO23 against Config::Pins and actual header labels. Each LED is a fixed color; swap the wiring or configured GPIOs |
| Endstop always pressed/released | Check the module's labeled VCC/GND/SIGNAL and switch actuator; power at 3.3 V; measure released HIGH/pressed LOW; inspect connector reversal and switch contact selection. A disconnected input typically reads released through the pull-up |
| ESP32 resets when audio gets loud | Measure 12 V and 5 V under load; inspect wire/connector resistance, ground distribution, buck thermal/current limits, amplifier gain and short circuits. Keep amplifier current off breadboards and ESP32 supply wiring |
| Sensor or LEDs work but audio reports failure | Expected isolation: use the individual modes, repair audio, then `retry`. A sensor/LED initialization log verifies software setup, not external wiring |
| Commands do nothing | Finish boot selection with Enter before timeout; check mode, lowercase syntax and `help`. Wait for audio ready. Reboot to change modes. Watch queue/full/input-overflow errors |

## References

- [Espressif ESP32-WROOM-32D/32U datasheet](https://documentation.espressif.com/esp32-wroom-32d_esp32-wroom-32u_datasheet_en.html): module pinout and electrical limits.
- [PlatformIO esp32dev board](https://docs.platformio.org/en/stable/boards/espressif32/esp32dev.html): board target, build/upload configuration.
- [DFRobot protocol implementation](https://github.com/DFRobot/DFRobotDFPlayerMini/blob/master/DFRobotDFPlayerMini.cpp) and [module reference](https://wiki.dfrobot.com/dfr0299/docs/20905): compatible UART command IDs, MP3-folder addressing and checksum algorithm. No DFRobot library is used by this firmware.
- [YX5200 manufacturer's module manual](https://datasheet4u.com/pdf-down/Y/X/5/YX5200-24SS-YueXin.pdf): numeric addressing and module error responses. Arbitrary-path playback is not exposed by this UART protocol.
- [Diodes PAM8610 datasheet](https://www.diodes.com/datasheet/download/PAM8610.pdf): supply, bridge outputs and load/power characteristics.
- [gcovr documentation](https://www.gcovr.com/en/8.3/): coverage commands and compiler-specific data handling.
