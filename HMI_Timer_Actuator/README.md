# Human-Machine Interface (HMI) Timer with Hardware Actuation

## Project Overview
This project showcases the development of a complete Human-Machine Interface (HMI) system on an ATmega2560 microcontroller. It integrates multiple physical peripherals—a 4x4 matrix keypad, an alphanumeric LCD, and an active buzzer—managed by a cohesive state-machine logic. The core timekeeping is handled asynchronously via 16-bit hardware timer interrupts, ensuring accurate countdowns regardless of UI blocking functions.

## Hardware Architecture & Pin Mapping
* Microcontroller: ATmega2560 @ 16MHz
* Matrix Keypad (Input): Mapped to PORTA. The lower nibble (Pins 0-3) drives the rows as outputs, while the upper nibble (Pins 4-7) reads the columns as inputs with internal pull-ups enabled.
* LCD (Output): Mapped via standard character LCD driver for user state visualization.
* Actuator (Output): Mapped to PORTF, Pin 7, controlling an audible buzzer alert upon timer completion.

## Key Technical Features
1. Data Type Optimization: Timekeeping variables are strictly typed as signed integers (`int8_t`) to safely manage subtraction logic and prevent unsigned underflow errors during zero-crossing calculations.
2. Custom Matrix Driver: Implementation of a row-sweeping algorithm with bitwise masking (`PINA & 0xF0`) to accurately decode user inputs without relying on external libraries.
3. Hardware Timebase: Utilization of `TIMER1_OVF_vect` to generate an exact 1Hz tick rate. The 16-bit `TCNT1` register is loaded with a calculated offset (3035) to compensate for the 256 prescaler at 16MHz.
4. Input Validation & Flow Control: The software includes bounded constraints to prevent users from entering invalid time formats (e.g., values > 59) and features "wait-for-release" blocking loops (`while(data != '\0')`) to handle physical switch bouncing and prevent continuous triggering.
