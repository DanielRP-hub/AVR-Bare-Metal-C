# Calendar Clock with Matrix Keypad Driver

## Project Overview
This project is an advanced implementation of a calendar system built from the ground up for AVR microcontrollers. It features an asynchronous timebase driven by hardware timer interrupts, a dynamic calendar logic supporting leap years, and a fully functional 4x4 matrix keypad driver for user input.

This repository demonstrates the ability to manage complex state machines, write peripheral drivers, and utilize hardware timers to prevent CPU blocking.

## Hardware Architecture & Configuration
* **Microcontroller:** AVR (e.g., ATmega2560) @ 16MHz
* **Timebase (Timer0):** `TIMER0` configured with a 256 prescaler. The overflow interrupt (`TIMER0_OVF_vect`) triggers a counter to accurately track time in the background.
* **Matrix Keypad (PORTC):** Custom 4x4 keypad driver. The lower nibble configures rows as outputs, and the upper nibble configures columns as inputs with internal pull-ups enabled (`DDRC = 0x0F`, `PORTC = 0xFF`).
* **Display Output:** Interfacing with an alphanumeric LCD using standard C string parsing (`sprintf`).

## Key Technical Features
1. **Matrix Scanning Algorithm:** Implementation of a sweeping logic-zero algorithm to scan keypad rows and read columns. Includes software debouncing to prevent multi-triggering.
2. **Interrupt-Driven Timekeeping:** The core clock logic (`reloj()`) is tied to the hardware timer overflow, ensuring that the time remains accurate even while the CPU is busy scanning the keypad or updating the LCD.
3. **Dynamic Data Parsing:** Implementation of a ring-buffer logic that captures ASCII input from the keypad, shifts the data, and converts it to integer values (`atoi`) to dynamically update time/date variables.
4. **State-Machine UI:** An advanced user interface structure utilizing a `switch-case` state machine to isolate editable parameters (Day, Month, Year, Hour, Min) and provide visual feedback (digit blinking) without disrupting the main timekeeping background task.
