# NTRIP Client for GNSS

This is a C++ application that functions as a versatile NTRIP (Networked Transport of RTCM 
via Internet Protocol) client for GNSS applications. Its core purpose is to connect to an 
NTRIP caster, receive RTCM correction data, and forward it to a serial port connected to a 
GNSS receiver, enabling high-precision RTK positioning for u-blox Zed F9P or similar devices.

The application is designed to be configurable and includes several advanced features such 
as automatic mountpoint detection and initialization of u-blox receivers.

It was concieved to be used with the [Centipede project](https://map.centipede-rtk.org/),
a community project to build a network of freely accessible NTRIP base stations.

## Features

- **NTRIP Client:** Connects to a specified NTRIP caster and mountpoint to stream RTCM correction data.
- **GPS Initialization:** Sends configuration messages (UBX protocol) to a u-blox GNSS receiver at startup to set the measurement rate to 10Hz and query the device version.
- **Automatic Mountpoint Detection:** If the mountpoint is set to `auto`, the client uses the receiver's initial position to find and connect to the closest NTRIP mountpoint from the caster's source table.
- **NMEA GPS Data Display:** Can concurrently read NMEA messages from the serial port, parse them, and display key information (position, fix status, speed, etc.) in a formatted, continuously updating view in the console.
- **Interactive Configuration:** A setup script is provided to generate the `config.ini` file interactively.
- **Lightweight and cross platform:** Only dependency is the *Qt* library, which is completely cross platform.

## Getting Started

Follow these instructions to configure, build, and run the application.

### Prerequisites

- A C++17 compatible compiler (e.g., GCC, Clang)
- CMake (version 3.14 or higher)
- Qt6 libraries (core, network, serialport)

On a Debian-based system (like Ubuntu), you can install these with:
```sh
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-serialport-dev
```

### 1. Configuration

Before the first run, generate the `config.ini` file using the interactive setup script.

```sh
chmod +x setup.sh
./setup.sh
```

This script will guide you through setting the necessary parameters, such as the NTRIP 
caster details, your serial port, and credentials. To use the automatic mountpoint 
detection feature, simply enter `auto` when prompted for the mountpoint.

### 2. Building

The project uses a standard CMake build process.

```sh
mkdir -p build
cmake -S . -B build
cmake --build build -j
```

This will create the executable `rtkrover_qt` inside the `build` directory.

### 3. Running

Once built, you can run the client from the project's root directory:

```sh
./build/rtkrover_qt
```

The application will read `config.ini`, initialize the GPS receiver, perform mountpoint detection if configured, and then start streaming data.

## Configuration

The application's behavior is controlled by the `config.ini` file, which is structured as follows:

```ini
[ntrip]
host = crtk.net
port = 2101
mountpoint = auto
username = centipede
password = centipede

[serial]
port = /dev/ttyACM0
baud = 115200

[output]
read_from_serial = true
```

- **[ntrip]**: Settings for the NTRIP caster.
    - `host`/`port`: The address of the NTRIP caster. For Centipede use `crtk.net` and `2101`
    - `mountpoint`: The specific data stream to connect to. Use `auto` for automatic detection.
    - `username`/`password`: Your credentials for the NTRIP service. For Centipede use `centipede`/`centipede`
- **[serial]**: Settings for the serial port connected to your GNSS receiver.
    - `port`: The device path (e.g., `/dev/ttyACM0` on Linux).
    - `baud`: The baud rate for the serial connection.
- **[output]**:
    - `read_from_serial`: If `true`, the application will read NMEA data from the GPS and display a live status screen in the console.

---

## License

**GtkRover 1.0**
Copyright (C) 2026  *Barna Keresztes*

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
