# Hardware commissioning record

Status: **not yet performed on physical hardware**. Native tests and a successful firmware build do not complete these checks.

| Item | Observation |
| --- | --- |
| Date / operator | Pending |
| Firmware commit | Pending |
| ESP32 carrier model and USB/5V power-path verification | Pending |
| YX5200 module marking / card format | Pending |
| LED part / count / group configuration | Pending |
| PAM8610 board and speaker markings | Pending |
| Unloaded LM2596 output | Pending: target 5.0 V |
| Loaded 5 V / 12 V, including loudest intended playback | Pending |

- [ ] Buck set and measured before attaching 5 V devices.
- [ ] ESP32 USB-only boot, upload and serial menu.
- [ ] SENSOR TEST: press/release, bounce, hold, second press, mounted hand.
- [ ] LIGHTS TEST: all lamps, off, cycle and every pixel's RGB dance.
- [ ] AUDIO TEST: verified UART initialization, play/pause/resume/stop, volume, next/previous.
- [ ] Resistor-summed DAC audio through one amplifier channel produces clean sound.
- [ ] No grounded speaker output, channel bridging, or YX5200 SPK-to-amplifier wiring.
- [ ] SEQUENCE TEST runs through every transition using serial highfive.
- [ ] FULL runs through every transition using the physical endstop.
- [ ] Early/held press does not retrigger; later release and new press work.
- [ ] Missing audio remains diagnosable and `retry` works after repair.
- [ ] No resets, overheating, loose conductors or voltage sag at intended sound level.
- [ ] Normal operation without serial attached and safe final mounting.

Record faults and measured corrections here before checking an item off.
