/*
 * v1_threshold_alarm.c
 * Noise detector, first working version.
 * STC89C52RC (8051-compatible), KY-037 sound module, onboard LED and buzzer.
 *
 * Behaviour: when the sound module's comparator trips, the LED lights and the
 * passive buzzer sounds for ~300 ms, then the loop waits another 300 ms before
 * re-arming.
 *
 * Known defects, kept as they were rather than retro-fixed:
 *   - Beep() blocks for its full duration; the loop is blind while it runs.
 *   - The threshold is set by a screwdriver on the module, not in software.
 *   - The comparator output polarity is module-dependent. See note below.
 *
 * NOTE ON POLARITY (this cost an afternoon):
 *   The KY-037 / LM393 module has an adjustable threshold. Turning the
 *   potentiometer until the module's own indicator LED goes out means the
 *   comparator is sitting at its trip point; the DO pin then reads the
 *   *other* state. Which logic level means "loud" is not standardised across
 *   manufacturers, so the comparison below must be checked against the actual
 *   module before trusting it. This version triggers on 0; if the buzzer runs
 *   continuously in a quiet room, invert the comparison.
 */

#include <reg51.h>

sbit SoundSensor = P1^0;   /* module DO -> P1.0 */
sbit LED         = P2^0;   /* onboard LED, active low */
sbit BEEP        = P2^5;   /* onboard passive buzzer */

/* Coarse delay. Loop count is empirically tuned for a 12 MHz crystal. */
void delay_ms(unsigned int ms)
{
    unsigned int i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 110; j++);
}

/* Passive buzzer needs a square wave, not a DC level. */
void Beep(unsigned int duration_ms)
{
    unsigned int i;
    unsigned int cycles = duration_ms * 2;

    for (i = 0; i < cycles; i++) {
        BEEP = ~BEEP;      /* toggle: one half period */
        delay_ms(1);
    }
    BEEP = 1;              /* stop */
}

void main(void)
{
    /*
     * 8051 ports are quasi-bidirectional. To read a pin as an input, the latch
     * must first be written to 1 so the weak pull-up takes over; otherwise the
     * pin reads back the latch value, not the external signal. This applies to
     * the 8051 and its clones (STC89C52 included), not to AVR or ARM ports.
     */
    SoundSensor = 1;

    LED  = 1;              /* LED off (active low) */
    BEEP = 1;              /* buzzer silent */

    while (1) {
        if (SoundSensor == 0) {   /* comparator tripped */
            LED = 0;              /* LED on */
            Beep(300);            /* blocking: ~300 ms of no sampling */
            LED = 1;              /* LED off */
            delay_ms(300);        /* blocking: another 300 ms */
        } else {
            LED  = 1;
            BEEP = 1;
        }
    }
}
