/**
 * @file main.c
 * @brief LED Matrix Snake Game implementation on AVR architecture.
 * @details This firmware combines game engine logic (coordinate tracking, collision detection, 
 * random generation) with low-level hardware control (8x8 LED matrix multiplexing and LCD interfacing).
 * External interrupts handle user directional inputs asynchronously.
 * @author Daniel Ruiz Pérez
 * @date 2024-04-17
 */

#define F_CPU 16000000UL 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "LCD/lcd.h"
#include <stdio.h>
#include <stdlib.h>

// Game state variables
volatile uint8_t start = 0;
volatile uint8_t snakeX = 8;
volatile uint8_t snakeY = 0;
volatile uint8_t direccion = 0;
volatile uint8_t comidaX = 0;
volatile uint8_t comidaY = 0;
volatile uint8_t score = 0;
volatile uint8_t tiempo = 0;

// LED Matrix memory map (8x8)
volatile uint8_t matriz[8][8];

char data[32];

/**
 * @brief Initializes hardware ports, external interrupts, timer, and LCD.
 */
void init(){
	// Matrix Port Configuration (F = Rows, K = Columns)
    DDRF = 0xFF;
    DDRK = 0xFF;
    
    // Interrupt Port Configuration (Inputs)
    DDRD &= ~((1 << 0) | (1 << 1) | (1 << 2));  
    DDRE &= ~((1 << 4) | (1 << 5));  
    
    // Enable Internal Pull-Ups
    PORTD |= ((1 << 0) | (1 << 1) | (1 << 2));			
    PORTE |= ((1 << 4) | (1 << 5));
    
    // Timer 0 Configuration (Normal mode, Prescaler 64)
    TCCR0A = 0; 
    TCCR0B = (1 << CS01) | (1 << CS00); 
    TIMSK0 = (1 << TOIE0); // Enable Timer 0 overflow interrupt
    
    // External Interrupt Configuration (Edge triggering)
    EICRA |= ((1 << ISC01) | (1 << ISC00) | (1<<ISC11) | (1<<ISC10) | (1<<ISC21) | (1<<ISC20));
    EICRB |= ((1 << ISC41) | (1 << ISC40)); 
    EIMSK |= ((1 << INT0) | (1 << INT1) | (1 << INT2) | (1 << INT4) | (1 << INT5)); 
    
    sei(); // Enable global interrupts
		
    // Initialize LCD
    lcd_init(LCD_DISP_ON_CURSOR);
    lcd_clrscr();
    
    // Seed random number generator for food placement
    srand((unsigned int)(NULL));
}

/**
 * @brief Time-division multiplexing routine to render the matrix array to the physical LEDs.
 * Implemented here for localized display refresh synchronization.
 */
void actualizarDisplay() {
    for (uint8_t fila = 0; fila < 8; fila++) {
        PORTF = ~(1 << fila); // Activate row (Active Low)
        for (uint8_t columna = 0; columna < 8; columna++) {
            if (matriz[fila][columna] == 1) {
                PORTK |= (1 << columna); // Turn on LED (Active High)
            } else {
                PORTK &= ~(1 << columna); // Turn off LED
            }
        }
        _delay_us(850); // Persistence of vision delay
        PORTF = (1 << fila); // Deactivate row
    }
	
    // Update LCD based on game state
	if(start==0){
		lcd_gotoxy(3,0);
		lcd_puts("SNAKE GAME");
		lcd_gotoxy(1,1);
		lcd_puts("PRESIONE START");
	}else{
		lcd_clrscr();
		sprintf(data, "%d",score);
		lcd_gotoxy(3,0);
		lcd_puts("SNAKE GAME");
		lcd_gotoxy(3,1);
		lcd_puts("SCORE = ");
		lcd_gotoxy(11,1);
		lcd_puts(data);
	}
}

/**
 * @brief Clears the matrix and updates coordinate points for the snake and food.
 */
void actualizarMatriz() {
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			matriz[i][j] = 0;
		}
	}
	
	// Set the position of the snake in the array
	matriz[snakeY][snakeX] = 1; 
	
	// Set the position of the food in the array
	matriz[comidaY][comidaX] = 1; 
}

/**
 * @brief Generates random X/Y coordinates for the food target.
 */
void generarComida() {
	comidaX = rand() % 8; 
	comidaY = rand() % 8; 
}

/**
 * @brief Core game logic. Updates physical position based on current direction.
 */
void updateSnakePosition() {
	switch (direccion) {
		case 0:  
			if(snakeX>0){
				_delay_ms(1);
				PORTF = ~(1 << (snakeY));
				PORTK = (1<<(snakeX-1)) | (1 << (comidaX));
				snakeX = snakeX-1;
			}
			break;
		case 1:  
			if(snakeY<8){
				_delay_ms(1);
				PORTK = (1<<(snakeX-1)) | (1 << (comidaX));
				PORTF = ~(1<<snakeY);
				snakeY = snakeY+1;
			}
			break;
		case 2: 
			if(snakeY>0){
				_delay_ms(1);
				PORTK = (1<<(snakeX-1)) | (1 << (comidaX));
				PORTF = ~(1<<(snakeY-1));
				snakeY = snakeY-1;
			}
			break;
		case 3:  
			if(snakeX<8){
				_delay_ms(1);
				PORTF = ~(1 << (snakeY));
				PORTK = (1<<(snakeX-1)) | (1 << (comidaX));
				snakeX = snakeX+1;
			}
			break;
	}
	actualizarMatriz(); 
}

/**
 * @brief Collision detection between the snake and the food target.
 */
void ya_comi(){
	if ((snakeX == comidaX) && (snakeY == comidaY)){
		score++;
		generarComida();
	}
}

int main(void)
{
	init();
	while (1) {
		if(start){
			updateSnakePosition();
			ya_comi();
			//_delay_ms(100);
		}
		_delay_ms(100);
	}
}

// ---------------- Interrupt Service Routines (ISRs) ----------------

ISR(INT0_vect){ // Move UP
	direccion = (direccion!=2) ? (direccion = 1) : (direccion = 2);
}

ISR(INT1_vect){// Move Right
	direccion = (direccion!=3) ? (direccion = 0) : (direccion = 3);
}

ISR(INT2_vect){// Move Down
	direccion = (direccion!=1) ? (direccion = 2) : (direccion = 1);
}

ISR(INT4_vect){// Move Left
	direccion = (direccion!=0) ? (direccion = 3) : (direccion = 0);
}
ISR(INT5_vect){//Start/Pause Toggle
	start = !start;
	if (score == 0){
		generarComida();
	}
}

// Timer 0 ISR for Display Refresh
ISR(TIMER0_OVF_vect) {
	tiempo++;
	if (tiempo==150){
		actualizarDisplay(); 
		tiempo=0;
	}
}
