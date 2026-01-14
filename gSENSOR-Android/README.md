# gSENSOR Android Companion App

Android companion app for the gSENSOR accelerometer device.

## Features

- **BLE Connection**: Scan and connect to gSENSOR devices via Bluetooth Low Energy
- **Real-time Display**: View live X, Y, Z acceleration and magnitude values
- **Scrolling Chart**: Visual representation of acceleration data over time
- **Peak Tracking**: Monitor and reset peak acceleration values
- **Recording**: Record acceleration sessions to local database
- **CSV Export**: Export recorded sessions to CSV format for analysis
- **Share**: Share exported data via Android share sheet

## Requirements

- Android 8.0 (API 26) or higher
- Bluetooth Low Energy support
- gSENSOR device (ESP32 + ADXL375)

## Building

### Using Docker (recommended)

```bash
docker build -t gsensor-android-build .
docker run --rm --user root -e GRADLE_USER_HOME=/tmp/gradle \
  -v "$(pwd):/app" -w /app gsensor-android-build \
  sh -c "./gradlew assembleDebug --no-daemon --project-cache-dir=/tmp/project-cache && chown -R $(id -u):$(id -g) /app/app/build"
```

The APK will be at `app/build/outputs/apk/debug/app-debug.apk`

### Using Android Studio

1. Open the project in Android Studio
2. Sync Gradle files
3. Build > Build Bundle(s) / APK(s) > Build APK(s)

## Architecture

- **Kotlin** with Jetpack Compose for UI
- **MVVM** architecture pattern
- **Room** database for local storage
- **Coroutines/Flow** for reactive data handling
- **Material3** design components

## Project Structure

```
app/src/main/java/com/gsensor/app/
├── data/
│   ├── ble/          # BLE manager and constants
│   ├── db/           # Room database
│   ├── export/       # CSV export functionality
│   ├── model/        # Data classes
│   └── repository/   # Data repository
├── ui/
│   ├── components/   # Reusable UI components
│   ├── navigation/   # Navigation graph
│   ├── screens/      # App screens
│   └── theme/        # Material theme
└── viewmodel/        # ViewModels
```

## License

MIT License

## Author

Giorgio Gilestro - giorgio@gilest.ro

## Links

- GitHub: https://github.com/gilestrolab/gSensor
