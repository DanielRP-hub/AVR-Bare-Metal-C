# Bare-Metal Digital Clock with 7-Segment Multiplexing

## Project Overview
This project is a firmware implementation of a real-time digital clock (HH:MM:SS) developed entirely in C for the AVR microcontroller architecture. It features a 6-digit multiplexed 7-segment display driver built from scratch without the use of third-party HAL (Hardware Abstraction Layer) libraries. 

This project demonstrates strong foundational knowledge in **embedded C, bitwise operations, GPIO register manipulation, and software-based debouncing.**

## Hardware Architecture & Pin Mapping
* **Microcontroller:** AVR (e.g., ATmega2560) @ 16MHz
* **PORTF (`DDRF` / `PORTF`):** Configured as an 8-bit output to drive the individual segments (A-G) of the display.
* **PORTK (`DDRK` / `PORTK`):** Configured as an output to act as the multiplexing selector. It sequentially enables each of the 6 digits fast enough to exploit the human eye's persistence of vision (POV).
* **PORTH (`DDRH` / `PINH`):** Configured with specific pins as inputs (Pins 3, 4, 5, and 6) to read external push-buttons for Clock Stop/Resume and Time Adjustment logic.

## Key Technical Features
1. **Direct Register Access:** Hardware configuration is handled exclusively via Bitwise operators (AND, OR, NOT) on Data Direction Registers (`DDRx`).
2. **Display Multiplexing Algorithm:** Custom implementation of a rapid switching routine `escritura()` that isolates digits and prevents ghosting effects.
3. **State Machine & Polling:** Continuous polling of input pins in the main super-loop to update the system state.
4. **Software Debouncing:** Implementation of delay-based debouncing logic integrated with `while()` loops that continue to refresh the display, ensuring the multiplexing is not interrupted while a button is held down.
