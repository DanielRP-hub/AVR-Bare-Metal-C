# Multi-Sensor Central Controller Node

## Project Overview
This project constitutes a comprehensive central control node running on an ATmega2560. It functions as a complex systems integrator, bridging multiple communication protocols (I2C, SPI, USART) and on-chip hardware features (ADC, Fast PWM, Phase Correct PWM) into a single, unified firmware architecture. The system processes external USART string commands dynamically to poll environmental data or actuate physical outputs.

## Hardware Architecture & Supported Peripherals
* Microcontroller: ATmega2560 @ 16MHz
* Communication Protocols Handled:
  * USART0 (9600 Baud): Main Command Line Interface (CLI) input and telemetry output.
  * USART3 (9600 Baud): Dedicated serial line for NMEA sentence parsing from a GPS module.
  * I2C (400 kHz Fast Mode): Interfacing with MPU6050 (6-axis IMU), BH1750 (Lux meter), and MCP4725 (12-bit DAC).
  * SPI (Mode 0): Synchronous data extraction from a MAX6675 Thermocouple digitizer.
* On-Chip Actuation & Sensing:
  * ADC (Channel 0): Analog telemetry for LM35 ambient temperature sensor.
  * Timer1 (Fast PWM - Mode 14): Provides a 50 kHz hardware pulse-width modulation output for driving an external DC Motor H-Bridge.
  * Timer0 (Phase Correct PWM - Mode 5): Generates an exact 50Hz frequency base to dictate precise angular positions for a servo motor.
  * Timer4: Used as a high-speed microsecond counter for calculating HC-SR04 ultrasonic echo pulse widths.

## Key Technical Features
1. Unified Command Parser: Implementation of an asynchronous USART interrupt buffer (`USART0_RX_vect`) that captures incoming string vectors, filters `\r\n` carriage returns, and executes specific subroutines based on string comparisons (`strcmp` / `strncmp`).
2. Hardware Locking Prevention: The SPI transaction functions are unified into a strict polling mechanism (`spi_transfer`) that explicitly prevents infinite while-loop hangs caused by hardware flag mismatching.
3. Safe Bitwise DAC Transactions: Transmission logic to the MCP4725 explicitly masks the MSB and LSB using `(MCP >> 8) & 0x0F` to adhere strictly to the datasheet requirements for I2C Fast Mode transfers.
4. NMEA GPS Parsing: The `USART3_RX_vect` acts as an asynchronous state machine, searching specifically for the `$GPGGA` string header to extract localization data without blocking the main telemetry operations of the microcontroller.
