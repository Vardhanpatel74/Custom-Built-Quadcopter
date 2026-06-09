# Custom Built Quadcopter

## Overview

This project is a custom-built quadcopter designed and developed from scratch using Arduino microcontrollers, nRF24L01 wireless communication modules, Electronic Speed Controllers (ESCs), and BLDC motors. The system includes a self-designed transmitter and receiver for wireless control and demonstrates the fundamentals of drone electronics, embedded systems, wireless communication, and flight control.

The project was developed as part of hands-on learning in embedded systems, drone technology, and control systems.

---

## Features

* Custom-built quadcopter platform
* Self-designed RF transmitter and receiver
* Wireless communication using nRF24L01 modules
* 6-channel control architecture
* ESC calibration functionality
* Motor arming and disarming mechanism
* Real-time PWM signal generation
* Modular and expandable design
* Low-cost prototype using readily available components

---

## Hardware Components

### Flight System

* Arduino Uno
* 4 × 30A ESC
* 4 × 1000KV BLDC Motors
* MPU 6050 IMU
* LiPo Battery
* Quadcopter Frame
* Propellers

### Transmitter

* Arduino Nano
* nRF24L01 Module
* Joystick Modules
* Potentiometers
* Toggle Switches
* Power Supply Module

### Receiver

* Arduino Nano
* nRF24L01 Module
* PWM Output Channels

---

## Software and Tools

* Arduino IDE
* Embedded C/C++
* EasyEDA
* Git & GitHub

---

## System Architecture

Transmitter → nRF24L01 Wireless Link → Receiver → Arduino Uno → ESCs → BLDC Motors

The transmitter captures joystick and switch inputs and sends control commands wirelessly through the nRF24L01 module. The receiver decodes these commands and generates PWM signals for the flight controller, which controls the ESCs and motors.

---

## Project Structure

```text
Custom-Built-Quadcopter/
│
├── Code/
│   ├── Transmitter/
│   ├── Receiver/
│   └── Flight_Controller/
│
├── Diagrams/
│   ├── Circuit_Diagrams/
│   ├── Wiring_Diagrams/
│
├── Report/
│   └── Project_Report.pdf
│
└── README.md
```

## Working Principle

1. Joystick inputs are read by the transmitter.
2. Control signals are transmitted using nRF24L01 modules.
3. The receiver receives and decodes the commands.
4. PWM signals are generated for motor control.
5. ESCs regulate motor speed.
6. BLDC motors produce thrust for flight.

---

## Applications

* Educational drone development
* Embedded systems learning
* Wireless communication experiments
* Flight control research
* UAV prototyping

---

## Results

* Successful wireless communication between transmitter and receiver
* Stable PWM signal generation
* Reliable ESC calibration process
* Successful motor arming/disarming operation
* Functional quadcopter prototype developed

---

## Future Improvements

* IMU-based stabilization
* PID flight controller implementation
* GPS navigation
* Telemetry system
* FPV camera integration
* Autonomous flight capabilities

---

## Author

**Vardhan Patel**

Electronics and Communication Engineering Student

Government Engineering College, Bharuch

---

## License

This project is intended for educational and research purposes.
