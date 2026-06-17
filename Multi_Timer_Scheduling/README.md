# Multi-Timer Asynchronous Scheduling

## Project Overview
This project demonstrates advanced hardware timing management on the ATmega2560 microcontroller. It utilizes four independent 16-bit hardware timers (Timer 1, 3, 4, and 5) to generate highly accurate, asynchronous strobe pulses at varying intervals (1, 2, 3, and 4 seconds). The system operates entirely independently of the main execution loop, showcasing real-time scheduling capabilities critical for automotive embedded systems.

## Hardware Architecture & Pin Mapping
* Microcontroller: ATmega2560 @ 16MHz
* Timer 1 Output: PORTH, Pin 0 (1-second interval)
* Timer 3 Output: PORTL, Pin 2 (2-second interval)
* Timer 4 Output: PORTC, Pin 6 (3-second interval)
* Timer 5 Output: PORTF, Pin 5 (4-second interval)

## Mathematical Timebase Configuration
To achieve exactly a 1-second delay before an overflow interrupt occurs, the 16-bit timer counter registers (`TCNTx`) must be pre-loaded with a specific value. 

The ATmega2560 runs at 16 MHz. With the timer control registers (`TCCRxB`) set to a prescaler of 256, the frequency of the timer clock is $16,000,000 / 256 = 62,500$ ticks per second.

Since a 16-bit timer overflows at 65,536, the required pre-load value is calculated as follows:
$$TCNT = 65536 - \left( \frac{T_{delay} \cdot F_{CPU}}{Prescaler} \right)$$
$$TCNT = 65536 - \left( \frac{1 \cdot 16000000}{256} \right) = 65536 - 62500 = 3036$$

This pre-load value of `3036` is immediately re-injected at the very start of each Interrupt Service Routine (ISR) to prevent cycle drift caused by the interrupt latency.

## Technical Execution
* Software Prescaling: Timers 3, 4, and 5 use the hardware 1-second overflow as a base. Volatile state variables increment inside the ISRs to logically extend the overflow limit to 2, 3, and 4 seconds respectively, optimizing the 16-bit hardware limitations.
* Pin Manipulation: Actuation is executed strictly via bitwise exclusive-OR (XOR) toggling and AND complement clearing, ensuring neighboring pins on the same port register remain unaffected.
