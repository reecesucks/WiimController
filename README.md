# WiimController

An ESP32-based media controller project written in C++, designed to control a WiiM audio receiver.

Originally developed as a .NET desktop application, the project was later redesigned as an embedded system to explore lower-level programming, firmware development, and hardware/software integration using the ESP32 platform.

---

## Overview

WiimController began as a desktop media remote application built with C# and .NET.

The project was later migrated to an ESP32 development board and rewritten in C++ as a way to gain hands-on experience with:
- Embedded systems development
- Firmware architecture
- Hardware integration
- Network-based device communication
- Real-time input handling
- Lower-level debugging workflows

The controller communicates with a WiiM receiver to provide media control functionality through custom embedded hardware.

---

## Features

- ESP32-based embedded firmware
- Media control integration for WiiM receivers
- Rewritten from the original .NET implementation into C++
- Real-time input handling
- Embedded networking and device communication
- Modular firmware structure for future expansion
- Git and GitHub-based development workflow

---

## Tech Stack

### Embedded / Firmware
- C++
- ESP32
- Arduino Framework / PlatformIO

### Original Desktop Application
- C#
- .NET
- Visual Studio

### Tools
- Git
- GitHub
- VS Code
- Visual Studio

---

## Project Structure

```text
WiimController/
├── DesktopApp/          # Original .NET application
├── esp32-firmware/      # ESP32 firmware project
├── docs/
└── README.md
