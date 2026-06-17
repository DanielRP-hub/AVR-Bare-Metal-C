/**
 * @file main.c
 * @brief Full Real-Time Clock (RTC) implementation with matrix keypad configuration.
 * @details This firmware tracks time and date (including leap years) using Timer0 overflow interrupts. 
 * It features a 4x4 matrix keypad driver with debouncing and a state-machine UI for parameter editing.
 * @author Daniel Ruiz Pérez
 * @date 2024-04-09
 */

#define F_CPU 16000000ul
#include <avr/io.h>
#include <util/delay.h>
#include "LCD/lcd.h"
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <string.h>

// Volatile timekeeping variables modified within ISRs and main loop
volatile uint8_t segundos = 0;
volatile uint8_t minutos = 0;
volatile uint8_t horas = 0;
volatile uint16_t year = 2024;
volatile uint8_t meses = 1;
volatile uint8_t dias = 1;
volatile uint8_t stop = 0;
volatile uint8_t mostrar_dig = 1;

// String buffers for LCD formatting
char hora[32], fecha[32];
char dia[32], mes[32], yea[32], hor[32], min[32], sec[32];

// UI State Machine control variables
uint8_t cambio = 0; // Tracks which parameter is currently being edited
uint8_t contador = 0;

// Keypad input buffers
char buffer[3] = {'\0'};
char buffer2[5] = {'\0'}; // Buffer to store the digits entered from the keyboard

// 4x4 Keypad Mapping
const char teclas[4][4] = { {'1', '2', '3', 'A'},
							{'4', '5', '6', 'B'},
							{'7', '8', '9', 'C'},
							{'*', '0', '#', 'D'}};

/**
 * @brief Initializes Timer0 for asynchronous timekeeping.
 * @details Configures a prescaler of 256 and enables the Overflow Interrupt.
 */
void init_timer0() {
	TCCR0A = 0x00;
	TCCR0B |= (1 << CS02); // PRESCALAR 256
	TIMSK0 |= (1 << TOIE0); // Habilitar interrupcion por desbordamiento
	TCNT0 = 0X00;
}

/**
 * @brief Toggles the blinking flag for the UI edit mode.
 */
void toggle_dig() {
	mostrar_dig = !mostrar_dig; // Change the flag state
}


/**
 * @brief UI handling and LCD formatting function.
 * @details Formats the current time/date into strings. If in edit mode ('stop' == 1), 
 * it manages the blinking effect for the currently selected parameter ('cambio').
 */
void escritura() {
	uint8_t h1 = (horas / 10);
	uint8_t h2 = (horas % 10);
	uint8_t m1 = (minutos / 10);
	uint8_t m2 = (minutos % 10);
	uint8_t s1 = (segundos / 10);
	uint8_t s2 = (segundos % 10);
	
	uint8_t d1 = (dias / 10);
	uint8_t d2 = (dias % 10);
	uint8_t me1 = (meses / 10);
	uint8_t me2 = (meses % 10);
	uint8_t a1 = (year / 10);
	uint8_t a2 = (year % 10);
	
	if(stop==1){
		lcd_gotoxy(0, 0);
		sprintf(dia, "%d%d", d1, d2);
		sprintf(mes, "%d%d", me1, me2);
		sprintf(yea, "%d%d", a1, a2);
		sprintf(hor, "%d%d", h1, h2);
		sprintf(min, "%d%d", m1, m2);
		sprintf(sec, "%d%d", s1, s2);
		sprintf(hora, "%d%d : %d%d : %d%d", h1, h2, m1, m2, s1, s2);
		sprintf(fecha, "%d%d / %d%d / %d%d", d1, d2, me1, me2, a1, a2);

		toggle_dig();
		_delay_ms(100);
		lcd_clrscr();
		
		switch (cambio){
			case 0:
			lcd_gotoxy(0, 0);
			sprintf(hora, "%d%d : %d%d : %d%d", h1, h2, m1, m2, s1, s2);
			sprintf(fecha, "%d%d / %d%d / %d%d", d1, d2, me1, me2, a1, a2);
			lcd_clrscr();
			lcd_gotoxy(1, 0);
			lcd_puts(fecha);
			lcd_gotoxy(2, 1);
			lcd_puts(hora);
			break;
			case 1:
			if (mostrar_dig) {
				lcd_gotoxy(1, 0);
				lcd_puts(dia);
				lcd_gotoxy(4,0);
				lcd_puts("/");
				lcd_gotoxy(6, 0);
				lcd_puts(mes);
				lcd_gotoxy(9,0);
				lcd_puts("/");
				lcd_gotoxy(11, 0);
				lcd_puts(yea);
				lcd_gotoxy(2, 1);
				lcd_puts(hora);
				} else {
				lcd_gotoxy(4,0);
				lcd_puts("/");
				lcd_gotoxy(6, 0);
				lcd_puts(mes);
				lcd_gotoxy(9,0);
				lcd_puts("/");
				lcd_gotoxy(11, 0);
				lcd_puts(yea);
				lcd_gotoxy(2, 1);
				lcd_puts(hora);
			}
			break;
			case 2:
			if (mostrar_dig) {
				lcd_gotoxy(1, 0);
				lcd_puts(dia);
				lcd_gotoxy(4,0);
				lcd_puts("/");
				lcd_gotoxy(6, 0);
				lcd_puts(mes);
				lcd_gotoxy(9,0);
				lcd_puts("/");
				lcd_gotoxy(11, 0);
				lcd_puts(yea);
				lcd_gotoxy(2, 1);
				lcd_puts(hora);
				} else {
				lcd_gotoxy(1, 0);
				lcd_puts(dia);
				lcd_gotoxy(4,0);
				lcd_puts("/");
				lcd_gotoxy(9,0);
				lcd_puts("/");
				lcd_gotoxy(11, 0);
				lcd_puts(yea);
				lcd_gotoxy(2, 1);
				lcd_puts(hora);
			}
			break;
			case 3:
			if (mostrar_dig) {
				lcd_gotoxy(1, 0);
				lcd_puts(dia);
				lcd_gotoxy(4,0);
				lcd_puts("/");
				lcd_gotoxy(6, 0);
				lcd_puts(mes);
				lcd_gotoxy(9,0);
				lcd_puts("/");
				lcd_gotoxy(11, 0);
				lcd_puts(yea);
				lcd_gotoxy(2, 1);
				lcd_puts(hora);
				} else {
				lcd_gotoxy(1, 0);
				lcd_puts(dia);
				lcd_gotoxy(4,0);
				lcd_puts("/");
				lcd_gotoxy(6, 0);
				lcd_puts(mes);
				lcd_gotoxy(9,0);
				lcd_puts("/");
				lcd_gotoxy(2, 1);
				lcd_puts(hora);
			}
			break;
			case 4:
			if (mostrar_dig) {
				lcd_gotoxy(1,0);
				lcd_puts(fecha);
				lcd_gotoxy(2,1);
				lcd_puts(hor);
				lcd_gotoxy(5,1);
				lcd_puts(":");
				lcd_gotoxy(7,1);
				lcd_puts(min);
				lcd_gotoxy(10,1);
				lcd_puts(":");
				lcd_gotoxy(12,1);
				lcd_puts(sec);
				}else{
				lcd_gotoxy(1,0);
				lcd_puts(fecha);
				lcd_gotoxy(5,1);
				lcd_puts(":");
				lcd_gotoxy(7,1);
				lcd_puts(min);
				lcd_gotoxy(10,1);
				lcd_puts(":");
				lcd_gotoxy(12,1);
				lcd_puts(sec);
			}
			break;
			case 5:
			if (mostrar_dig) {
				lcd_gotoxy(1,0);
				lcd_puts(fecha);
				lcd_gotoxy(2,1);
				lcd_puts(hor);
				lcd_gotoxy(5,1);
				lcd_puts(":");
				lcd_gotoxy(7,1);
				lcd_puts(min);
				lcd_gotoxy(10,1);
				lcd_puts(":");
				lcd_gotoxy(12,1);
				lcd_puts(sec);
				}else{
				lcd_gotoxy(1,0);
				lcd_puts(fecha);
				lcd_gotoxy(2,1);
				lcd_puts(hor);
				lcd_gotoxy(5,1);
				lcd_puts(":");
				lcd_gotoxy(10,1);
				lcd_puts(":");
				lcd_gotoxy(12,1);
				lcd_puts(sec);
			}
			break;
		}
	}
	else{
		lcd_gotoxy(0, 0);
		sprintf(hora, "%d%d : %d%d : %d%d", h1, h2, m1, m2, s1, s2);
		sprintf(fecha, "%d%d / %d%d / %d%d", d1, d2, me1, me2, a1, a2);
		lcd_clrscr();
		lcd_gotoxy(1, 0);
		lcd_puts(fecha);
		lcd_gotoxy(2, 1);
		lcd_puts(hora);
	}
}

/**
 * @brief Leap year calculation.
 * @return 1 if leap year, 0 otherwise.
 */
int esBisiesto() {
	return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

/**
 * @brief Determines the maximum days in a given month.
 */
int dias_en_mes(uint8_t mes, uint16_t year) {
	switch (mes) {
		case 2: // Febrero
		return esBisiesto(year) ? 29 : 28;		// Retorna 29 si es bisiesto, 28 si no lo es
		case 4: case 6: case 9: case 11: return 30; // Abril, Junio, Septiembre y Noviembre tienen 30 d�as
		default: return 31;							// Los dem�s meses tienen 31 d�as
	}
}

/**
 * @brief Core calendar and clock logic. Increments variables logically.
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
					dias++;
					if (dias > dias_en_mes(meses, year)) {
						dias = 1;
						if (meses == 12) {
							meses = 1;
							year++;
							} else {
							meses++;
						}
					}
				}
			}
		}
	}
}


/**
 * @brief Scans a 4x4 matrix keypad connected to PORTC.
 * @details Sweeps logic zeroes across rows and reads columns with debouncing logic.
 * @return The character pressed, or '\0' if no press is detected.
 */
char leer_teclado() {
	uint8_t filas[4] = {0b11111110, 0b11111101, 0b11111011, 0b11110111}, data;
	
	for (uint8_t i = 0; i < 4; i++) {
        PORTC = filas[i];
        _delay_us(90);
        data = PINC & 0xF0; // Column mask
        
        if (data != 0xF0) { // Check if any column pulled low (Active Low due to pullups)
            _delay_ms(60);  // Debounce delay
            data = PINC & 0xF0;
            
            if (data != 0xF0) {
                switch(data) {
                    case 0xE0: return teclas[i][0];
                    case 0xD0: return teclas[i][1];
                    case 0xB0: return teclas[i][2];
                    case 0x70:
                        // Control logic for A, B, C, D buttons
                        switch(i) {
                            case 0: // A: Toggle Stop
                                if (cambio == 0) { stop = !stop; }
                                break;
                            case 1: // B: Apply constraints & exit edit
                                if(meses > 12) meses = 12;
                                if(dias > dias_en_mes(meses, year)) dias = dias_en_mes(meses, year);
                                if(horas > 23) horas = 23;
                                if(minutos > 59) minutos = 59;
                                cambio = (stop == 1) ? 0 : cambio; 
                                break;
                            case 2: // C: Enter edit mode
                                cambio = (stop == 1) ? 1 : 0; 
                                break;
                            case 3: // D: Navigate edit parameters
                                if (cambio < 5 && stop == 1) { cambio++; } 
                                else { cambio = 1; }
                                break;
                        }
                        return teclas[i][3];
                }
            }
        }
    }
    return '\0';
}

/**
 * @brief Timer0 Overflow ISR. Acts as the system timebase.
 */
ISR(TIMER0_OVF_vect) { 
	contador++;

	// ~248 overflows at 16MHz with 256 prescaler approximates 1 second

	if (contador == 248) {
		reloj();
		contador = 0;
	}
}

/**
 * @brief Parses keypad input and assigns values to the respective time variables.
 */
void actualizar_buffer() {
	char tecla = leer_teclado();
	
	if (tecla >= '0' && tecla <= '9') {
		if (strlen(buffer) == 2) {
			// Si el buffer est� lleno, mover los d�gitos hacia la izquierda para eliminar el m�s significativo
			for (int i = 0; i < strlen(buffer) - 1; i++) {
				buffer[i] = buffer[i + 1];
			}
			// Agregar el nuevo d�gito al final del buffer
			buffer[strlen(buffer) - 1] = tecla;
			} else {
			// Si el buffer no est� lleno, simplemente agregar el nuevo d�gito al final
			buffer[strlen(buffer)] = tecla;
		}
		// Convertir el buffer a un n�mero entero seg�n el cambio actual y asignarlo a la variable correspondiente
		switch (cambio) {
			case 1: dias = atoi(buffer); break;
			case 2: meses = atoi(buffer); break;
			case 3:
			if (strlen(buffer2) == 4) {
				for (int i = 0; i < strlen(buffer2) - 1; i++) {
					buffer2[i] = buffer2[i + 1];
				}
				buffer2[strlen(buffer2) - 1] = tecla;
				} else {
				buffer2[strlen(buffer2)] = tecla;
			}
			year= atoi(buffer2);
			break;
			case 4: horas = atoi(buffer); break;
			case 5: minutos = atoi(buffer); break;
		}
	}
}

int main() {
	DDRC = 0x0F;  // Lower nibble output (rows), upper nibble input (cols)
    PORTC = 0xFF; // Enable internal pull-ups on inputs

	init_timer0();
	lcd_init(LCD_DISP_ON_CURSOR);
	lcd_clrscr();

	sei(); // Enable global interrupts

	while (1) {
		escritura();
		actualizar_buffer();
	}
	return 0;
}