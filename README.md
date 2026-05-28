

This project is a high-efficiency security system that leverages low-level hardware features to provide secure entry and visual feedback with minimal power consumption.

*Intelligent Power Management*:- Automatically enters a low-power Stop-2 Mode when idle (using RCC & PWR) and resulted in ~60% current reduction in current consumption. LPUART wakes up the system when detects a Start bit at the Rx pin. 

*Dynamic Visual Interface*:- Drives an OLED display to show system status & animations (using I2C's Fast Mode). It uses direct hardware memory transfers to keep animations smooth without slowing down the security logic using hardware timers.

*Secure Passkey Entry*:- Receives and processes security codes via LPUART communication. It handles incoming data & prompt transmissions using non-blocking methods to ensure the system never freezes while waiting for user input.

*Hardware-Level Efficiency*:- Offloads data movement and timing tasks to dedicated hardware controllers (DMA and Timers), ensuring the main "brain/CPU" of the lock is free to handle critical security and state-machine logic.

*Real-Time Verification*:- System performance, Current Consumption and Signal timing are validated using Logic Analyzer & Multimeter to ensure reliability in real-world conditions.

(*Click the image below to see the full demo*)
<p align="center">
  <a href="https://youtu.be/6xeKPMEZ6gM" target="_blank">
     <img src="https://img.youtube.com/vi/6xeKPMEZ6gM/maxresdefault.jpg" alt="Smart Locking System Demo" width="600" />
  </a>
</p>
