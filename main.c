/**
 * @file main.c
 * @brief Bare-metal digital clock implementation with 6-digit multiplexed 7-segment display.
 * @details This project demonstrates direct register manipulation for GPIO control, 
 * software-based state machines for timekeeping, and manual button debouncing without external libraries.
 * * @author Daniel Ruiz Pérez
 * @date 2024-04-04
 */

#define F_CPU 16000000ul
#include <avr/io.h>
#include <util/delay.h>

volatile uint8_t milisegundos = 0;
volatile uint8_t segundos = 0;
volatile uint8_t minutos = 0;
volatile uint8_t horas = 0;
volatile uint8_t stop = 0;

// Hexadecimal array for 7-segment display digits (0-9)
const uint8_t valores[] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01100111  // 9
};

/**
 * @brief Sends the specific value to a single digit of the multiplexed display.
 * @param digit The digit position to enable (0 to 5).
 * @param value The numerical value to display (0 to 9).
 */
void escritura(uint8_t digit, uint8_t value) {
    PORTF = valores[value];       // Output segment data
    PORTK |= (1 << digit);        // Enable specific digit (common pin)
    _delay_ms(1);                 // Brief delay for display stabilization (refresh rate)
    PORTK &= ~(1 << digit);       // Disable digit to prevent ghosting
}

/**
 * @brief Extracts the tens and units from the time variables and updates the display.
 */
void llamar_escritura() {
    escritura(5, horas / 10);     // Hours tens
    escritura(4, horas % 10);     // Hours units
    escritura(3, minutos / 10);   // Minutes tens
    escritura(2, minutos % 10);   // Minutes units
    escritura(1, segundos / 10);  // Seconds tens
    escritura(0, segundos % 10);  // Seconds units
}

/**
 * @brief Timekeeping logic. Increments counters to simulate a real-time clock.
 */
void reloj() {
    if (!stop) {
        milisegundos++;
        // Assuming 10ms delay loop, 60 iterations = approx 1 second (tuning required)
        if (milisegundos == 60) { 
            milisegundos = 0;
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
    }
    _delay_ms(10); // Base tick delay
}

/**
 * @brief Polls specific input pins to stop or resume the clock. Includes software debouncing.
 */
void detener_reanudar() {
    // Check if Stop condition is met (Pins H5 and H6)
    if ((PINH & (1 << 5)) && (PINH & (1 << 6))) {
        _delay_ms(1); // Debounce delay
        // Wait for button release while maintaining display multiplexing
        while ((PINH & (1 << 5)) && (PINH & (1 << 6))) {
            llamar_escritura();
        }
        stop = 1;
    }

    // Check if Resume condition is met
    if (!(PINH & (1 << 5)) && !(PINH & (1 << 6))) {
        _delay_ms(1); // Debounce delay
        while (!(PINH & (1 << 5)) && !(PINH & (1 << 6))) {
             llamar_escritura();
        }
        stop = 0;
    }
}

/**
 * @brief Allows manual adjustment of hours and minutes when the clock is stopped.
 */
void aumentar_H_M() {
    if (stop == 1) {
        // Increment Minutes (Pins H3 and H4 low)
        if (!(PINH & (1 << 3)) && !(PINH & (1 << 4))) {
            _delay_ms(1); // Debounce
            while (!(PINH & (1 << 3)) && !(PINH & (1 << 4))) {
                llamar_escritura(); 
            }
            minutos++;
            if (minutos == 60) {
                minutos = 0;
            }
        }

        // Increment Hours (Pins H3 and H4 high)
        if ((PINH & (1 << 3)) && (PINH & (1 << 4))) {
            _delay_ms(1); // Debounce
            while ((PINH & (1 << 3)) && (PINH & (1 << 4))) {
                llamar_escritura(); 
            }
            horas++;
            if (horas == 24) {
                horas = 0;
            }
        }
    }
}

int main() {
    // 1. Hardware Initialization
    DDRF = 0xFF; // Set PORTF as Output (7-Segment data lines)
    DDRK = 0xFF; // Set PORTK as Output (Digit selectors for multiplexing)
    
    // Set specific PORTH pins as Inputs (Bits 3, 4, 5, 6)
    DDRH &= ~((1 << 3) | (1 << 4) | (1 << 5) | (1 << 6));

    // 2. Main Super Loop
    while (1) {
        reloj();             // Update time counters
        llamar_escritura();  // Refresh display
        detener_reanudar();  // Check stop/resume buttons
        aumentar_H_M();      // Check time adjustment buttons
    }

    return 0;
}