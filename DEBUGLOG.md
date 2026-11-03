# Debug log

Chronological. Kept as it happened, including the faults I caused. Dates are from the commit history.

---

## Stage 1 — threshold alarm (basic version)

**Goal:** light an LED and sound a buzzer when the room gets loud.

**Finding P1.0.** I could not find `P1.0` on the board's silkscreen. Boards in this class often omit the dot to save space and label the pin `P10`. Ten minutes lost.

**The "always ringing" bug.** After downloading the first program, the buzzer sounded continuously from power-on.

My procedure was: unplug the DO pin from the KY-037, turn the potentiometer until the module's indicator LED just goes out, plug DO back in — and it still rang.

The cause was an assumption about hardware, not a bug in software. The module's `DO` pin reports the comparator's state, and **which logic level means "loud" is not standardised** across LM393 board manufacturers. The adjustment procedure makes this worse rather than better: turning the potentiometer until the module's own indicator LED goes out puts the comparator *at* its trip point, so the pin then reports the opposite of the state you just dialled in. Reading "LED off" as "quiet" and therefore "0" is an inference, and on this board it was the wrong one.

The comparison in `v1_threshold_alarm.c` was consistent with that wrong assumption, which is why it ran continuously. Inverting the comparison fixed it:

```c
if (SoundSensor == 1) {   /* where v1 had == 0 */
```

**The lesson is not "the module is reversed" — it is that a hardware convention I inferred rather than measured cost an afternoon.** The correct procedure is to put a meter on the pin in both states and read the actual levels, which takes two minutes and which I did not do. The file `v1_threshold_alarm.c` is kept in its failing state with a header comment describing how to verify the polarity instead of trusting a tutorial.

**Downloading.** Keil uVision4 for compilation, STC-ISP for flashing. The MCU must be power-cycled (cold start) at the moment STC-ISP begins the download; otherwise the handshake fails. This is not a software problem and the error message does not say so.

The first attempt failed completely: the USB cable I had was charge-only, with no data lines. Charging cables and data cables look identical and are not interchangeable. This is the incident described in my personal statement.

**A line that looks redundant and is not.** Before reading the sensor pin, `v1` writes to it:

```c
SoundSensor = 1;   /* required before reading on an 8051 */
```

The 8051 and its clones have quasi-bidirectional ports: weak pull-up, strong pull-down, and a read that returns the latch rather than the pin unless the latch has first been set to 1. Setting it to 1 releases the pin to the external signal. On an AVR or an ARM the same line would be dead code and would be deleted by anyone reviewing it — which is exactly why it survives in 8051 code and not elsewhere. Understanding this one line is what made the LCD's write-only wiring in Stage 4 legible rather than mysterious.

---

## Stage 2 — LCD1602 self-test

Before integrating the display into the main program I wrote a separate minimal program that only initialises the LCD and prints a fixed string:

```c
LCD_Init();
LCD_ShowString(1, 1, " Noise Meter  ");
LCD_ShowString(2, 1, "   TEST OK!   ");
while(1) { }
```

Sixteen lines, no ADC, no alarm logic. **Its purpose was diagnostic:** if this program produces a blank screen, the fault is in the wiring or the display; if it works, the fault is in the code I am about to add. Separating the two reduced the search space before the real build started.

This program is kept in the repository as `v2_lcd_selftest.c`.

---

## Stage 3 — ADC0832 + LCD integrated

Integrating the ADC and the display meant routing everything through a breadboard. Using the breadboard's vertical internal connections saved space, but debugging became far harder: one wrong row is invisible.

**Fault 1 — blank screen, faulty jumper.** The screen would not light at all. I tested continuity along each jumper in the data path and found one that read open. Replaced it. This is the cheapest possible fault and it still took time, because I started by checking the code.

**Fault 2 — stray header pin, power rail down.** A female header on the breadboard was shorting against a pin in the adjacent row, which removed the sensor's supply rail. The MCU survived.

That outcome was not luck. Three things were deliberate: power was switched off before every rewire and on only after the board was checked, the short was found by inspecting the pins before powering up rather than by noticing heat, and the short sat on the sensor's rail rather than the MCU's. The one component lost in the whole project was mechanical, not electrical — see fault 3.

**Fault 3 — continuous buzzing and blank screen after the short was cleared.** This was the worst state to be in, because both known faults had been dealt with and the system still did not work.

**What I did first: full rebuild.** I pulled every wire out and rewired the whole circuit from the diagram, one wire at a time, rather than hunting for the fault in place. The reasoning was that after two separate faults I no longer trusted the board: if the wiring itself was wrong somewhere I had not noticed, searching for a clever fault would waste hours. A full rebuild costs twenty minutes and eliminates "my wiring is wrong" as a category. **It did not fix it** — which was itself informative: the wiring was now known-good and the fault had to be in a component.

**Then, and only then, component-by-component.** With the wiring eliminated, I went through the components one at a time. The contrast potentiometer had failed. I had twisted it past its mechanical limit while chasing faults 1 and 2 — the adjustment is a multi-turn screw and I had turned it far past the end of its travel in both directions looking for a response that was never going to come from there. A worn potentiometer reads open or intermittent, and the contrast pin then floats, which produces a blank screen indistinguishable from a dead controller.

**So the only part destroyed in this project died from my hand, not from the circuit.** Nothing else was lost: no MCU, no display, no sensor. That is worth stating plainly rather than as good fortune — the electrical side was never the risk, because everything ran at 5 V with the power off during every change. The risk was that I did not know when to stop turning a screw.

**Workaround, and the mistake I made with it.** A potentiometer is a variable. I replaced it with a fixed resistor sized for the LCD's recommended contrast voltage, wired to the contrast pin. The screen lit up.

Then I tried the same thing with a second resistor and it did nothing at all. That was the more useful failure. The reason: I had pushed the resistor legs into the same breadboard rows as the existing jumper wires without stripping any insulation off the wires themselves. The resistor's own leads were bare, but the wire I was trying to tap into was still covered, so nothing was actually connected to anything — the resistor was sitting on the board as decoration. **The circuit had "changed" on the schematic and been unaltered in reality**, which is why the screen behaved exactly as before.

What fixed it was stripping a short section from the jumper and twisting the resistor lead onto bare copper before pushing it back into the row. The lesson is not about resistors. It is that **a breadboard makes it easy to build a connection that looks right, sits in the right holes, and conducts nothing**, and that "I made the change and nothing happened" has two very different explanations: the change was wrong, or the change was never made.

This is also why the final build is hard to photograph usefully: the repair is hidden inside a row of the board, and there is nothing to see.

---

## Stage 4 — timing

With the hardware stable, the display still showed garbage characters.

The LCD's read/write pin is tied to ground on the breadboard, so the display is write-only. That means all timing has to come from the code: the enable strobe must stay high long enough for the controller to latch the data, and the data lines must be stable before the strobe falls.

At 12 MHz the delay loops were too tight. Lengthening the delays fixed the garbage. **I had been treating a display problem as a display problem when it was a timing problem** — the characters were being latched mid-transition.

---

## Stage 5 — conversion and display

*On the code in this section: I did not write the drivers. `v3` uses an existing ADC0832 driver and an existing LCD1602 driver, which I adapted rather than authored. My changes were the conversion arithmetic, the display formatting, and the sampling loop. I am recording that here because the hardware work below is mine and I do not want the two confused.*

The first conversion used floating point:

```c
float db = 30.0 + (adc_val / 255.0) * 70.0;
sprintf(buf, "Val: %2.1f dB   ", db);
```

It worked. It also pulled in the floating-point library and the full printf formatter, which on an 8 KB part is a large fraction of the available flash, and slowed the loop enough to matter.

Replaced with fixed-point:

```c
unsigned int reading_x10 = 300 + ((unsigned int)adc_val * 700) / 255;
/* print as: reading_x10 / 10, '.', reading_x10 % 10 */
```

Overflow check before making the change: `255 * 700 = 178,500`, which fits in `unsigned int` on this compiler (16-bit, max 65,535) only as `unsigned long`. Using `unsigned long` for the intermediate is required; using `unsigned int` silently truncates. **Writing down the boundary condition before the edit is what stopped a silent overflow bug.**

---

## What is still wrong

- The buzzer routine blocks the main loop for its entire duration (50 ms), and the loop then waits another 100 ms. A short impulse can fall entirely within that window and never be sampled. `v1` was worse: `Beep(300)` followed by `delay_ms(300)` left the loop blind for 600 ms, and because the LED was switched off only after the buzzer finished, the alarm was visible for exactly as long as the sound lasted and no longer.
- The alarm threshold is a compile-time constant in `v3`, and on `v1` it is a screwdriver.
- The reading is uncalibrated and unweighted. See the Limitations section of the README.

None of these are fixed. They are the first three items on the list if I rebuild it.

---

## What I would measure first

Every fault above was found by reasoning about what could be wrong. The one procedure that would have found all of them faster is measurement rather than inference: a multimeter on the sensor pin in both states would have settled the polarity in two minutes, and a logic analyser on the LCD bus would have shown the timing fault directly instead of leaving me to deduce it from garbage characters. I did not have either for most of this project, and the cost shows up as roughly a week of substitutions and guesses.
