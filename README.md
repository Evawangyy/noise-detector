# Noise Meter (STC89C52RC / 8051)

A sound-level indicator built on an 8051-compatible MCU. It started as a threshold alarm and was rebuilt twice: once to display a numeric reading on an LCD1602, and once to fix a power fault that turned out to be my own doing.

This project was my first contact with embedded hardware. Almost everything below is a record of mistakes, kept in the order they happened.

> **Scope:** I had not studied programming before this. The drivers in `v3` are adapted from existing code, not written by me; the code I changed myself was the conversion, the display formatting and the sampling loop. The hardware — every wire, and every fault in the log — is mine. See [DEBUGLOG.md](DEBUGLOG.md) for the detail.

> **Note on the readings:** the number displayed is a *relative loudness index* on a 30–100 scale, **not** calibrated dB SPL. There was no reference sound source available for calibration. See [Limitations](#limitations).

---

## Repository layout

| File | What it is |
|---|---|
| `v1_threshold_alarm.c` | First working version. Sound module + LED + buzzer, no display. Kept in its failing state — the polarity bug is still in it, with a header comment on how to check. |
| `v2_lcd_selftest.c` | LCD1602 driver test only. Shows `TEST OK` and then halts, so a blank screen can be attributed to hardware or to code. |
| `v3_adc_lcd_meter.c` | Final version. ADC0832 + LCD1602 + threshold alarm, fixed-point conversion. |
| `DEBUGLOG.md` | Chronological fault log, including the three failures in the LCD build. |
| `noise detector middle1.jpg` … `middle3.jpg` | The breadboard build, photographed from three angles. |
| `noise detector(sample).jpg` | The finished device. |
| `noise detector video for sample one.mp4` | Device running: quiet room → clap → reading responds. |
| `test.hex`, `test2.hex` | Compiled firmware, kept for reference. |

`v2` exists for a specific reason: while debugging a blank screen I could not tell whether the fault was in the wiring or in the driver code. A minimal program that only initialises the LCD and prints a fixed string removes the driver from the list of suspects. It is the most useful twenty lines in this repository.

---

## Hardware

| Part | Role |
|---|---|
| STC89C52RC (8051-compatible) | MCU, 8 KB flash, 12 MHz crystal |
| KY-037 sound module (LM393 + electret mic) | Analogue sound pickup, comparator with adjustable threshold |
| ADC0832 | 8-bit serial ADC, software-driven SPI |
| LCD1602 | 16×2 character display, driven in 8-bit parallel mode (whole byte on P0) |
| LED + buzzer | Threshold alarm |
| Rotary potentiometer | LCD contrast (replaced by a fixed resistor, see log) |

Wiring details and pin assignments are in the header comment of `v3_adc_lcd_meter.c`.

---

## How it works

**1. Threshold stage.** The KY-037 provides both an analogue output and a comparator output. In `v1`, only the comparator output was used: the potentiometer sets the trip point, and the MCU reads the digital result.

**2. Conversion.** `v3` reads the module's analogue output through an ADC0832. The 8051 has no SPI peripheral, so the ADC has to be clocked bit by bit in software: CS low, three pulses to send the start bit and channel select, eight pulses to shift out the byte.

*I did not write this driver.* It is adapted from an existing one, and it is kept here with the header comment intact. Understanding it well enough to change the code around it — and to know which parts were not mine to touch — was the actual task.

**3. Mapping and filtering.** Five consecutive conversions are averaged, then mapped to a display value with fixed-point arithmetic:

```c
reading_x10 = 300 + (adc_val * 700) / 255;   /* 30.0 .. 100.0, tenths */
```

The first version used `float` and `sprintf("%2.1f")`. On this part that pulls in the floating-point library and the printf formatter, which together take a large share of the 8 KB of flash. Integer arithmetic with a manually inserted decimal point removed both. **The mapping is linear, which means the output is a relative index and not a sound level — see Limitations.**

**4. Output.** The value is written to the LCD1602, and the LED and buzzer trip above a fixed threshold.

---

## Debug log (summary)

Full detail in [`DEBUGLOG.md`](DEBUGLOG.md). The three faults worth remembering:

1. **Comparator polarity.** The buzzer sounded continuously from power-on. Which logic level the LM393 module's `DO` pin reports for "loud" is **not standardised** across manufacturers, and the adjustment procedure is counter-intuitive: turning the potentiometer until the module's own indicator LED goes out puts the comparator *at* its trip point, so the pin then reads the opposite state from the one just dialled in. The code was consistent with the assumption; the assumption was wrong. `v1_threshold_alarm.c` is kept exactly as it failed, with a header comment explaining how to check which way round it is.
2. **Faulty jumper wire.** Screen blank. Confirmed by substitution.
3. **Stray header pin.** A female header shorted against a neighbouring pin and removed the sensor's supply rail. The MCU survived — power was off during every rewire, the short was found by inspection rather than by smell, and the short sat on the sensor's rail rather than the MCU's.
4. **Failed contrast potentiometer.** After the short was cleared, the display stayed blank and the buzzer ran continuously. By then I had done a full rebuild — every wire out and back in from the diagram — which did not fix it, but did eliminate "the wiring is wrong" as a category. Only then did I go component by component, and found the potentiometer had failed: I had twisted a multi-turn screw far past its travel while chasing faults 2 and 3. **The only part lost in this project died from my hand, not from the circuit.**
5. **The repair that did nothing.** A later attempt at the same resistor replacement had no effect at all, because I pushed the leads into the rows without stripping insulation from the jumpers. Nothing was connected. See [`DEBUGLOG.md`](DEBUGLOG.md) — this one is more instructive than the fault it was meant to fix.

The lesson that generalises: **when a system fails immediately after a change, suspect the change before the design — and check that the change was actually made.** Every fault above was caused by something I had just done or just touched.

---

## Limitations

These are known and intentional, not oversights:

- **Uncalibrated.** A real dB SPL reading requires a logarithmic conversion against a reference pressure and, in practice, a 94 dB / 1 kHz calibrator. Neither was available. The output is comparable between environments but is not a measurement.
- **Unweighted.** The module's response follows the microphone's own frequency response. There is no A-weighting, so low-frequency noise is over-reported relative to human perception.
- **Impulse blind spots.** The loop averages five conversions and then waits 100 ms; the buzzer routine blocks the loop for its full duration. A short transient can fall entirely between samples. The correct fix is timer-interrupt-driven sampling with a non-blocking buzzer, which is not implemented here.
- **Fixed threshold.** The alarm trip point is a compile-time constant, not user-settable.

---

## One detail worth keeping

In `v1`, before the sound module's pin is ever read:

```c
SoundSensor = 1;
```

This looks redundant and is not. The 8051 and its clones have **quasi-bidirectional** ports: the pin has a weak pull-up and a strong pull-down, and reading the pin reads the *latch* unless the latch has first been written to 1. Writing 1 is what releases the pin to the external signal. This is specific to the 8051 family — on an AVR or an ARM the same line would be dead code, which is why it is the first thing to go when code is copied between architectures.

The same reasoning explains a second thing this project ran into: the LCD's read/write line was tied to ground, so the display could only be written to, and every timing requirement had to be met in software.

---

## If I rebuilt it

- Timer interrupt for sampling; the main loop should never block on a buzzer.
- A real calibrator, or at minimum a single-point calibration against a phone playing a known tone at a measured distance.
- I²C display to free GPIO pins.
- Proper PCB instead of a breadboard — most of the failures above were contact failures.

---

## Licence

MIT for the code. Photographs and video are my own.
