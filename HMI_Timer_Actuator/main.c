/**
 * @file main.c
 * @brief HMI Countdown Timer with Keypad, LCD, and Audible Alarm.
 * @details Implements a user-configurable countdown timer. Demonstrates input debouncing, 
 * state-machine UI logic, hardware timer interrupts for the timebase, and GPIO actuator control.
 * @author Daniel Ruiz Pérez
 * @date 2024-06-18
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "LCD/lcd.h"

// Timekeeping variables changed to signed integers (int8_t) to safely handle sub-zero logic
int8_t M = 0; 
int8_t S = 0;
uint8_t M1, M2, S1, S2;

uint8_t p = 1; // Pause/Run flag (1 = Paused, 0 = Running)
uint8_t j = 0; // State variable for digit input
char array[32];
char data;

/**
 * @brief Initializes Timer1 for 1Hz ticks and configures GPIO for the keypad and buzzer.
 */
void init_timer1_teclado(void) {
    // Timer 1 Configuration: Normal mode, Prescaler 256
    TCCR1A = 0x00;
    TCCR1B |= (1 << CS12);
    TCCR1C = 0x00;
    TIMSK1 |= (1 << TOIE1);
    TCNT1 = 3035; // Pre-load for 1-second overflow at 16MHz/256
    
    // Keypad Matrix Configuration (PORTA)
    DDRA = 0x0F;  // Lower nibble (Rows) as output, Upper nibble (Cols) as input
    PORTA = 0xFF; // Enable pull-ups on inputs, set rows high
    
    // Buzzer Configuration (PORTF, Pin 7)
    DDRF |= (1 << 7);
    PORTF &= ~(1 << 7);
    
    sei(); // Enable global interrupts
}

/**
 * @brief Scans the 4x4 matrix keypad and decodes the pressed key.
 * @return The character corresponding to the pressed key, or '\0' if none.
 */
char keypad_read() {
    char keypad[4][4] = { 
        {'1','2','3','A'},
        {'4','5','6','B'},
        {'7','8','9','C'},
        {'*','0','#','D'} 
    };
    
    uint8_t FILAS[4] = {0xFE, 0xFD, 0xFB, 0xF7};
    uint8_t nf, dato;
    
    for (nf = 0; nf <= 3; nf++) {
        PORTA = FILAS[nf];
        _delay_us(100);
        
        dato = PINA & 0xF0; // Mask column inputs
        _delay_us(100);
        
        if (dato != 0xF0) {
            switch(dato) {
                case 0x70: return keypad[nf][0];
                case 0xB0: return keypad[nf][1];
                case 0xD0: return keypad[nf][2];
                case 0xE0: return keypad[nf][3];
            }
        }
    }
    return '\0'; 
}

/**
 * @brief Traps execution until a numeric key is pressed, ignoring control characters.
 */
void ComNum(void) {  
    while(data == 'A' || data == 'B' || data == 'C' || data == 'D' || data == '#' || data == '*') {
        data = keypad_read();
    }
}

/**
 * @brief Triggers the audible alarm and waits for the user acknowledgment (Button 'B').
 */
void alarma(void) {
    while(data != 'B') {
        p = 1; // Pause timer
        M = 0;
        S = 0;
        
        lcd_gotoxy(0,0);
        lcd_puts("                    ");
        lcd_gotoxy(0,1);
        lcd_puts("    PRESIONA 'B'    ");
        lcd_gotoxy(0,2);
        lcd_puts("    PARA APAGAR     ");
        
        PORTF |= (1 << 7); // Turn on buzzer
        _delay_ms(10);
        data = keypad_read();
    }
    
    if (data == 'B') {
        PORTF &= ~(1 << 7); // Turn off buzzer
    }
}

/**
 * @brief Updates the logical countdown state and handles minute rollovers.
 */
void contar(void) {
    p = 0; // Set state to running
    
    if (S < 0) { // Safely catches below-zero due to int8_t type
        S = 59;
        M--;
    }
    
    M1 = M / 10;
    M2 = M % 10;
    S1 = S / 10;
    S2 = S % 10;
    
    if ((M == 0) && (p == 0) && (S == 0)) {
        alarma();
    }
}

/**
 * @brief Formats and renders the current time variables to the LCD.
 */
void display(void) {
    lcd_gotoxy(0,0);
    lcd_puts("                    ");
    lcd_gotoxy(0,1);
    lcd_puts("    TEMPORIZADOR    ");
    
    sprintf(array,"        %d%d:%d%d       ", M1, M2, S1, S2);
    lcd_gotoxy(0,2);
    lcd_puts(array);
    _delay_ms(10);
}

/**
 * @brief Displays an error message for out-of-bounds input values.
 */
void falla(void) {
    lcd_gotoxy(0,0);
    lcd_puts("                    ");
    lcd_gotoxy(0,1);
    lcd_puts(" ERROR, SOLO INGRESE");
    lcd_gotoxy(0,2);
    lcd_puts("VALORES ENTRE 1 Y 59");
    _delay_ms(2000);
}

/**
 * @brief Main UI state machine handling user input logic for setting and running the timer.
 */
void acciones() {
    data = keypad_read();
    
    if (data != '\0') {
        // Set Seconds Logic
        if (data == 'D') {
            p = 1; // Pause
            while(data == 'D') { data = keypad_read(); } // Wait for release
            
            S = 60; // Force loop entry
            while(S > 59) {
                j = 0;
                while(j < 2) {
                    data = keypad_read();
                    ComNum();
                    if(data != '\0') {
                        if(j == 0) { S1 = data - '0'; display(); j++; }
                        else if(j == 1) { S2 = data - '0'; display(); j++; }
                        while(data != '\0') { data = keypad_read(); } // Wait for release
                    }
                    S = (S1 * 10) + S2;
                    display();
                }
                if(S > 59) { falla(); }
            }
        }
        
        // Set Minutes Logic
        if (data == 'C') {
            p = 1; // Pause
            while(data == 'C') { data = keypad_read(); } // Wait for release
            
            M = 60; // Force loop entry
            while(M > 59) {
                j = 0;
                while(j < 2) {
                    data = keypad_read();
                    ComNum();
                    if(data != '\0') {
                        if(j == 0) { M1 = data - '0'; display(); j++; }
                        else if(j == 1) { M2 = data - '0'; display(); j++; }
                        while(data != '\0') { data = keypad_read(); } // Wait for release
                    }
                    M = (M1 * 10) + M2;
                    display();
                }
                if(M > 59) { falla(); }
            }
        }
        
        // Run Timer Logic
        while (data == 'A') {
            p = 0;
            contar();
            display();
        }
    }
    display();
}

int main(void) {
    init_timer1_teclado();
    lcd_init(LCD_DISP_ON);
    
    while (1) {
        acciones();
    }
}

// ---------------- Interrupt Service Routines (ISRs) ----------------

/**
 * @brief Timer 1 ISR - Acts as the 1Hz timebase.
 * @details Decrements the seconds counter if the system is not paused.
 */
ISR(TIMER1_OVF_vect) {
    if (p != 1) {
        S--;
        TCNT1 = 3035; // Reload pre-scaler for next 1-second interval
    }
}