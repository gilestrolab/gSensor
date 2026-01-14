package com.gsensor.app.data.model

import androidx.room.Entity
import androidx.room.PrimaryKey

/**
 * Recording session metadata stored in the database.
 */
@Entity(tableName = "recording_sessions")
data class RecordingSession(
    @PrimaryKey(autoGenerate = true)
    val id: Long = 0,

    /** Unix timestamp when recording started */
    val startTime: Long,

    /** Unix timestamp when recording ended, null if still recording */
    val endTime: Long? = null,

    /** Total number of samples recorded */
    val sampleCount: Int = 0,

    /** Peak magnitude observed during recording */
    val peakMagnitude: Float = 0f,

    /** Optional user notes */
    val notes: String? = null
) {
    /** Duration in milliseconds, or null if still recording */
    val durationMs: Long?
        get() = endTime?.let { it - startTime }

    /** Check if recording is still in progress */
    val isRecording: Boolean
        get() = endTime == null
}
