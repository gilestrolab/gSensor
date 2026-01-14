package com.gsensor.app.data.model

import androidx.room.Entity
import androidx.room.ForeignKey
import androidx.room.Index
import androidx.room.PrimaryKey

/**
 * Individual accelerometer sample stored in the database.
 */
@Entity(
    tableName = "accel_samples",
    foreignKeys = [
        ForeignKey(
            entity = RecordingSession::class,
            parentColumns = ["id"],
            childColumns = ["sessionId"],
            onDelete = ForeignKey.CASCADE
        )
    ],
    indices = [Index("sessionId")]
)
data class AccelSample(
    @PrimaryKey(autoGenerate = true)
    val id: Long = 0,

    /** Foreign key to RecordingSession */
    val sessionId: Long,

    /** Device timestamp in milliseconds */
    val timestamp: Long,

    /** X-axis acceleration in g */
    val x: Float,

    /** Y-axis acceleration in g */
    val y: Float,

    /** Z-axis acceleration in g */
    val z: Float,

    /** Calculated magnitude in g */
    val magnitude: Float
) {
    companion object {
        /**
         * Create AccelSample from AccelData for a specific session.
         */
        fun fromAccelData(sessionId: Long, data: AccelData): AccelSample {
            return AccelSample(
                sessionId = sessionId,
                timestamp = data.timestamp,
                x = data.x,
                y = data.y,
                z = data.z,
                magnitude = data.magnitude
            )
        }
    }
}
