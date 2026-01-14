package com.gsensor.app.data.db

import androidx.room.*
import com.gsensor.app.data.model.AccelSample
import com.gsensor.app.data.model.RecordingSession
import kotlinx.coroutines.flow.Flow

/**
 * Data Access Object for recording sessions and samples.
 */
@Dao
interface RecordingDao {

    // ==================== Recording Sessions ====================

    @Insert
    suspend fun insertSession(session: RecordingSession): Long

    @Update
    suspend fun updateSession(session: RecordingSession)

    @Delete
    suspend fun deleteSession(session: RecordingSession)

    @Query("SELECT * FROM recording_sessions ORDER BY startTime DESC")
    fun getAllSessions(): Flow<List<RecordingSession>>

    @Query("SELECT * FROM recording_sessions WHERE id = :sessionId")
    suspend fun getSession(sessionId: Long): RecordingSession?

    @Query("SELECT * FROM recording_sessions WHERE endTime IS NULL LIMIT 1")
    suspend fun getActiveSession(): RecordingSession?

    @Query("UPDATE recording_sessions SET endTime = :endTime, sampleCount = :sampleCount, peakMagnitude = :peakMagnitude WHERE id = :sessionId")
    suspend fun finishSession(sessionId: Long, endTime: Long, sampleCount: Int, peakMagnitude: Float)

    // ==================== Accelerometer Samples ====================

    @Insert
    suspend fun insertSample(sample: AccelSample)

    @Insert
    suspend fun insertSamples(samples: List<AccelSample>)

    @Query("SELECT * FROM accel_samples WHERE sessionId = :sessionId ORDER BY timestamp ASC")
    suspend fun getSamplesForSession(sessionId: Long): List<AccelSample>

    @Query("SELECT * FROM accel_samples WHERE sessionId = :sessionId ORDER BY timestamp ASC")
    fun getSamplesForSessionFlow(sessionId: Long): Flow<List<AccelSample>>

    @Query("SELECT COUNT(*) FROM accel_samples WHERE sessionId = :sessionId")
    suspend fun getSampleCount(sessionId: Long): Int

    @Query("SELECT MAX(magnitude) FROM accel_samples WHERE sessionId = :sessionId")
    suspend fun getPeakMagnitude(sessionId: Long): Float?

    @Query("DELETE FROM accel_samples WHERE sessionId = :sessionId")
    suspend fun deleteSamplesForSession(sessionId: Long)
}
