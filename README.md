========================================================================
SUMOBOT V1: ESP32-POWERED SUMO ROBOT
========================================================================

A custom-built Sumo robot featuring an ESP32 microcontroller, differential 
drive system, and wireless Xbox controller Bluetooth integration.

------------------------------------------------------------------------
1. SYSTEM ARCHITECTURE
------------------------------------------------------------------------
* Controller: ESP32 (ESP-IDF Framework)
* Communication: Bluetooth Low Energy (BLE) via NimBLE stack
* Drive System: Differential Drive (2x DC Motors)
* Motor Control: PWM speed regulation via LEDC driver

------------------------------------------------------------------------
2. PROJECT STRUCTURE
------------------------------------------------------------------------
sumobot_v1/
├── main/
│   ├── sumobot_v1.c       # BLE Logic & Xbox integration
│   ├── motor_control.c    # PWM and GPIO Motor Drivers
│   ├── motor_control.h    # Header with pin definitions
│   └── CMakeLists.txt     # Build configuration
└── CMakeLists.txt         # Project root configuration

------------------------------------------------------------------------
3. GETTING STARTED & ENVIRONMENT SETUP
------------------------------------------------------------------------
Every time you open a new terminal session, run the following commands 
from the project root directory:

Step 1: Export ESP-IDF environment variables
    . $HOME/esp/esp-idf/export.sh

Step 2: Navigate to your project root
    cd ~/Desktop/sumo/sumobot_v1

Step 3: Compile, flash, and open the serial monitor
    idf.py flash monitor

* Note: To exit the serial monitor, use the shortcut: Ctrl + ]
* Note: If you encounter bonding or pairing loops, clear the NVS flash 
  by running 'idf.py erase-flash' before re-flashing.

------------------------------------------------------------------------
4. CORE FEATURES
------------------------------------------------------------------------
* Differential Steering: Kinematic mixing algorithm that translates 
  joystick axes and trigger inputs into smooth arc turns and zero-radius 
  pivots.
* Robust BLE Link: Configured using the NimBLE stack with optimized 
  connection parameters and an extended supervision timeout (5.12s) to 
  survive heavy motor electromagnetic interference (EMI).
* Fail-safe Braking: Actively pulls control pins HIGH to engage the 
  electronic brake of the H-Bridge when stopping or upon unexpected 
  disconnection.
