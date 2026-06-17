# LED Matrix Game Engine and Hardware Multiplexing

## Project Overview
This project involves the development of a complete embedded software architecture to run a classic Snake game on an ATmega2560 microcontroller. It demonstrates the ability to combine application-level logic (coordinate tracking, collision detection, and array management) with low-level hardware control (time-division multiplexing for an 8x8 LED matrix and LCD interfacing).

## Hardware Architecture & Configuration
* Microcontroller: ATmega2560 @ 16MHz
* LED Matrix Output: PORTF configured as Row sinks (Active Low), PORTK configured as Column sources (Active High).
* User Interface: Alphanumeric LCD mapped to PORTB for score tracking and game state visualization.
* Input Control: Physical push-buttons mapped directly to hardware interrupt vectors (`INT0`, `INT1`, `INT2`, `INT4`, `INT5`) to ensure zero input lag during gameplay.

## Key Technical Features
1. Matrix Multiplexing Algorithm: Implementation of a custom display driver utilizing a nested loop structure and precise microsecond delays to achieve persistence of vision (POV) across 64 individual LEDs.
2. Hardware Interrupts for Controls: User inputs are handled strictly via asynchronous External Interrupts (`ISR(INTx_vect)`), preventing missed inputs that typically occur in blocking/polling architectures.
3. Timer-Driven Refresh Rate: Utilization of Timer0 Overflow (`TIMER0_OVF_vect`) to create a consistent, non-blocking timebase for refreshing the physical displays independently of the game logic execution speed.
4. Logical State Management: Use of a 2D array (`matriz[8][8]`) as a memory buffer to decouple the game logic (coordinate updates) from the hardware rendering routine.
