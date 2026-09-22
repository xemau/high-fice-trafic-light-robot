# High-Five Traffic-Light Robot

PlatformIO / Arduino / C++17 firmware for the existing **ESP32-WROOM-32D development board** and **six four-leg common-anode RGB LEDs, wired as three pairs: top, middle and bottom**. The robot displays red for 3 seconds, yellow for 1 second, then waits on green for a debounced high-five. A high-five randomly selects one of `/MP3/0001.mp3` through `/MP3/0003.mp3` and starts a unique full-spectrum RGB fade. At 26 seconds the animation changes to the middle yellow pair; at 30 seconds audio stops and the robot returns to red.

The automatic red phase remains 3 seconds, below the 5-second maximum. The 30-second reward timer starts when the high-five triggers the reward, not when audible playback begins. The yellow warning starts at 26 seconds. At the deadline, firmware immediately sends one stop command and shows red. After another 3-second red and 1-second yellow phase, green waits indefinitely for a new button press. Hardware entropy and button timing vary the pseudo-random track sequence, which avoids playing the same reward track twice consecutively. Firmware plays no boot or failure sound; only reward tracks 1–3 are selected automatically.

The reward animation continuously fades between seeded, randomized HSV colors. Targets take 2–3.5 seconds to reach, with gamma-corrected brightness updates every 20 ms; no deliberate black frame or sudden strobe is part of the dance. Hardware PWM runs at 5 kHz, while the visible color motion remains below one transition per second. The ESP32 seeds the generator at boot and every animation mixes in its start time and evolving random state, so successive rewards do not repeat a fixed sequence. This reduced-flash design uses the W3C limit of no more than three flashes per second as a conservative reference, but it cannot guarantee that every person will tolerate the effect. Brightness, contrast, viewing distance and individual sensitivity still matter. See [W3C G19](https://www.w3.org/WAI/WCAG22/Techniques/general/G19) and the [Epilepsy Foundation photosensitivity guidance](https://www.epilepsy.com/what-is-epilepsy/seizure-triggers/photosensitivity).

The ESP32 controls a **YX5200 Mini MP3 module** over UART2; the YX5200 decodes the audio. Its DAC outputs feed one PAM8610 amplifier channel and the existing 4 Ω speaker. No additional microcontroller or replacement MP3 module is needed.

## Status and verification

The ESP32 firmware builds, and 50 native tests pass across six suites, plus 12 host tests for SD preparation, audio conversion and PWM output. A clean Apple Clang coverage run measured **100% core line coverage and 96.0% branch coverage**. Reproduce these results with the commands below; generated reports are ignored by Git.

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
| 1 | Cherry DB1 mechanical microswitch (COM/NO/NC contacts; no powered endstop module) |
| 6 | Four-leg common-anode RGB LEDs, reported label `LB E2-03-04-FCN RGB CA`; two per traffic-light position |
| 2 | Approximately 1 kΩ resistors for analog mono summing |
| 18 | LED current-limiting resistors, initially 470 Ω each: one per color channel per physical LED |
| 9 | Recommended 10 kΩ pull-up resistors from each RGB GPIO to 3V3, keeping channels off while GPIOs are inputs during reset/boot |
| As needed | Wire, solder, heat-shrink, proper distribution connectors, optional perfboard and removable connectors |

No Arduino Uno, second ESP32, DFPlayer-branded replacement, DY-SV5W, different speaker, battery pack, or finished-installation breadboard is required.

## Power and wiring

For a standalone, offline-friendly visual guide, open [the wiring diagram](docs/wiring.html) in a browser. It includes the RGB pair pin map, individual resistor branches, power/audio connections and first-test checklist.

**Never apply 12 V to the ESP32, YX5200, endstop, or LEDs. Adjust the disconnected LM2596 to 5.0 V with a multimeter before connecting any 5 V devices.** The 5V connection below is the carrier board's regulated-input pin, not the WROOM module's 3.3 V supply pin. All six LED common anodes connect to ESP32 3V3; their color cathodes connect through individual resistors to GPIOs that sink current. Never connect these common anodes to 5 V with this direct-GPIO circuit. Confirm the carrier's labels and pinout; WROOM-32D identifies the module, not the carrier's USB power circuit.

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
                         `-- DB1 COM / terminal 1
```

Use a soldered/perfboard ground bus, suitable connectors, or WAGO-style distribution. Run separate power/return branches to the amplifier and buck converter; keep heavy speaker/amplifier return currents out of the sensor and audio-signal return wiring. Do not daisy-chain device power or route amplifier power through a breadboard. Insulate joints and mount the amplifier/buck so they can dissipate heat away from cardboard.

### Complete connection table

GPIO numbers are ESP32 GPIO labels, **not physical header positions**. Check the molded contact labels on the DB1 and connector labels on the amplifier; physical orientation varies.

| From | To | Notes |
| --- | --- | --- |
| PSU +12 V via DC jack | Rocker switch input | Verify polarity and switch's DC rating |
| Rocker switch output | PAM8610 VCC and LM2596 IN+ | Both receive switched +12 V |
| PSU negative | Common GND bus | Positive supply is switched; ground stays common |
| GND bus | PAM8610 power GND; LM2596 IN− and OUT− | Buck input/output grounds share the same system |
| LM2596 OUT+ (5.0 V) | ESP32 carrier 5V; YX5200 VCC | Separate branches; never the ESP32 3V3 pin |
| GND bus | ESP32 GND; YX5200 GND; DB1 COM / terminal 1 | Connect before applying signals; RGB common legs go to 3V3, not GND |
| ESP32 GPIO17 / UART2 TX | YX5200 RX | ESP32 transmits; optional ~1 kΩ series resistor if UART noise warrants it |
| YX5200 TX | ESP32 GPIO16 / UART2 RX | ESP32 receives, 9600 baud, 8N1; confirm module TX is 3.3 V logic |
| DB1 NO / terminal 4 | ESP32 GPIO27 | Normally open; pressing connects GPIO27 to GND via COM. Firmware enables input pull-up |
| DB1 NC / terminal 2 | Leave disconnected and insulated | No 3V3, 5V or 12V connection to the switch |
| ESP32 3V3 | Common anode (+) of all six RGB LEDs | Confirm the common leg from the LED's pinout or diode test |
| RGB GPIOs in table below | Corresponding color cathodes through one 470 Ω resistor per LED/channel | LOW = on, HIGH = off; two separate resistor branches per GPIO |
| ESP32 3V3 | 10 kΩ pull-up → each RGB GPIO | Nine recommended pull-ups; these do not replace the 18 LED series resistors |
| YX5200 DAC_L | First 1 kΩ resistor → mono sum node | Line audio, **not SPK output** |
| YX5200 DAC_R | Second 1 kΩ resistor → same mono sum node | Never short DAC_L and DAC_R together |
| Mono sum node | PAM8610 left audio input | Use the board's line-input terminal/jack, not a speaker output |
| YX5200 GND | PAM8610 input GND | Common signal reference, short wiring |
| PAM8610 L+ | Speaker + | Use one amplifier channel only |
| PAM8610 L− | Speaker − | Both speaker terminals are driven; neither goes to GND |
| PAM8610 R+ / R− | Leave unconnected | Do not bridge channels; terminate unused input only as board instructions specify |

### DB1 high-five switch

The Cherry DB1 is a bare mechanical switch, not a VCC/GND/SIGNAL module. Wire **COM (1) to GND**, **NO (4) to GPIO27**, and leave **NC (2) unused**. The ESP32's internal pull-up holds the input HIGH when released; pressing closes COM–NO and pulls it LOW. The current firmware already implements this polarity and 30 ms debounce, so no code change is required.

Do not wire 3V3 to NC: with COM grounded, that would short the supply while released. The 3V3 LED supply is separate from the DB1 wiring. Identify contacts by markings or a power-off continuity test: COM–NO closes only when pressed, while COM–NC opens when pressed. Do not guess terminal position from a generic switch picture. Verify repeated presses; the published DB1 power-switch ratings alone do not guarantee contact reliability at GPIO pull-up currents.

### RGB pairs and GPIO budget

Each pair has three shared controls (R, G and B), so six RGB packages need **9 GPIOs rather than 18**. Both LEDs in a pair always show the same color; the three pairs remain independent. Do not join the color controls of different pairs.

| Pair / position | Red cathodes | Green cathodes | Blue cathodes | Normal traffic color |
| --- | --- | --- | --- | --- |
| Top: LEDs 1–2 | GPIO18 | GPIO19 | GPIO23 | Red only |
| Middle: LEDs 3–4 | GPIO25 | GPIO26 | GPIO32 | Red + green = yellow |
| Bottom: LEDs 5–6 | GPIO33 | GPIO21 | GPIO22 | Green only |

The application uses 12 GPIOs: nine RGB outputs, UART2 on GPIO16/17, and the sensor on GPIO27. GPIO4/13/14 remain unused output-capable candidates on the WROOM-32D, subject to the carrier's wiring; GPIO1/3 stay reserved for programming/serial. No GPIO expander or multiplexing is required. GPIO21/22 are assigned to LEDs, so do not also use them for I²C without remapping. The map avoids flash pins, boot-strapping pins and input-only pins; see the [Espressif pinout](https://documentation.espressif.com/esp32-wroom-32d_esp32-wroom-32u_datasheet_en.html).

Use **18 separate current-limiting resistors**: six LEDs × three channels. For each GPIO, connect two parallel branches, each containing its own resistor and one LED color cathode. Never share a single series resistor between LEDs or put one in the common-anode leg.

Start with 470 Ω per channel. For a channel with a 2.0 V forward drop, idealized current is `(3.3 - 2.0) / 470 ≈ 2.8 mA` per LED, or approximately 5.6 mA through a shared GPIO. Actual current also depends on the GPIO low voltage and the LED. Verify all channel currents, the board's 3.3 V regulator capacity and visible brightness before installation. Red/green brightness ratios affect the mixed yellow hue; resistor values can be increased per channel to balance it.

The exact datasheet and physical leg order for the reported `LB E2-03-04-FCN` label have not been verified. Confirm the common-anode and R/G/B legs by datasheet or diode test, not position alone. Green/blue channels may be dim if their forward voltage leaves too little headroom at 3.3 V. If brighter operation or a higher LED supply is needed, use suitable external current-sinking drivers and revise the wiring; **do not move the common anodes to 5 V while cathodes are connected directly to ESP32 GPIOs**.

For one color of one pair (repeat for all nine GPIOs):

```text
3V3 ---- LED 1 common anode
           LED 1 color cathode ---- 470Ω ----+
                                           +---- assigned GPIO
3V3 ---- LED 2 common anode                 |
           LED 2 color cathode ---- 470Ω ----+
3V3 -------------------------- 10kΩ --------+

LOW on GPIO lights both channels; HIGH turns them off.
No LED common leg connects to GND.
```

### Audio signal wiring

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

The dependencies are pinned in `platformio.ini`: Espressif32 6.10.0 and Arduino ESP32 2.0.17 via that platform. LEDs use Arduino GPIO output directly, with no LED library dependency. PlatformIO stores tools locally in `.pio-core/` and builds in `.pio/`. Firmware binaries are under `.pio/build/esp32dev/`. No hardware upload occurs when running native tests.

The uploaded firmware starts FULL immediately after reset; there is no boot menu or selectable hardware-test mode. A serial connection is optional. With PlatformIO monitor, `--filter send_on_enter` is useful for line editing. Exit the monitor with Ctrl+C before uploading.

## Production mode and serial controls

FULL initializes the sensor, LEDs and UART2 audio driver and immediately starts the red → yellow → green sequence. Internal diagnostic controllers remain covered by automated tests, but the production entry point never exposes their mode selector.

Optional serial commands are lowercase, one per line: `status`, `help`, `play <1-9999>`, `stop`, `pause`, `resume`, `volume <0-30>`, `next`, `previous`, `retry`, `lights red`, `lights yellow`, `lights green`, `lights off`, `simulate highfive`, and `reset`. There are no `boot` or `error` sound commands. Manual lamp commands persist until the next state transition; they do not change the state machine. `reset` stops pending reward playback and returns to red.

Boot-held switches are displayed as pressed but generate no high-five until released and pressed again. A press during red/yellow/reward is consumed and ignored by the state machine. A switch held into green cannot trigger a later reward automatically. Mount the switch inside the hand/arm so a high-five moves the hand enough to actuate it, without transferring the full impact into the switch or exposed wires.

### Audio initialization and errors

`begin()` starts asynchronous initialization, not a blocking success check. After a 3-second module settling period, the driver sends stop, select TF/SD, configured volume, DAC enable, the configured YX5200 equalizer preset, and a volume query, at least 200 ms apart. `[OK] YX5200` requires a valid checksummed volume reply matching the configured startup volume. The EQ command is requested but not verified by that handshake. A reply timeout produces `[ERROR] YX5200 initialization failed`; UART failure, SD removal, and explicit module errors are also logged. FULL continues processing LEDs, sensor, timing, and diagnostics if audio fails.

Runtime commands go into a fixed eight-entry queue and return success **only for acceptance**, not audible playback. Invalid volumes/tracks, failed/not-ready audio, and queue overflow return failure. Commands do not wait for ACKs. The driver parses error/finish notifications and attempts an optional status poll every 5 seconds. Some YX5200 variants do not answer status command `0x42`; after the first 1.5-second timeout, firmware logs one warning and disables further runtime polling while keeping the initialized audio link ready. Runtime module error replies are logged without permanently disabling the initialized UART link or discarding queued commands. A module error while a status query is outstanding also disables further polling. Startup errors, UART write failure and SD removal still fail the driver and require `retry`. A rejected playback command is not automatically replayed. `status` reports cached driver health; there is no claim that the speaker is connected or that a file exists. Repair wiring/card issues and send `retry` to restart initialization. Commands received before audio is ready are rejected rather than replayed unexpectedly later. Stop discards pending playback, cancels a pending health response, and transmits immediately.

Every transmitted command logs its timestamp, command byte and argument as `[AUDIO TX]`. Module error logs include the last transmitted command for context; the protocol does not identify which command an error belongs to. The boot log includes the firmware build date/time to identify the uploaded binary.

Initialization failures, missing tracks, module errors and rejected serial commands are logged only. They never select another audio track, so a high-five cannot be followed by a boot or failure cue.

## SD-card layout and YX5200 compatibility

Use a FAT32 microSD card up to 32 GB with an MBR partition table. Run the importer with your **original filenames**; no manual renaming is needed:

```sh
python scripts/prepare_sd.py --output /Volumes/ROBOT \
  --music '/path/to/My favourite song.wav' /path/to/more-music/
```

The importer accepts MP3, WAV, FLAC, M4A, OGG, WMA and other sources supported by the installed FFmpeg decoder. It converts them to metadata-free 44.1 kHz stereo, 128 kbps CBR MP3, checks that each output decodes, assigns IDs, and verifies copied hashes. It does not claim support for every possible codec, encrypted/DRM audio, or corrupt files. Unsupported sources fail before the destination is modified. It does not format disks.

The YX5200 requires numeric addressing for deterministic playback. The importer creates only numbered reward tracks:

```text
SD root/
├── MP3/
│   ├── 0001.mp3       # first imported music file: high-five reward
│   └── ...
└── audio-manifest.json # original names, IDs, file sizes and SHA-256 hashes
```

`play 1` uses command **0x12 (MP3-folder addressing)** for `/MP3/0001.mp3`, not FAT copy-order track selection. Explicit input order determines music IDs; folders are scanned in sorted path order. The first music file is the first reward track. `next`/`previous` use native card enumeration and are not guaranteed to follow numeric ordering. Use `play N` for deterministic selection. The importer removes AppleDouble `._` metadata sidecars for generated files after copying. macOS can recreate them while the volume is mounted; safely eject the card after preparation.

To update an already prepared card, repeat the import command with `--replace` and the **entire desired music collection**. Replacement verifies the existing manifest and hashes, preserves unrelated files, refuses collisions with unowned files, and removes obsolete generated tracks. Keep the card attached until verification completes; conversion occurs first, but copying multiple files is not a filesystem transaction. Retain source audio on the laptop so an interrupted write can be rebuilt. Do not drag arbitrary audio directly into MP3 and expect format conversion or ID assignment to happen on the module.

The current card contains three randomized reward tracks: Pufino's “Rock Me Now” as track 1, Aetheric's “Snap Crackle” as track 2, and Moavii's “Root” as track 3. The original sources remain on the laptop. The 30-second reward limit is enforced by firmware, so changing that duration does not require trimming or recopying the audio. See [the SD preparation record](docs/sd-card-preparation.md) for the prepared files and hashes.

The driver directly implements the YX5200/DFPlayer-compatible 10-byte protocol at 9600 baud, 8N1. DFRobot documentation is used as a **protocol reference**, not as a requirement to replace the existing YX5200. Module variants can differ; use the FULL-mode `play N` and `status` commands to verify the actual module. The checksum is the 16-bit two's complement of bytes 1–6 (version through parameter-low). For volume 15, the frame is `7E FF 06 06 00 00 0F FE E6 EF`. Some older manual examples contain inconsistent checksums; this implementation follows the algorithm and tests complete frames and corrupted/fragmented replies.

## Configuration and architecture

All tunable defaults are in [`include/Config.h`](include/Config.h): the nine RGB GPIOs, sensor/UART GPIOs, UART number/baud, serial baud, animation timings, state durations, debounce, reward tracks, volume, YX5200 equalizer preset, UART pacing/timeouts/queue capacity, and serial buffer limits.

| Setting | Default |
| --- | --- |
| ESP32 UART2 RX / TX | GPIO16 ← YX5200 TX / GPIO17 → YX5200 RX |
| Top pair R / G / B | GPIO18 / GPIO19 / GPIO23 |
| Middle pair R / G / B | GPIO25 / GPIO26 / GPIO32 |
| Bottom pair R / G / B | GPIO33 / GPIO21 / GPIO22 |
| High-five input | GPIO27 |
| Serial / MP3 UART baud | 115200 / 9600 |
| RGB drive / animation | Common-anode, active-low 5 kHz 8-bit PWM / 20 ms updates / randomized 2–3.5 s fades |
| Red / yellow / reward warning / reward end | 3000 / 1000 / 26000 / 30000 ms |
| Green | Wait indefinitely |
| Debounce | 30 ms |
| Reward tracks | 1–3 (random, no immediate repeat) |
| Initial volume | 30 (supported volume range 0–30) |
| YX5200 equalizer | Jazz / 3 (Normal=0, Pop=1, Rock=2, Jazz=3, Classic=4, Bass=5) |

To remap the LEDs, edit `Config::Pins::LedRgb`: rows are top/middle/bottom pairs and columns are R/G/B. Compile-time checks reject duplicate pins, sensor/UART conflicts and pins outside the safe output list. RobotController needs no changes. The nine channels use the ESP32 LEDC peripheral for independent active-low PWM and software gamma correction; resistor values still affect current and color balance. Initialization drives all channels HIGH (off) before attaching PWM. Recommended external pull-ups keep them off before initialization.

The production entry point always starts FULL. Do not use ESP32 flash-connected GPIO6–11 or change to boot-strapping pins without checking the carrier/module documentation.

```text
include/interfaces/     Clock, sensor, audio, traffic light, logger, and electrical IO contracts
include/core/          Pure C++ application and driver declarations
src/core/              State machine, debounce, RGB pair animation, UART protocol, parser, diagnostics
include/hardware/      ESP32 adapter declarations
src/hardware/          Arduino GPIO/millis/UART/serial access
src/main.cpp           Owns and connects components; Arduino setup()/loop() only
test/                  Six native Unity suites and fake IO
scripts/               Native compiler/coverage integration
```

Core code uses injected interfaces, fixed buffers and bounded queues, with no Arduino calls, exceptions, or dynamic allocation. Hardware startup may allocate UART buffers. Every duration check uses unsigned subtraction; service the loop regularly (well within the 32-bit millis wrap period, about 49.7 days). Long loop stalls advance at most one state per update, preserving a visible interval for each state. Animation skips missed frames without an unbounded catch-up loop.

There are no application `delay()` calls or busy waits. Serial input, UART receive, and log transmission each have per-loop budgets. Logs use a bounded buffer, drop excess whole messages, and report overflow when the buffer drains. Each fade update writes nine hardware PWM duties; the reward animation and final yellow warning do not block sensor or audio updates.

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

The tests cover state transitions, −1/exact/+1 timing boundaries and rollover; randomized reward-track bounds and repeat avoidance; early/held/repeated high-fives; actual debounce bounce sequences; RGB pair selection, the final yellow warning, seeded repeatability, successive animation variation, smooth fade deltas, full-spectrum targets, animation cancellation and restart; fixed UART frames, fragmented/corrupt input, bounded RX, command pacing/queue overflow, initialization failures, optional status-poll fallback, volume limits, stop delivery, a second reward playback without runtime status replies, and the absence of system-sound commands and cues. An integration test runs the real controller, diagnostics, sensor, RGB pair frames and audio driver together through fake electrical IO.

GitHub Actions runs native tests/coverage and host importer/conversion tests on Linux and macOS and builds the ESP32 firmware on Linux for pushes and pull requests. It publishes coverage reports and firmware binaries as workflow artifacts. A host test compiles the production GPIO adapter against a recording Arduino stub and checks PWM initialization, active-low polarity, pin/channel mapping, gamma correction and setup failure; it does not measure real electrical behavior. The importer tests include real WAV, MP3, FLAC and M4A conversion, arbitrary/Unicode filenames, read-back hashes, corrupt input, and safe library replacement.

## Hardware bring-up checklist

Make connections with power off. Use one stage at a time; a missing MP3 module must not prevent sensor or light tests. Do not connect the whole robot just to test one component.

1. **Set LM2596 to 5.0 V.** Leave the ESP32, MP3 and LEDs disconnected. Apply the 12 V supply, verify jack polarity, set/measure buck output, then turn power off. Check the output again under load later.
2. **Power ESP32 only.** Use USB with the external feed isolated as described above. Build/upload, open the 115200-baud monitor, reset and verify that FULL starts immediately. Resolve USB/external power isolation before live externally powered tests.
3. **Sensor.** With power off, wire DB1 COM/1 → GND and NO/4 → GPIO27; leave NC/2 disconnected. No switch power wire is needed. In FULL, verify that a press on green produces exactly one HIGH FIVE log and reward. Hold it for several seconds: no repeated events. Test release and a second press.
4. **Lights.** First verify each LED's common-anode/R/G/B pinout. Connect all six common anodes to 3V3, and each color cathode through its own 470 Ω resistor to the GPIO in the pair table (18 series resistors total). Add the nine GPIO pull-ups. Use `lights red`, `lights yellow`, `lights green` and `lights off` to verify pair mapping. Trigger a reward to verify slow, continuous full-spectrum fades with no visible PWM flicker or abrupt black frames. Measure currents and check mixed-yellow visibility, blue/green brightness, and off behavior during reset.
5. **YX5200 audio.** Insert the prepared card while unpowered; connect 5V/GND/UART. Leave amplifier/speaker disconnected initially. Wait for verified `[OK] YX5200`, then try `play 1`, `pause`, `resume`, `stop`, `volume 12`, and `status`. UART success alone does not establish audible output.
6. **PAM8610 and speaker.** Power off, add resistor-summed DAC line audio to the left input and speaker across L+/L−. Power the amplifier from switched 12 V. Start its gain low. Repeat playback, listen for clean sound, and measure 5 V/12 V under load. Check for hot components or reset/brownout behavior. Never rewire speaker outputs while energized.
7. **Complete sequence.** Verify red (3 s), yellow (1 s), green (indefinite), high-five → reward animation → yellow at 26 s → stop and red at 30 s. Confirm the next green waits for a release and new press, plays another reward, and never plays a boot or failure cue. Test normal operation without a serial monitor.

Record results, module markings/carrier model, measured voltage under load, and any configuration changes in [`docs/hardware-validation.md`](docs/hardware-validation.md). Do not mark electrical/audio checks passed based on unit tests.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| ESP32 will not boot/upload | USB data cable and correct port; stable 5 V/3.3 V; common ground; carrier 5V versus 3V3 labels; no 12 V exposure; BOOT/EN procedure; no added loads on flash/strapping pins. Disconnect peripherals to isolate the fault |
| YX5200 not responding | In FULL, inspect `status`; check GPIO17 TX → module RX and GPIO16 RX ← module TX, module power/ground, 3.3 V UART levels, 9600 8N1 and the card; then send `retry`. Clones may require longer startup/command timings or have protocol differences |
| Audio stops working after one reward | Capture the boot build timestamp, `[AUDIO TX]` and error lines through the second press, then send `status`. `PLAY N queued` means accepted by firmware; `cmd=0x12 arg=N` means written to UART, neither proves sound. If `audio=failed`, send `retry`, wait for `[OK] YX5200`, then try `play 1`, `play 2`, `play 3` individually and record which work |
| No speaker audio | A queued command is not proof of playback. Verify `/MP3/0001.mp3`, valid MP3, volume, amplifier 12 V supply/gain/mute state, DAC_L/R resistor sum to line input, input ground, and speaker across one channel's +/−. Leave SPK outputs unused |
| Audio distortion | Lower MP3 volume and amplifier gain, check clipped source audio, resistor sum, supply sag, loose connections and board temperature. Do not expect clean continuous 15 W from the marketing label |
| LED stays dark | Use the FULL-mode `lights` commands; check common anode to 3V3, correct R/G/B cathode and individual resistor to GPIO. LOW lights the channel. Check forward voltage/headroom and both separate branches; never bypass a resistor or connect the LED common anode to 5 V in this circuit |
| Wrong pair/color lights | Compare all nine R/G/B connections with the pair table and Config::Pins::LedRgb. Middle yellow requires both red and green, with blue off. Correct leg mapping before adjusting resistor balance; both LEDs in each pair should match |
| Endstop always pressed/released | Check DB1 COM/1 → GND and NO/4 → GPIO27, with NC/2 unused and no switch power wire. Test contacts with power off; verify released HIGH/pressed LOW when powered. A disconnected input reads released through the pull-up. Using NC reverses the expected behavior |
| ESP32 resets when audio gets loud | Measure 12 V and 5 V under load; inspect wire/connector resistance, ground distribution, buck thermal/current limits, amplifier gain and short circuits. Keep amplifier current off breadboards and ESP32 supply wiring |
| Sensor or LEDs work but audio reports failure | Repair audio, then send `retry`. A sensor/LED initialization log verifies software setup, not external wiring |
| Commands do nothing | Check lowercase syntax and `help`, wait for audio ready, and inspect queue/full/input-overflow logs. Production firmware always remains in FULL |

## References

- [Cherry DB-series datasheet](https://www.neuhold-elektronik.at/media/6d/1b/17/1636722530/N4652_Datenblatt.pdf?ts=1636722530): mechanical switch and NC/NO/Common contact diagram on page 1. [ZF switch terminal definitions](https://switches-sensors.zf.com/switches-lexicon/): COM = 1, NC = 2, NO = 4; contact operation and low-current considerations.
- [Espressif ESP32-WROOM-32D/32U datasheet](https://documentation.espressif.com/esp32-wroom-32d_esp32-wroom-32u_datasheet_en.html): module pinout and electrical limits.
- [PlatformIO esp32dev board](https://docs.platformio.org/en/stable/boards/espressif32/esp32dev.html): board target, build/upload configuration.
- [DFRobot protocol implementation](https://github.com/DFRobot/DFRobotDFPlayerMini/blob/master/DFRobotDFPlayerMini.cpp) and [module reference](https://wiki.dfrobot.com/dfr0299/docs/20905): compatible UART command IDs, MP3-folder addressing and checksum algorithm. No DFRobot library is used by this firmware.
- [YX5200 manufacturer's module manual](https://datasheet4u.com/pdf-down/Y/X/5/YX5200-24SS-YueXin.pdf): numeric addressing and module error responses. Arbitrary-path playback is not exposed by this UART protocol.
- [Diodes PAM8610 datasheet](https://www.diodes.com/datasheet/download/PAM8610.pdf): supply, bridge outputs and load/power characteristics.
- [gcovr documentation](https://www.gcovr.com/en/8.3/): coverage commands and compiler-specific data handling.
