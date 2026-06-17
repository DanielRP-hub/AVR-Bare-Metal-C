/**
 * @file main.c
 * @brief Interrupt-driven digital clock with alphanumeric LCD interfacing.
 * @details Demonstrates asynchronous event handling using hardware interrupts (ISRs) 
 * to manage user inputs independently of the main execution loop.
 * @author Daniel Ruiz Pérez
 * @date 2024-04-06
 */

#define F_CPU 16000000UL // Corrected to 16 MHz standard AVR clock

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "LCD/lcd.h"
#include <stdio.h>

// Volatile keyword used to prevent compiler optimization on variables modified inside ISRs
volatile uint8_t milisegundos = 0;
volatile uint8_t segundos = 0;
volatile uint8_t minutos = 0;
volatile uint8_t horas = 0;
volatile uint8_t stop = 0;
char data[32];

/**
 * @brief Formats the time variables into a string and sends them to the LCD.
 */
void escritura() {
    uint8_t h1 = (horas / 10);
    uint8_t h2 = (horas % 10);
    uint8_t m1 = (minutos / 10);
    uint8_t m2 = (minutos % 10);
    uint8_t s1 = (segundos / 10);
    uint8_t s2 = (segundos % 10);
    
    lcd_gotoxy(0,0);
    sprintf(data, "%d%d : %d%d : %d%d", h1, h2, m1, m2, s1, s2);
    lcd_clrscr();
    
    lcd_gotoxy(1,0);
    lcd_puts("DIGITAL CLOCK");
    
    lcd_gotoxy(1,1);
    lcd_puts(data);
}

/**
 * @brief Software counter simulating a clock tick.
 */
void reloj() {
    if (!stop) {
        segundos++;
        if (segundos == 60) {
            segundos = 0;
            minutos++;
            if (minutos == 60) {
                minutos = 0;
                horas++;
                if (horas == 24) {
                    horas = 0;
                }
            }
        }
    }
    _delay_ms(100); // 100ms simulation tick
}

/**
 * @brief Configures the External Interrupt Control Registers and sets data direction.
 */
void init_interrupts() {
    // Set PD0, PD1, PE4, PE5 as inputs for external interrupts
    DDRD &= ~((1 << 0) | (1 << 1));  
    DDRE &= ~((1 << 4) | (1 << 5));  
    
    // Configure INT0 and INT4 for Rising Edge
    // Configure INT1 and INT5 for Falling Edge using proper bitwise clearing/setting
    EICRA = (EICRA & ~(1 << ISC10)) | (1 << ISC01) | (1 << ISC00) | (1 << ISC11); 
    EICRB = (EICRB & ~(1 << ISC50)) | (1 << ISC41) | (1 << ISC40) | (1 << ISC51); 
    
    // Enable external interrupts INT0, INT1, INT4, INT5
    EIMSK |= ((1 << INT0) | (1 << INT1) | (1 << INT4) | (1 << INT5)); 
    
    sei(); // Enable global interrupts
}

int main(int argc, char** argv) {
    DDRB = 0xff; // Configure PORTB as output for LCD
    
    lcd_init(LCD_DISP_ON_CURSOR);
    lcd_clrscr();
    init_interrupts(); 
    
    while(1) {
        reloj();
        escritura();
    }

    return 0;
}

// ---------------- Interrupt Service Routines (ISRs) ----------------

/**
 * @brief ISR for INT0: Increments minutes when the clock is stopped.
 */
ISR(INT0_vect) {
    if(stop == 1) {
        minutos++;
        if (minutos == 60) {
            minutos = 0;
        }
    }
}

/**
 * @brief ISR for INT1: Increments hours when the clock is stopped.
 */
ISR(INT1_vect) {
    if (stop == 1) {
        horas++;
        if (horas == 24) {
            horas = 0;
        }
    }
}

/**
 * @brief ISR for INT4: Stops the clock timekeeping.
 */
ISR(INT4_vect) {
    stop = 1;
}

/**
 * @brief ISR for INT5: Resumes the clock timekeeping.
 */
ISR(INT5_vect) {
    stop = 0;
}