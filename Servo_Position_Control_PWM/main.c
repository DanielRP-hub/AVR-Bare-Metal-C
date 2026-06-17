/**
 * @file main.c
 * @brief Servo Motor Position Control via Hardware PWM and Keypad HMI.
 * @details Implements Phase and Frequency Correct PWM on Timer5 to generate a precise 50Hz signal. 
 * Duty cycle is dynamically updated via a 4x4 matrix keypad to actuate a servo motor from 0 to 180 degrees.
 * @author Daniel Ruiz Pérez
 * @date 2024-06-19
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "LCD/lcd.h"

volatile uint16_t grados = 0;
volatile uint16_t ancho_pulso = 0;
volatile uint8_t activar_motor = 0;

// Bug Fix: Increased array sizes to safely accommodate 3 digits plus the null terminator '\0'
char grade[8] = {'\0'};
char buffer[4] = {'\0'};

/**
 * @brief Scans the 4x4 matrix keypad and decodes the pressed key.
 * @return The character corresponding to the pressed key, or '\0' if none.
 */
char keypad_read(void) {
    char keypad[4][4] = { {'1','2','3','A'},
                          {'4','5','6','B'},
                          {'7','8','9','C'},
                          {'*','0','#','D'} };
    
    uint8_t FILAS[4] = {0xFE, 0xFD, 0xFB, 0xF7}, nf, dato;
    
    // Row sweep scanning logic
    for (nf = 0; nf <= 3; nf++) {
        PORTA = FILAS[nf];
        _delay_us(100);
        
        // Column read with bitmask
        dato = PINA & 0xF0; 
        _delay_us(100);
        
        if (dato != 0xF0) {
            switch (dato) {
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
 * @brief Initializes Timer5 for 50Hz Phase and Frequency Correct PWM.
 * @details Uses ICR5 (or OCR5A in Mode 9) as TOP to establish the 20ms period required by standard servos.
 */
void init_TIMER5() {
    // Mode 9: Phase and Frequency Correct PWM, TOP = OCR5A
    TCCR5A = (1 << WGM50) | (1 << COM5B1);
    TCCR5B = (1 << WGM53) | (1 << CS51); // Prescaler 8
    TCCR5C = 0x00;
    
    // Set TOP value for 50Hz frequency
    OCR5A = 20000;
    
    // Initial pulse width calculation based on 0 degrees
    ancho_pulso = 800 + (grados * 8);
    OCR5B = ancho_pulso;
    
    // Enable Output Compare Pin (PL4 maps to OC5B)
    DDRL |= (1 << 4);
}

/**
 * @brief Parses numerical keypad input into a ring buffer for degree assignment.
 */
void actualizar_buffer() {
    char tecla = keypad_read();
    
    if (activar_motor == 0) {
        if (tecla >= '0' && tecla <= '9') {
            int len = strlen(buffer);
            if (len < 3) {
                buffer[len] = tecla;
                buffer[len + 1] = '\0';
            } else {
                // Shift digits left if buffer is full
                for (int i = 0; i < len - 1; i++) {
                    buffer[i] = buffer[i + 1];
                }
                buffer[len - 1] = tecla;
            }
            grados = atoi(buffer);
        }
    }
}

/**
 * @brief Evaluates control keys to actuate the motor or reset the input state.
 */
void servo_motor(){
    char key = keypad_read();
    if (key != '\0') {
        switch (key){
            case 'A': // Confirm and execute angle
                activar_motor = 1;
                ancho_pulso = 800 + (grados * 8);
                OCR5B = ancho_pulso;
                break;
            case 'B': // Reset and re-configure
                activar_motor = 0;
                grados = 0;
                memset(buffer, '\0', sizeof(buffer));
                break;
        }
    }
}

/**
 * @brief Handles LCD UI formatting based on the motor's operational state.
 */
void mensajes(){
    // Optimization: Let standard C library handle the integer-to-string conversion
    sprintf(grade, "%d", grados);
    
    switch(activar_motor){
        case 0:
            lcd_clrscr();
            lcd_gotoxy(0,0);
            lcd_puts("  INGRESE UN ANGULO");
            lcd_gotoxy(0,1);
            lcd_puts("     0-180");
            lcd_gotoxy(0,3);
            lcd_puts("ANGULO = ");
            lcd_gotoxy(10,3);
            lcd_puts(grade);
            lcd_gotoxy(14,3);
            lcd_puts("grados");
            break;
        case 1:
            if(grados > 180){
                lcd_clrscr();
                lcd_gotoxy(0,0);
                lcd_puts("    ERROOOOOOOOOR");
                lcd_gotoxy(0,1);
                lcd_puts("INGRESE UN VALOR");
                lcd_gotoxy(7,2);
                lcd_puts("VALIDO");
            } else {
                lcd_clrscr();
                lcd_gotoxy(0,0);
                lcd_puts("  EL MOTOR ESTA EN:");
                lcd_gotoxy(0,3);
                lcd_puts("ANGULO =  ");
                lcd_gotoxy(11,3);
                lcd_puts(grade);
                lcd_gotoxy(15,3);
                lcd_puts("grados");
            }
            break;
    }
}

int main(void) {
    init_TIMER5();
    lcd_init(LCD_DISP_ON);
    lcd_clrscr();
    
    DDRA = 0x0F;  // Keypad rows as outputs (PA0-PA3)
    PORTA = 0xFF; // Keypad columns as inputs with pull-ups (PA4-PA7)
    
    while (1){
        mensajes();
        actualizar_buffer();
        servo_motor();
        _delay_ms(200); // UI Refresh and debounce delay
    }
}