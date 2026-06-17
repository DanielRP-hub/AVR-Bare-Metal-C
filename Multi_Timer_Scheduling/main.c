/**
 * @file main.c
 * @brief Multi-Timer Asynchronous Scheduling on ATmega2560.
 * @details Configures four independent 16-bit hardware timers (Timer 1, 3, 4, 5) 
 * to generate precise strobe pulses at 1s, 2s, 3s, and 4s intervals. 
 * Demonstrates pre-scaler calculations, precise TCNT reloading, and ISR state management.
 * @author Daniel Ruiz Pérez
 * @date 2024-06-17
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// State counters for software-extended timer intervals
volatile uint8_t led_2s = 0;
volatile uint8_t led_3s = 0;
volatile uint8_t led_4s = 0;

/**
 * @brief Initializes Timer 1 for a 1-second overflow timebase.
 * @details Output mapped to PORTH, Pin 0.
 */
void init_TIMER1() {
    TCCR1A = 0x00;
    TCCR1B |= (1 << CS12); // Prescaler 256
    TCCR1C = 0x00;
    TIMSK1 |= (1 << TOIE1); // Enable overflow interrupt
    TCNT1 = 3036; // Pre-load value for exactly 1Hz at 16MHz/256
    
    DDRH |= (1 << 0);
    PORTH &= ~(1 << 0); // Correctly clear the output pin using bitwise AND
}

/**
 * @brief Initializes Timer 3 for a 1-second overflow timebase.
 * @details Output mapped to PORTL, Pin 2.
 */
void init_TIMER3() {
    TCCR3A = 0x00;
    TCCR3B |= (1 << CS32); // Prescaler 256
    TCCR3C = 0x00;
    TIMSK3 |= (1 << TOIE3); 
    TCNT3 = 3036;
    
    DDRL |= (1 << 2);
    PORTL &= ~(1 << 2);
}

/**
 * @brief Initializes Timer 4 for a 1-second overflow timebase.
 * @details Output mapped to PORTC, Pin 6.
 */
void init_TIMER4() {
    TCCR4A = 0x00;
    TCCR4B |= (1 << CS42); // Prescaler 256
    TCCR4C = 0x00;
    TIMSK4 |= (1 << TOIE4); 
    TCNT4 = 3036;
    
    DDRC |= (1 << 6);
    PORTC &= ~(1 << 6);
}

/**
 * @brief Initializes Timer 5 for a 1-second overflow timebase.
 * @details Output mapped to PORTF, Pin 5.
 */
void init_TIMER5() {
    TCCR5A = 0x00;
    TCCR5B |= (1 << CS52); // Prescaler 256
    TCCR5C = 0x00;
    TIMSK5 |= (1 << TOIE5); 
    TCNT5 = 3036;
    
    DDRF |= (1 << 5);
    PORTF &= ~(1 << 5);
}

int main(void) {
    init_TIMER1();
    init_TIMER3();
    init_TIMER4();
    init_TIMER5();
    
    sei(); // Enable global interrupts
    
    while (1) {
        // Main loop remains empty. All timing is handled asynchronously by hardware.
    }
    return 0;
}

// ---------------- Interrupt Service Routines (ISRs) ----------------

/**
 * @brief Timer 1 ISR - Triggers every 1 second.
 */
ISR(TIMER1_OVF_vect) {
    TCNT1 = 3036; // Reload TCNT immediately to prevent drift
    PORTH ^= (1 << 0);
    _delay_ms(10); // 10ms strobe pulse
    PORTH ^= (1 << 0);
}

/**
 * @brief Timer 3 ISR - Triggers every 1 second, actuates every 2 seconds.
 */
ISR(TIMER3_OVF_vect) {
    TCNT3 = 3036; 
    if (led_2s >= 1) {
        PORTL ^= (1 << 2);
        _delay_ms(10);
        PORTL ^= (1 << 2);
        led_2s = 0;
    } else {
        led_2s++;
    }
}

/**
 * @brief Timer 4 ISR - Triggers every 1 second, actuates every 3 seconds.
 */
ISR(TIMER4_OVF_vect) {
    TCNT4 = 3036; 
    if (led_3s >= 2) {
        PORTC ^= (1 << 6);
        _delay_ms(10);
        PORTC ^= (1 << 6);
        led_3s = 0;
    } else {
        led_3s++;
    }
}

/**
 * @brief Timer 5 ISR - Triggers every 1 second, actuates every 4 seconds.
 */
ISR(TIMER5_OVF_vect) {
    TCNT5 = 3036; 
    if (led_4s >= 3) {
        PORTF ^= (1 << 5);
        _delay_ms(10);
        PORTF ^= (1 << 5);
        led_4s = 0;
    } else {
        led_4s++;
    }
}