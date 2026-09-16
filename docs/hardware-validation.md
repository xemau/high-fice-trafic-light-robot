# Hardware commissioning record

Status: **not yet performed on physical hardware**. Native tests and a successful firmware build do not complete these checks.

| Item | Observation |
| --- | --- |
| Date / operator | Pending |
| Firmware commit | Pending |
| ESP32 carrier model and USB/5V power-path verification | Pending |
| YX5200 module marking / card format | Pending |
| Six common-anode RGB LEDs: actual leg order, forward voltages and channel currents | Pending; reported label LB E2-03-04-FCN RGB CA, exact datasheet unverified |
| Three paired RGB outputs, 18 series resistors, nine pull-ups and 3V3 supply capacity | Pending; use README's nine-GPIO map |
| PAM8610 board and speaker markings | Pending |
| Unloaded LM2596 output | Pending: target 5.0 V |
| Loaded 5 V / 12 V, including loudest intended playback | Pending |

- [ ] Buck set and measured before attaching 5 V devices.
- [ ] ESP32 USB-only boot, upload and serial menu.
- [ ] SENSOR TEST: press/release, bounce, hold, second press, mounted hand.
- [ ] LIGHTS TEST: top red, middle red+green yellow, bottom green; only the selected pair lights.
- [ ] Both LEDs of every pair match; dance cycles each pair through red, green and blue.
- [ ] All channels off during reset and on `off`; verify resistor currents, mixed-yellow hue and blue/green brightness at 3.3 V.
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
