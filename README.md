# gSENSOR

An open-source high-g accelerometer project based on ESP32-C3 and ADXL375.

## Overview

gSENSOR is designed for measuring high-g accelerations (up to ±200g) with real-time display and wireless data streaming. Perfect for impact testing, vibration analysis, and other applications requiring high-g measurement.

## Features

- **High-g Measurement**: ADXL375 accelerometer with ±200g range
- **Real-time Display**: Round LCD showing live X, Y, Z values and magnitude
- **BLE Streaming**: Wireless data transmission at 20Hz to companion app
- **USB Serial Output**: 100Hz CSV data stream for logging
- **Peak Tracking**: Monitor and reset peak acceleration values
- **Android App**: Companion app for visualization, recording, and data export

## Hardware

- **MCU**: ESP32-2424S012 (ESP32-C3 with 1.28" round LCD)
- **Accelerometer**: ADXL375 (±200g, I2C interface)
- **Display**: GC9A01 round LCD (240x240)

### Wiring

| ADXL375 | ESP32-2424S012 |
|---------|----------------|
| VCC     | 3.3V           |
| GND     | GND            |
| SDA     | GPIO4          |
| SCL     | GPIO5          |

## Project Structure

```
gSENSOR/
├── Firmware/           # ESP32 PlatformIO project
│   ├── src/           # Source code
│   ├── include/       # Header files
│   └── platformio.ini # Build configuration
├── gSENSOR-Android/   # Android companion app
│   ├── app/           # App source code
│   └── Dockerfile     # Docker build environment
└── 3D_Printed_Parts/  # Enclosure STL files
```

## Building

### Firmware

```bash
cd Firmware
pio run
pio run -t upload
```

### Android App

Using Docker:
```bash
cd gSENSOR-Android
docker build -t gsensor-android-build .
docker run --rm --user root -e GRADLE_USER_HOME=/tmp/gradle \
  -v "$(pwd):/app" -w /app gsensor-android-build \
  sh -c "./gradlew assembleDebug --no-daemon --project-cache-dir=/tmp/project-cache"
```

APK output: `gSENSOR-Android/app/build/outputs/apk/debug/app-debug.apk`

## BLE Protocol

**Service UUID**: `12345678-1234-5678-1234-56789abcde00`

| Characteristic | UUID | Description |
|----------------|------|-------------|
| Accel Data | `...de01` | 20-byte packet: timestamp(4) + x(4) + y(4) + z(4) + magnitude(4) |
| Peak Value | `...de02` | Peak magnitude tracking |
| Control | `...de03` | Commands: 0x01=reset peak, 0x02=reset filters |

## License

MIT License

## Author

**Giorgio Gilestro**
- Email: giorgio@gilest.ro
- GitHub: https://github.com/gilestrolab/gSensor
