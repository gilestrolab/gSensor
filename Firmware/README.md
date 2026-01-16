# gSENSOR Firmware

ESP32-based high-G accelerometer firmware for the ESP32-2424S012 development board with ADXL375 sensor.

## Features

- Real-time acceleration measurement up to **200g**
- Round LCD display with Racing HUD gauge UI
- Configurable sample rates: 100, 200, 400, 800 Hz
- BLE data streaming
- Serial data output (CSV format)
- Touch-based settings interface
- Peak value tracking with visual indicators

## Hardware

### Main Board: ESP32-2424S012

| Component | Specification |
|-----------|---------------|
| MCU | ESP32-C3-MINI-1U (4MB Flash) |
| Display | GC9A01 240x240 round LCD |
| Touch | CST816D capacitive |
| Connectivity | WiFi 802.11 b/g/n, Bluetooth 5.0 |
| Power | USB-C or 3.7V LiPo battery |

### Accelerometer: ADXL375

| Parameter | Value |
|-----------|-------|
| Range | +/- 200g |
| Sensitivity | 49 mg/LSB |
| Interface | I2C (0x53) |
| Max Data Rate | 3200 Hz |

### Wiring

Connect the ADXL375 to the JST connector on the ESP32-2424S012:

| ADXL375 | ESP32 GPIO |
|---------|------------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO21 |
| SCL | GPIO20 |

## Building & Flashing

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or VSCode extension)

### Build

```bash
cd Firmware
pio run
```

### Flash

```bash
pio run -t upload
```

### Serial Monitor

```bash
pio device monitor
```

## Serial Interface

**Baud Rate:** 115200

### Commands

| Command | Description |
|---------|-------------|
| `r` | Reset peak value |
| `c` | Reset filters (recalibrate) |
| `s1` | Set sample rate to 100 Hz |
| `s2` | Set sample rate to 200 Hz |
| `s3` | Set sample rate to 400 Hz |
| `s4` | Set sample rate to 800 Hz |
| `?` | Show current status |

### Output Format

CSV data is output continuously:

```
timestamp_ms,x,y,z,magnitude,peak
7257,-0.002,-0.375,-0.896,0.972,1.013
7267,-0.012,-0.380,-0.891,0.969,1.013
```

- `timestamp_ms`: Milliseconds since boot
- `x`, `y`, `z`: Acceleration in g (calibrated)
- `magnitude`: Vector magnitude sqrt(x^2 + y^2 + z^2)
- `peak`: Maximum magnitude since last reset

## Serial Plotter Tool

A Python tool for real-time visualization and data recording.

### Installation

```bash
cd tools
pip install -r requirements.txt
```

### Usage

```bash
python serial_plotter.py --port /dev/ttyACM0
```

### Controls

| Key/Button | Action |
|------------|--------|
| Space | Start/stop recording |
| S | Save recorded data to CSV |
| C | Clear display data |
| R | Reset peak value |

Recording automatically switches to 200 Hz for higher resolution.

## BLE Interface

### Service

- **Device Name:** gSENSOR
- **Service UUID:** `12345678-1234-5678-1234-56789abcde00`

### Characteristics

| Name | UUID | Properties |
|------|------|------------|
| Accel Data | `...de01` | Notify |
| Peak | `...de02` | Notify |
| Control | `...de03` | Write |
| Config | `...de04` | Read/Write |

### Control Commands (Write to Control characteristic)

- `0x01` - Reset peak
- `0x02` - Reset filters

### Config (Write to Config characteristic)

- Byte 0: Notification rate in Hz (5-50)

## User Interface

### Main Screen (Racing HUD)

- Arc gauge showing current magnitude
- Digital readout of current value
- Peak value display
- Color-coded thresholds:
  - Green: < 10g
  - Amber: < 50g
  - Coral: < 100g
  - Red: > 100g

### Settings Screen

Tap the display to access settings:

- Toggle BLE on/off
- Toggle Serial output on/off
- Long press to return to main screen

## Configuration

Edit `src/config.h` to customize:

### Sample Rate

```cpp
constexpr uint32_t ADXL_DEFAULT_SAMPLE_RATE_HZ = 100;
```

### Calibration Offsets

Adjust for your specific ADXL375 unit:

```cpp
constexpr float ADXL375_OFFSET_X = 0.35f;
constexpr float ADXL375_OFFSET_Y = -0.60f;
constexpr float ADXL375_OFFSET_Z = 0.70f;
```

### Display Refresh Rate

```cpp
constexpr uint32_t DISPLAY_UPDATE_INTERVAL_MS = 100;  // 10 Hz
```

## Project Structure

```
Firmware/
├── src/
│   ├── main.cpp              # Entry point, main loop
│   ├── config.h              # Hardware config, constants
│   ├── accelerometer.cpp/h   # ADXL375 driver
│   ├── display.cpp/h         # GC9A01 display driver
│   ├── signal_processing.cpp/h  # Filters, magnitude calc
│   ├── ble_service.cpp/h     # BLE GATT server
│   ├── touch.cpp/h           # Touch controller
│   ├── ui_manager.cpp/h      # UI state machine
│   └── settings.h            # Runtime settings
├── tools/
│   ├── serial_plotter.py     # Python visualization tool
│   └── requirements.txt      # Python dependencies
└── platformio.ini            # Build configuration
```

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| LovyanGFX | ^1.1.16 | Display driver |
| Adafruit ADXL375 | ^1.0.1 | Accelerometer driver |
| Adafruit BusIO | ^1.16.1 | I2C abstraction |
| NimBLE-Arduino | ^1.4.0 | BLE stack |

## Troubleshooting

### No serial output

1. Ensure USB CDC is enabled (check `platformio.ini` build flags)
2. Use correct port (`/dev/ttyACM0` on Linux)
3. Check baud rate is 115200

### I2C errors / No accelerometer data

1. Verify wiring to JST connector
2. Check I2C address (default 0x53)
3. Ensure 3.3V power to ADXL375

### Display not working

1. The ESP32-2424S012 uses fixed SPI pins - no configuration needed
2. Check backlight pin (GPIO3) is not conflicting

### Sample rate not changing

1. Ensure firmware is updated (serialEvent fix in v6b3c057)
2. Send command without newline: `s4` not `s4\n`

## License

MIT License - See repository root for details.
