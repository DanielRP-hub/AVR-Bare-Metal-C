# DC Motor Control with Hardware PWM and Emergency Interrupts

## Project Overview
This project implements a complete closed-loop control interface for a DC Motor using an ATmega2560. It leverages Phase and Frequency Correct hardware PWM for precise speed regulation and integrates a matrix keypad/LCD Human-Machine Interface (HMI) for real-time parameter configuration. Critical safety is ensured via an asynchronous hardware interrupt acting as an immediate emergency stop.

## Hardware Architecture & Configuration
* Microcontroller: ATmega2560 @ 16MHz
* Motor Control (Timer4): Configured in Phase and Frequency Correct PWM Mode (WGM 8). The `ICR4` register defines a TOP value that establishes a precise 25 kHz switching frequency, ideal for driving an external H-Bridge (e.g., L298N) without audible acoustic noise. Output is mapped to OC4A (PH3).
* Safety Interrupt (INT5): An external push-button is wired to PE5. It utilizes the `EICRB` register configured for Rising Edge detection, allowing an immediate override of the motor state regardless of the main super-loop execution.
* Status Indicators (Timer5): A Clear Timer on Compare Match (CTC) interrupt generates an accurate 0.25s timebase to toggle directional LEDs (Blue for CW, Red for CCW) at distinct blinking rates.

## Key Technical Features
1. Register-Level PWM Manipulation: Speed regulation is achieved without blocking delays by dynamically calculating and updating the Output Compare Register (`OCR4A`) based on a percentage of the fixed `ICR4` TOP limit.
2. Bitwise Bug Prevention: Implementation of 32-bit cast arithmetic during PWM duty cycle calculations `(uint32_t)ICR4 * pwm_duty_cycle` to strictly prevent 16-bit integer overflow limits within the AVR-GCC compiler.
3. Multi-Peripheral Integration: Simultaneous, non-blocking management of matrix scanning, LCD string formatting, hardware PWM generation, and asynchronous interrupt handling.
