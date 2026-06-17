# Interrupt-Driven Clock with LCD Interfacing

## Project Overview
This project showcases an asynchronous, event-driven architecture using an AVR ATmega2560 microcontroller. It functions as a digital clock that outputs formatted data to an alphanumeric LCD display while relying on **Hardware Interrupts (ISRs)** to handle user inputs (buttons) independently of the main CPU loop.

This repository demonstrates the transition from blocking/polling methods to a robust, real-time responsive system—a critical skill for automotive embedded software development.

## Hardware Architecture & Configuration
* **Microcontroller:** ATmega2560 @ 16MHz
* **Display Output:** Interfacing with a standard character LCD using a custom `lcd.h` driver, connected via PORTB. String formatting is handled dynamically using standard C libraries (`sprintf`).
* **Interrupt Vectors:**
  * `INT0` (PORTD 0): Triggers minute adjustment (Rising Edge).
  * `INT1` (PORTD 1): Triggers hour adjustment (Falling Edge).
  * `INT4` (PORTE 4): Triggers system Stop state.
  * `INT5` (PORTE 5): Triggers system Resume state.

## Key Technical Features
1. **Interrupt Service Routines (ISR):** Implementation of `<avr/interrupt.h>` to handle asynchronous physical button presses instantly via the `EICRA`, `EICRB`, and `EIMSK` control registers.
2. **Volatile Variables:** Correct usage of the `volatile` keyword for state variables (`horas`, `minutos`, `stop`) shared between the main execution context and the ISRs, preventing aggressive compiler optimization.
3. **External Driver Integration:** Successful integration and utilization of an external header file/driver (`LCD/lcd.h`) to abstract hardware specifics, keeping the main application layer clean.
