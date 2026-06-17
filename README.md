# AVR Bare-Metal C Configurations 

## Overview
This repository contains low-level C configurations and drivers for AVR microcontrollers. The primary focus of these projects is direct register manipulation (bare-metal programming) without relying on high-level hardware abstraction libraries. This approach ensures deterministic behavior, optimized memory footprint, and a deep understanding of the microcontroller's architecture.

## Hardware Used
* **Microcontroller: ATmega2560** 
* **Components: LEDs, Push buttons, GPS NEO6, DC-motor, matrix LED** 
* **Toolchain/IDE: Microchip Studio** 

## Peripheral Configurations Included
* **GPIO Management:** Direct configuration of Data Direction Registers (`DDRx`), Port Data Registers (`PORTx`), and Input Pins (`PINx`).
* **Analog-to-Digital Converter (ADC):** Configuration of `ADMUX` and `ADCSRA` registers for precise sensor readings.
* **Timers and Interrupts:** Setup of Control Registers (`TCCRxA`, `TCCRxB`) for hardware delays, PWM generation, and Interrupt Service Routines (ISR).
* **USART Communication:** Implementation of serial data transmission and reception configuring the Baud Rate Registers (`UBRRn`), Control and Status Registers (`UCSRnA`, `UCSRnB`, `UCSRnC`), and handling the Data Register (`UDRn`).
* **SPI Protocol:** High-speed synchronous communication setup manipulating the SPI Control Register (`SPCR`), Status Register (`SPSR`), and Data Register (`SPDR`) to interface with external peripherals.
* **I2C / TWI (Two-Wire Interface):** Configuration of the Two-Wire Interface using the Bit Rate Register (`TWBR`), Control Register (`TWCR`), and Status Register (`TWSR`) for master/slave communication with sensors and EEPROMs.

## Technical Highlights
* **Register-Level C Programming:** Developed entirely in C, utilizing bitwise operations to configure specific hardware flags.
* **Hardware Interfacing:** Successfully isolated control signals using optocouplers and handled bouncing issues on physical inputs.

## How to Compile and Run
1. Clone this repository: `git clone https://github.com/YourUsername/YourRepoName.git`
2. Open the project in your preferred AVR IDE.
3. Build the solution and flash the `.hex` file directly to the microcontroller via your programmer.
