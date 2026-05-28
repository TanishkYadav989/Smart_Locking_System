### Module Map

* **LPUART_pass.c / .h** (contains `Enter_Stop2()` at the bottom) — Handles asynchronous communication, low-power entry logic, and DMA request generation.
* **main.c** — Contains the core Mealy Finite State Machine logic and main execution loop.
* **LATCH.c / .h** — Driver for the solenoid lock hardware switching circuit.
* **REED_EXTI.c / .h** — Handles the external door sensor interrupt routine to wake the MCU from Stop 2 mode for alerts & door status detection
* **OLED_I2C.c / .h** — Custom register-level driver for the SSD1306 OLED display panel, achieving 25-FPS of lock opening animatiom & other UI. 
* **BUZZER.c / .h** — Driver for the passive buzzer feedback system.
  
*State-Transition Diagram*:-

![Smart Lock State Machine](State%20Machine%20Transition%20Diagram.JPG)
