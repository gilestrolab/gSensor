package com.gsensor.app.data.ble

import java.util.UUID

/**
 * BLE constants matching the ESP32 gSENSOR firmware.
 */
object BleConstants {
    const val DEVICE_NAME = "gSENSOR"

    // Service UUID
    val SERVICE_UUID: UUID = UUID.fromString("12345678-1234-5678-1234-56789abcde00")

    // Characteristic UUIDs
    val CHAR_ACCEL_UUID: UUID = UUID.fromString("12345678-1234-5678-1234-56789abcde01")
    val CHAR_PEAK_UUID: UUID = UUID.fromString("12345678-1234-5678-1234-56789abcde02")
    val CHAR_CONTROL_UUID: UUID = UUID.fromString("12345678-1234-5678-1234-56789abcde03")
    val CHAR_CONFIG_UUID: UUID = UUID.fromString("12345678-1234-5678-1234-56789abcde04")

    // Client Characteristic Configuration Descriptor (for notifications)
    val CCCD_UUID: UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")

    // Control commands
    const val CMD_RESET_PEAK: Byte = 0x01
    const val CMD_RESET_FILTERS: Byte = 0x02

    // Scan settings
    const val SCAN_TIMEOUT_MS = 10000L
}
