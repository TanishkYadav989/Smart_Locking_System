This project is a high-efficiency security system that leverages low-level hardware features to provide secure entry and visual feedback with minimal power consumption.

*Acoustic Wake-up & Monitoring*:- Uses a microphone to detect physical knocks on the door. The system "listens" in the background and only alerts the main processor when a significant acoustic event occurs (using Analog Watchdog), allowing the device to stay quiet and save power.

*Intelligent Power Management*:- Automatically enters a low-power sleep state when idle (using RCC & PWR). Crucial components and timing hardware remain active in the background (using SMENR in RCC) to bring the system back "online" the instant a 3-knocks are detected on the door.

*Dynamic Visual Interface*:- Drives an OLED display to show system status & animations (using I2C's Fast Mode). It uses direct hardware memory transfers to keep animations smooth without slowing down the security logic using hardware timers.

*Secure Passkey Entry*:- Receives and processes security codes via UART communication. It handles incoming data & prompt transmissions using non-blocking methods to ensure the system never freezes while waiting for user input.

*Hardware-Level Efficiency*:- Offloads data movement and timing tasks to dedicated hardware controllers (DMA and Timers), ensuring the main "brain/CPU" of the lock is free to handle critical security and state-machine logic.

*Real-Time Verification*:- System performance and signal timing are validated using Logic Analyzer to ensure reliability in real-world conditions
