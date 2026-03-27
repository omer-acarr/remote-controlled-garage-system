<img width="1600" height="1200" alt="image" src="https://github.com/user-attachments/assets/2d9ce830-cde0-4645-96f5-eececa3a5f9c" />
<img width="1200" height="1600" alt="image" src="https://github.com/user-attachments/assets/5cb93c27-0db3-4251-8a25-0b02542b6a3d" />

# STM32 Automated Smart Barrier System

This project is an embedded system developed using the **STM32F407G-DISC1** microcontroller. It implements a smart parking/garage barrier that detects vehicle proximity using dual ultrasonic sensors and controls a servo-mechanism with real-time feedback.

## 🚀 Working Principle & Logic

The core logic is built on an **Interrupt-driven architecture** to ensure high responsiveness and efficient CPU utilization. The system continuously monitors vehicle movement through two strategic points.

* **Distance Analysis:** Dual HC-SR04 ultrasonic sensors calculate the proximity of an approaching or departing vehicle.
* **Automated Servo Control:** When a vehicle is detected within the threshold, the system triggers the servo motor to lift the barrier.
* **Acoustic Feedback:** Integrated buzzer provides different frequency signals to notify the user of barrier movements and vehicle detection status.
* **Visual Status:** Real-time system states (e.g., "ARABA YOK" / "NO CAR") are displayed via an OLED screen and onboard LEDs.

## 🛠 Tech Stack & Hardware

* **Microcontroller:** STM32F407G (ARM Cortex-M4)
* **Sensors:** 2x HC-SR04 Ultrasonic Distance Sensors
* **Actuator:** SG90 Servo Motor
* **Feedback:** Active/Passive Buzzer & Status LEDs / OLED Display
* **Architecture:** External Interrupts (EXTI) for sensor echo and Timer-based PWM for servo control.

## 📂 Project Structure

* `Core/Src`: Main application logic and Interrupt Service Routines (ISR).
* `Drivers`: Peripheral drivers for sensors and actuators.
* `Debug`: Build artifacts and memory maps.

## ⚙️ How to Run

1.  Clone this repository to your local machine.
2.  Open the project in **STM32CubeIDE** or a compatible ARM IDE.
3.  Connect your STM32F4 Discovery board via USB.
4.  Build and Flash the project to the target.
5.  Monitor the distance values and system logs via the integrated display or serial console.

