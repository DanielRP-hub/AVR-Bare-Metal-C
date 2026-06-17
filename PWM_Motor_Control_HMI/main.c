/**
 * @file main.c
 * @brief DC Motor Control with Hardware PWM, HMI, and Emergency Interrupt.
 * @details Implements Phase and Frequency Correct PWM (Timer4) to control a DC motor's speed, 
 * utilizing a matrix keypad for duty cycle input. Includes an asynchronous emergency stop 
 * via external hardware interrupts and state-indication LEDs driven by a CTC timer (Timer5).
 * @author Daniel Ruiz Pérez
 * @date 2024-06-17
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "LCD/lcd.h"

// Pin and Port Definitions
#define MOTOR_PIN_DDR DDRH
#define MOTOR_PIN_PORT PORTH
#define MOTOR_PIN1 PH4 // Direction Control Pin 1
#define MOTOR_PIN2 PH5 // Direction Control Pin 2
#define PWM_PIN PH3    // PWM Signal Pin (OC4A)

#define LEDS_PIN_DDR DDRF
#define LEDS_PIN_PORT PORTF
#define LED_AZUL_PIN PF0
#define LED_ROJO_PIN PF1

#define EMERGENCY_BUTTON_PIN_DDR DDRE
#define EMERGENCY_BUTTON_PIN_PORT PORTE
#define EMERGENCY_BUTTON_PIN PINE
#define EMERGENCY_BUTTON PE5

// Global State Variables
volatile uint8_t motor_direction = 0; // 0: Stopped, 1: CW (Right), 2: CCW (Left)
volatile uint8_t pwm_duty_cycle = 50; // Default 50% duty cycle
volatile uint8_t counter_rojo = 0;
volatile uint8_t counter_azul = 0;
char buffer[3] = {'\0'};
char ciclo[32]; 

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
    
    for (nf = 0; nf <= 3; nf++) {
        PORTA = FILAS[nf];
        _delay_us(100);
        
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
 * @brief Initializes Timer4 in Phase and Frequency Correct PWM mode at 25 kHz.
 */
void timer4_init(void) {
    DDRH |= (1 << PWM_PIN); // Configure PWM pin as output
    
    // Clear OC4A on Compare Match when up-counting, set when down-counting
    TCCR4A = (1 << COM4A1);						
    // Phase and Frequency Correct PWM, ICR4 as TOP, prescaler = 1
    TCCR4B = (1 << WGM43) | (1 << CS40);		
    
    ICR4 = 320;         // TOP value for 25 kHz PWM frequency at 16MHz F_CPU
    OCR4A = ICR4 / 2;   // Initial duty cycle set to 50%
}

/**
 * @brief Initializes Timer5 in Clear Timer on Compare Match (CTC) mode.
 * @details Generates an interrupt every 0.25 seconds for LED blinking logic.
 */
void timer5_init() {
    TCCR5A = 0;											
    TCCR5B = (1 << WGM52) | (1 << CS51) | (1 << CS50);	// Prescaler 64, CTC Mode
    OCR5A = 62499;										// Top value for 0.25s interrupt
    TIMSK5 |= (1 << OCIE5A);							// Enable Compare A Match interrupt
}

/**
 * @brief Initializes the External Interrupt for the Emergency Stop button.
 */
void emergency_button_init() {
    EMERGENCY_BUTTON_PIN_DDR &= ~(1 << EMERGENCY_BUTTON); // Set as input
    EMERGENCY_BUTTON_PIN_PORT |= (1 << EMERGENCY_BUTTON); // Enable pull-up

    // Bug Fix: INT5 is controlled by EICRB, not EICRA. Configure for Rising Edge.
    EICRB |= (1 << ISC51) | (1 << ISC50); 
    EIMSK |= (1 << INT5); // Enable INT5 interrupt
}

/**
 * @brief Controls the H-Bridge direction logic.
 * @param direction 1 for CW, 2 for CCW, 0 for Stop.
 */
void motor_control(uint8_t direction) {
    if (direction == 1) { // CW
        MOTOR_PIN_PORT |= (1 << MOTOR_PIN1);
        MOTOR_PIN_PORT &= ~(1 << MOTOR_PIN2);
    } else if (direction == 2) { // CCW
        MOTOR_PIN_PORT |= (1 << MOTOR_PIN2);
        MOTOR_PIN_PORT &= ~(1 << MOTOR_PIN1);
    } else { // Stop
        MOTOR_PIN_PORT &= ~(1 << MOTOR_PIN1);
        MOTOR_PIN_PORT &= ~(1 << MOTOR_PIN2);
    }
}

/**
 * @brief Handles LCD UI formatting based on current motor state and duty cycle.
 */
void mensajes(){
    uint8_t d1 = (pwm_duty_cycle / 10);
    uint8_t d2 = (pwm_duty_cycle % 10);
    
    sprintf(ciclo, "%d%d", d1, d2);
    
    switch (motor_direction){
        case 1:
            lcd_clrscr();
            lcd_gotoxy(0,0);
            lcd_puts("       MOTOR GIRANDO");
            lcd_gotoxy(0,1);
            lcd_puts("     SENTIDO CW");
            lcd_gotoxy(0,3);
            lcd_puts("DUTY AL :  ");
            lcd_gotoxy(11,3);
            lcd_puts(ciclo);
            lcd_gotoxy(15,3);
            lcd_puts("%");
            break;
        case 2:
            lcd_clrscr();
            lcd_gotoxy(0,0);
            lcd_puts("       MOTOR GIRANDO");
            lcd_gotoxy(0,1);
            lcd_puts("     SENTIDO CCW");
            lcd_gotoxy(0,3);
            lcd_puts("DUTY AL :  ");
            lcd_gotoxy(11,3);
            lcd_puts(ciclo);
            lcd_gotoxy(15,3);
            lcd_puts("%");
            break;
        default:
            lcd_clrscr();
            lcd_gotoxy(0,0);
            lcd_puts(" INTRODUZCA UN DUTY");
            lcd_gotoxy(0,1);
            lcd_puts("  VALORES DEL 0-99  ");
            lcd_gotoxy(9,2);
            lcd_puts(ciclo);
            lcd_gotoxy(12,2);
            lcd_puts("%");
            break;
    }
}

/**
 * @brief Parses keypad input to update the PWM duty cycle ring buffer.
 */
void actualizar_buffer() {
    char tecla = keypad_read();
    
    if(motor_direction == 0){ // Only allow duty cycle changes when stopped
        if (tecla >= '0' && tecla <= '9') {
            if (strlen(buffer) == 2) {
                // Shift buffer left
                for (size_t i = 0; i < strlen(buffer) - 1; i++) {
                    buffer[i] = buffer[i + 1];
                }
                buffer[strlen(buffer) - 1] = tecla;
            } else {
                buffer[strlen(buffer)] = tecla;
            }
            pwm_duty_cycle = atoi(buffer);
        }
    }
}

/**
 * @brief Evaluates directional commands and updates the PWM Output Compare Register.
 */
void motor(){
    char key = keypad_read();
    if (key != '\0') {
        switch (key) {
            case 'A': motor_direction = 1; break; // CW
            case 'B': motor_direction = 2; break; // CCW
        }
    }

    motor_control(motor_direction);
    // Cast to uint32_t to safely prevent 16-bit integer overflow during calculation
    OCR4A = (uint16_t)(((uint32_t)ICR4 * pwm_duty_cycle) / 100); 
}

int main(void) {
    // Hardware Initialization
    timer4_init();
    timer5_init();
    emergency_button_init();
    lcd_init(LCD_DISP_ON);
    lcd_clrscr();
    
    LEDS_PIN_DDR |= (1 << LED_AZUL_PIN) | (1 << LED_ROJO_PIN);
    MOTOR_PIN_DDR |= (1 << MOTOR_PIN1) | (1 << MOTOR_PIN2);
    DDRA = 0x0F; // Keypad rows as output
    PORTA = 0xFF; // Keypad columns internal pull-ups
    
    sei(); // Enable global interrupts
    
    while (1) {
        mensajes();
        actualizar_buffer();
        motor();		
        _delay_ms(50); // Debounce and refresh delay
    }
}

// ---------------- Interrupt Service Routines (ISRs) ----------------

/**
 * @brief Timer 5 ISR - Controls LED blinking logic based on motor direction.
 */
ISR(TIMER5_COMPA_vect) {
    if (motor_direction == 1) { // CW
        counter_azul++;
        if(counter_azul == 3){ // Blink at slower rate
            PORTF ^= (1<<LED_AZUL_PIN);
            counter_azul = 0;
        }
    } else if (motor_direction == 2) { // CCW
        counter_rojo++;
        if (counter_rojo == 1) { // Blink at faster rate
            PORTF ^= (1<<LED_ROJO_PIN);
            counter_rojo = 0;
        }
    } else {
        // Bug Fix: Group pins before negating to correctly clear both bits
        PORTF &= ~((1<<LED_AZUL_PIN) | (1<<LED_ROJO_PIN));
    }
}

/**
 * @brief External Interrupt 5 ISR - Hardware-level emergency stop.
 */
ISR(INT5_vect) {
    motor_direction = 0; // Force logic stop
    // Bug Fix: Group pins before negating
    PORTF &= ~((1<<LED_AZUL_PIN) | (1<<LED_ROJO_PIN));
}