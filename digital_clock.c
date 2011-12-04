/* Firmware for a simple digital clock with 7-segment display. */

#define F_CPU 3686400UL

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define CENTISECOND_CYCLES F_CPU / 100

#define SEGMENT_PORT              PORTD
#define SEGMENT_DEC_POSN_PORT     PORTB
#define TIME_ADJ_PORT             PORTC
#define TIME_ADJ_PIN              PINC
#define SEGMENT_PORT_DDR          DDRD
#define SEGMENT_DEC_POSN_PORT_DDR DDRB
#define TIME_ADJ_PORT_DDR         DDRC

/*
1  Cathode E   PD0
2  Cathode D   PD1
3  Cathode DP  PD2
4  Cathode C   PD3
5  Cathode G   PD4
6  Anode D4    PB0

7  Cathode B   PD5
8  Anode D3    PB1
9  Anode D2    PB2
10 Cathode F   PD6
11 Cathode A   PD7
12 Anode D1    PB3
*/

#define SEG_E   1 << PIND0
#define SEG_D   1 << PIND1
#define SEG_DP  1 << PIND2
#define SEG_C   1 << PIND3
#define SEG_G   1 << PIND4
#define SEG_B   1 << PIND5
#define SEG_F   1 << PIND6
#define SEG_A   1 << PIND7

#define POSN_D1 1 << PINB3
#define POSN_D2 1 << PINB2
#define POSN_D3 1 << PINB1
#define POSN_D4 1 << PINB0

uint8_t digits[10] = {
    /* 0 */
    ~(SEG_DP|SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F),
    ~(SEG_DP|SEG_B|SEG_C),
    ~(SEG_DP|SEG_A|SEG_B|SEG_G|SEG_E|SEG_D),
    ~(SEG_DP|SEG_A|SEG_B|SEG_G|SEG_C|SEG_D),
    ~(SEG_DP|SEG_F|SEG_G|SEG_B|SEG_C),

    /* 5 */
    ~(SEG_DP|SEG_A|SEG_F|SEG_G|SEG_C|SEG_D),
    ~(SEG_DP|SEG_A|SEG_F|SEG_E|SEG_D|SEG_C|SEG_G),
    ~(SEG_DP|SEG_A|SEG_B|SEG_C),
    ~(SEG_DP|SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G),
    ~(SEG_DP|SEG_A|SEG_B|SEG_C|SEG_D|SEG_F|SEG_G)
}, positions[4] = {
    POSN_D1,
    POSN_D2,
    POSN_D3,
    POSN_D4
};

volatile uint8_t hours;
volatile uint8_t minutes;
volatile uint8_t seconds;
volatile uint8_t centiseconds;

ISR (TIMER1_COMPA_vect) {
    if (++centiseconds >= 100) {
        centiseconds = 0;
        if (++seconds >= 60) {
            seconds = 0;
            if (++minutes >= 60) {
                minutes = 0;
                if (++hours >= 24)
                    hours = 0;
            }
        }
    }
}

int
main (void) {
    uint8_t i;
    uint8_t seg, posn;
    uint8_t time_digs[4];

    hours = 0;
    minutes = 0;
    seconds = 0;
    centiseconds = 0;

    SEGMENT_PORT_DDR = 0xff;
    SEGMENT_DEC_POSN_PORT_DDR = 0xff;
    TIME_ADJ_PORT_DDR = 0x0;
    TIME_ADJ_PORT = 0xff;

    TCCR1B |= (1 << CS10 | 1 << WGM12); /* internal clock source, no prescaler; Clear Timer on Output Compare Match mode */
    OCR1A = CENTISECOND_CYCLES; /* set TOP counter value, so that an interrupt fires every 1/100 of a second */
    TIMSK |= (1 << OCIE1A); /* enable Ouput Compare A Match interrupt */

    sei();

    for (i = 0; /* empty */; ++i) {
        i %= 4;

        if (!(TIME_ADJ_PIN & 2)) {
            hours = (hours + 1) % 24;
            _delay_ms(200);
        }
        if (!(TIME_ADJ_PIN & 1)) {
            minutes = (minutes + 1) % 60;
            _delay_ms(200);
        }

        time_digs[0] = hours / 10;
        time_digs[1] = hours % 10;
        time_digs[2] = minutes / 10;
        time_digs[3] = minutes % 10;

        seg = digits[time_digs[i]];
        posn = positions[i];

        /* decimal point rotates right every second */
        if (i != seconds % 4) {
            seg |= SEG_DP;
        }

        SEGMENT_PORT = seg;
        SEGMENT_DEC_POSN_PORT = posn;
    }

    return 0;
}
