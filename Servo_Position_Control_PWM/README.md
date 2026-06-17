# Servo Motor Position Control via Hardware PWM

## Project Overview
This project demonstrates precise closed-loop angular actuation of a standard servo motor using the ATmega2560. A custom Human-Machine Interface (HMI) allows users to input target angles (0-180 degrees) via a matrix keypad, while the microcontroller calculates the required duty cycle and drives the motor using Phase and Frequency Correct hardware PWM.

## Hardware Architecture & Configuration
* Microcontroller: ATmega2560 @ 16MHz
* PWM Core: Utilizes `TIMER5` configured in Mode 9 (Phase and Frequency Correct PWM). 
* Actuator Output: Mapped to `OC5B` (PORTL, Pin 4).
* User Interface: 4x4 Matrix Keypad on PORTA and an alphanumeric LCD.

## Mathematical PWM Timebase
Standard servo motors require a strict 50Hz (20ms) control signal, where the pulse width determines the mechanical angle (typically ~1ms for 0° and ~2ms for 180°). 

By using a prescaler ($N$) of 8 and setting the TOP value in `OCR5A` to 20,000, the 50Hz frequency is perfectly synthesized from the 16MHz base clock:
$$F_{PWM} = \frac{F_{CPU}}{2 \cdot N \cdot TOP} = \frac{16000000}{2 \cdot 8 \cdot 20000} = 50 \text{ Hz}$$

The dynamic pulse width is mapped directly to the user input angle via linear scaling and written to the `OCR5B` register:
* Base Offset (0°): `OCR5B` = 800 (yields a 0.8ms pulse).
* Scaling Factor: +8 per degree.
* Max Actuation (180°): `OCR5B` = 2240 (yields a 2.24ms pulse).
