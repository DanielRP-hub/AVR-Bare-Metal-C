/**
 * @file main.c
 * @brief Multi-Channel Fast PWM for RGB LED Control.
 * @details Utilizes Timer4 to generate three synchronized Fast PWM signals (Channels A, B, and C)
 * to control the intensity of an RGB LED. External hardware interrupts update the duty cycle
 * of each channel independently in 5% increments.
 * @author Daniel Ruiz Pérez
 * @date 2024-06-18
 */ 

#define F_CPU 16000000UL // Microcontroller Frequency
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "LCD/lcd.h"
#include <stdio.h>
#include <stdlib.h>

// Button Pin Definitions (PORTD)
#define BUTTON1 PD0 // INT0
#define BUTTON2 PD1 // INT1
#define BUTTON3 PD3 // INT3

// Duty cycle variables in percentage (0-100)
volatile uint8_t duty_cycle1 = 0; // Red
volatile uint8_t duty_cycle2 = 0; // Green
volatile uint8_t duty_cycle3 = 0; // Blue

// LCD formatting buffers
char duty1[32];
char duty2[32];
char duty3[32];

/**
 * @brief Initializes Timer4 for 3-channel Fast PWM generation.
 * @details Configured for Fast PWM (Mode 14) with ICR4 acting as TOP.
 * Frequency is set to ~5 kHz at 16MHz using a 64 prescaler.
 */
void timer4_init() {
    // Mode 14: Fast PWM with ICR4 as TOP. 
    // Clear OC4A/OC4B/OC4C on Compare Match, set at BOTTOM (non-inverting mode).
    TCCR4A = (1 << COM4A1) | (1 << COM4B1) | (1 << COM4C1) | (1 << WGM41);
    TCCR4B = (1 << WGM43) | (1 << WGM42) | (1 << CS41) | (1 << CS40); // Prescaler 64

    // TOP value calculation for 5 kHz: (16MHz / (64 * 5000Hz)) - 1 = 49
    ICR4 = 49;

    // Initial duty cycle set to 0% (Outputs off)
    OCR4A = 0;
    OCR4B = 0;
    OCR4C = 0;

    // Enable hardware PWM output pins
    DDRH |= (1 << PH3) | (1 << PH4) | (1 << PH5); 
}

/**
 * @brief Configures external interrupt pins for user input.
 */
void buttons_init() {
    // Correctly clear bits to configure as inputs
    DDRD &= ~((1 << BUTTON1) | (1 << BUTTON2) | (1 << BUTTON3));
    
    // Enable internal pull-up resistors
    PORTD |=  (1 << BUTTON1) | (1 << BUTTON2) | (1 << BUTTON3);		

    // Configure INT0, INT1, and INT3 for Rising Edge triggers
    EICRA |= (1 << ISC31) | (1 << ISC30) | (1 << ISC01) | (1 << ISC00) | (1 << ISC10) | (1 << ISC11);			
    
    // Enable interrupt vectors
    EIMSK |= (1 << INT3) | (1 << INT0) | (1 << INT1);															
}

/**
 * @brief Formats the duty cycle percentages and outputs them to the LCD.
 */
void escritura() {
    uint8_t d1 = (duty_cycle1 / 10);
    uint8_t d12 = (duty_cycle1 % 10);
    
    uint8_t d2 = (duty_cycle2 / 10);
    uint8_t d22 = (duty_cycle2 % 10);
    
    uint8_t d3 = (duty_cycle3 / 10);
    uint8_t d32 = (duty_cycle3 % 10);
    
    sprintf(duty1, "%d%d", d1, d12);
    sprintf(duty2, "%d%d", d2, d22);
    sprintf(duty3, "%d%d", d3, d32);
    
    lcd_clrscr();
    lcd_gotoxy(2,0);
    lcd_puts("R:");
    lcd_gotoxy(5,0);
    lcd_puts(duty1);
    lcd_puts("%");
    
    lcd_gotoxy(10,0);
    lcd_puts("G:");
    lcd_gotoxy(12,0);
    lcd_puts(duty2);
    lcd_puts("%");
    
    lcd_gotoxy(5,1);
    lcd_puts("B:");
    lcd_gotoxy(8,1);
    lcd_puts(duty3);
    lcd_puts("%");
}

int main(void) {
    timer4_init(); 
    buttons_init(); 
    lcd_init(LCD_DISP_ON);

    sei(); // Enable global interrupts

    while (1) {
        escritura();
        _delay_ms(100); // UI Refresh rate limit
    }
}

// ---------------- Interrupt Service Routines (ISRs) ----------------

/**
 * @brief INT0 ISR - Increments Blue channel (Timer4, Channel A) duty cycle.
 */
ISR(INT0_vect) {
    if (duty_cycle3 <= 95) {
        duty_cycle3 += 5;
    } else {
        duty_cycle3 = 0;
    }
    // 16-bit casting prevents integer overflow during multiplication before division
    OCR4A = (uint16_t)(duty_cycle3 * (ICR4 + 1)) / 100;
}

/**
 * @brief INT1 ISR - Increments Green channel (Timer4, Channel B) duty cycle.
 */
ISR(INT1_vect) {
    if (duty_cycle2 <= 95) {
        duty_cycle2 += 5;
    } else {
        duty_cycle2 = 0;
    }
    OCR4B = (uint16_t)(duty_cycle2 * (ICR4 + 1)) / 100;
}

/**
 * @brief INT3 ISR - Increments Red channel (Timer4, Channel C) duty cycle.
 */
ISR(INT3_vect) {
    if (duty_cycle1 <= 95) {
        duty_cycle1 += 5;
    } else {
        duty_cycle1 = 0;
    }
    OCR4C = (uint16_t)(duty_cycle1 * (ICR4 + 1)) / 100;
}