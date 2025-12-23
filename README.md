Fun Projects I've Done:

# ⛈️ Project 1: Weather Station Display

This is the first project I've ever done with ESP32 and the Arduino Framework. 
It displays the time in an accurate, real time manner as well as fetches local weather updates such as temperature and condition.

## 🧰 Tool's
 - ArduinoIDE
 - ESP32 DevKit V1
 - SSD1306 Oled Display

## 💡 Features
 - Displays time using RTC on OLED
 - Displays weather info from OpenWeatherMap on OLED
 - Program structured through RTOS

# ⏲️ Project 2: ESP32 Reaction Test Timer

This project involves the ESP32 and Arduino Framework/C++ to implement a fun little reaction test timer.
It displays the users best and current reaction time to a toggled LED light, and displays on an OLED screen. 

## 🧰 Tool's
 - PlatformIO
 - ESP32 DevKit V1
 - SSD1306 Oled Display
 - LED + 220 Ohm resistor
 - External Pushbuttons

## 💡 Features
 - Displays current and best time for reaction to an LED
 - Uses push buttons to toggle measurement as well as reset system
 - Implemented as a FSM (Finite State Machine)
 - Uses Ticker's and Debouncing to ensure efficient state flow

# 📸 School Project: Security System

This was a project I worked on for half a semester in MECHTRON 2TA4, at McMaster University.
It's a mock security-system that utilizes a STM32F429ZI Discovery Board along with an OP344 Sound Sensor from SparkFun, to detect audible intrusion and alert the user via piezzo buzzer.

## 🧰 Tool's
 - STM32F429ZI Discovery Board
 - OP344 Sound Sensor
 - Piezobuzzer
 - Push Button's
   
## 💡 Features
 - Graphical UI for keypad to enable/disable system (via passcode), as well for displaying information to user
 - Touch-screen functionality for built in LCD display to register user input when entering passcode
 - Interrupt-driven button's to set states and enable functionality of the system
 - Timeout's/Ticker's to handle timing-sensitive logic, and ensure appropriate stateflow
 - Custom sound-sensor logic to handle sensing through voltage-spike detection
