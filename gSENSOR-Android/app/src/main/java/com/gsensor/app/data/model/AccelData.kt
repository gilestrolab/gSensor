package com.gsensor.app.data.model

import java.nio.ByteBuffer
import java.nio.ByteOrder

/**
 * Accelerometer data received from gSENSOR device.
 *
 * Binary format (20 bytes, little-endian):
 * - timestamp: UInt32 (4 bytes) - device timestamp in ms
 * - x: Float (4 bytes) - X-axis acceleration in g
 * - y: Float (4 bytes) - Y-axis acceleration in g
 * - z: Float (4 bytes) - Z-axis acceleration in g
 * - magnitude: Float (4 bytes) - calculated magnitude in g
 */
data class AccelData(
    val timestamp: Long,
    val x: Float,
    val y: Float,
    val z: Float,
    val magnitude: Float,
    val receivedAt: Long = System.currentTimeMillis()
) {
    companion object {
        const val PACKET_SIZE = 20

        /**
         * Parse binary data from BLE notification.
         *
         * @param bytes Raw byte array from BLE characteristic
         * @return AccelData if parsing successful, null otherwise
         */
        fun fromByteArray(bytes: ByteArray): AccelData? {
            if (bytes.size < PACKET_SIZE) return null

            return try {
                val buffer = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN)
                AccelData(
                    timestamp = buffer.int.toLong() and 0xFFFFFFFFL,
                    x = buffer.float,
                    y = buffer.float,
                    z = buffer.float,
                    magnitude = buffer.float
                )
            } catch (e: Exception) {
                null
            }
        }
    }
}

/**
 * Peak value data received from gSENSOR device.
 *
 * Binary format (8 bytes, little-endian):
 * - timestamp: UInt32 (4 bytes)
 * - peak: Float (4 bytes)
 */
data class PeakData(
    val timestamp: Long,
    val peak: Float
) {
    companion object {
        const val PACKET_SIZE = 8

        fun fromByteArray(bytes: ByteArray): PeakData? {
            if (bytes.size < PACKET_SIZE) return null

            return try {
                val buffer = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN)
                PeakData(
                    timestamp = buffer.int.toLong() and 0xFFFFFFFFL,
                    peak = buffer.float
                )
            } catch (e: Exception) {
                null
            }
        }
    }
}
