# Synchronized Multi-Channel Fast PWM for RGB Control

## Project Overview
This project demonstrates the generation of multiple synchronized hardware PWM signals using a single timer module on the ATmega2560. It effectively controls the independent intensity of three color channels (Red, Green, Blue) of an LED. User interaction is handled via asynchronous hardware interrupts to dynamically calculate and update the duty cycle of each channel.

## Hardware Architecture & Configuration
* Microcontroller: ATmega2560 @ 16MHz
* PWM Core: Utilizes `TIMER4` configured in Fast PWM (Mode 14).
* Frequency Base: The `ICR4` register acts as the TOP limit, pre-configured to generate a stable 5 kHz switching frequency using a prescaler of 64.
* Output Channels: 
  * Channel A (`OC4A` / PH3) -> Blue intensity.
  * Channel B (`OC4B` / PH4) -> Green intensity.
  * Channel C (`OC4C` / PH5) -> Red intensity.
* Input Control: External buttons wired to hardware interrupt vectors (`INT0`, `INT1`, `INT3`) with internal pull-ups enabled, configured for rising edge detection.

## Key Technical Features
1. Multi-Channel Synchronization: By leveraging three Output Compare Registers (`OCR4A`, `OCR4B`, `OCR4C`) driven by the same base counter (`TCNT4`), the system ensures that the PWM pulses across all three outputs are perfectly phase-aligned, preventing flicker or heterodyning effects in the combined light output.
2. Safe Register Arithmetic: Dynamic duty cycle calculations inside the ISRs implement strict 16-bit casting `(uint16_t)` to prevent integer overflow errors before assigning values back to the hardware registers.
3. Asynchronous Parameter Updating: The Output Compare Registers are updated inside the external interrupts, meaning the physical light intensity responds instantaneously to user input, entirely decoupling the hardware response time from the LCD UI refresh rate.
