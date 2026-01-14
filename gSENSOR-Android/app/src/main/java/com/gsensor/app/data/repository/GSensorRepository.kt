package com.gsensor.app.data.repository

import android.content.Context
import android.content.Intent
import com.gsensor.app.data.ble.BleManager
import com.gsensor.app.data.ble.ConnectionState
import com.gsensor.app.data.db.GSensorDatabase
import com.gsensor.app.data.db.RecordingDao
import com.gsensor.app.data.export.CsvExporter
import com.gsensor.app.data.model.AccelData
import com.gsensor.app.data.model.AccelSample
import com.gsensor.app.data.model.RecordingSession
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.*
import kotlinx.coroutines.launch

/**
 * Repository for gSENSOR data operations.
 *
 * Coordinates between BLE manager, database, and export functionality.
 */
class GSensorRepository(context: Context) {

    val bleManager = BleManager(context)
    private val database = GSensorDatabase.getDatabase(context)
    private val dao: RecordingDao = database.recordingDao()
    private val csvExporter = CsvExporter(context)

    private val repositoryScope = CoroutineScope(Dispatchers.IO)

    // Recording state
    private var activeSessionId: Long? = null
    private var recordingJob: Job? = null
    private var sampleBuffer = mutableListOf<AccelSample>()
    private var peakMagnitude = 0f

    private val _isRecording = MutableStateFlow(false)
    val isRecording: StateFlow<Boolean> = _isRecording.asStateFlow()

    private val _recordingSampleCount = MutableStateFlow(0)
    val recordingSampleCount: StateFlow<Int> = _recordingSampleCount.asStateFlow()

    private val _recordingStartTime = MutableStateFlow<Long?>(null)
    val recordingStartTime: StateFlow<Long?> = _recordingStartTime.asStateFlow()

    // Expose BLE data
    val connectionState: StateFlow<ConnectionState> = bleManager.connectionState
    val accelData: SharedFlow<AccelData> = bleManager.accelData
    val peakData = bleManager.peakData

    // Expose recording sessions
    val allSessions: Flow<List<RecordingSession>> = dao.getAllSessions()

    /**
     * Start recording accelerometer data.
     */
    suspend fun startRecording(): Long {
        if (_isRecording.value) return activeSessionId ?: -1

        val session = RecordingSession(startTime = System.currentTimeMillis())
        val sessionId = dao.insertSession(session)
        activeSessionId = sessionId
        sampleBuffer.clear()
        peakMagnitude = 0f
        _recordingSampleCount.value = 0
        _recordingStartTime.value = session.startTime
        _isRecording.value = true

        // Start collecting samples
        recordingJob = repositoryScope.launch {
            bleManager.accelData.collect { data ->
                if (_isRecording.value && activeSessionId != null) {
                    val sample = AccelSample.fromAccelData(activeSessionId!!, data)
                    sampleBuffer.add(sample)

                    if (data.magnitude > peakMagnitude) {
                        peakMagnitude = data.magnitude
                    }

                    _recordingSampleCount.value = sampleBuffer.size

                    // Batch insert every 100 samples
                    if (sampleBuffer.size >= 100) {
                        dao.insertSamples(sampleBuffer.toList())
                        sampleBuffer.clear()
                    }
                }
            }
        }

        return sessionId
    }

    /**
     * Stop recording and finalize the session.
     */
    suspend fun stopRecording(): RecordingSession? {
        if (!_isRecording.value) return null

        recordingJob?.cancel()
        recordingJob = null

        val sessionId = activeSessionId ?: return null
        val endTime = System.currentTimeMillis()

        // Insert remaining samples
        if (sampleBuffer.isNotEmpty()) {
            dao.insertSamples(sampleBuffer.toList())
        }

        val finalSampleCount = dao.getSampleCount(sessionId)
        val finalPeak = dao.getPeakMagnitude(sessionId) ?: peakMagnitude

        dao.finishSession(sessionId, endTime, finalSampleCount, finalPeak)

        _isRecording.value = false
        _recordingStartTime.value = null
        activeSessionId = null
        sampleBuffer.clear()

        return dao.getSession(sessionId)
    }

    /**
     * Get samples for a specific session.
     */
    suspend fun getSamplesForSession(sessionId: Long): List<AccelSample> {
        return dao.getSamplesForSession(sessionId)
    }

    /**
     * Get a specific session.
     */
    suspend fun getSession(sessionId: Long): RecordingSession? {
        return dao.getSession(sessionId)
    }

    /**
     * Delete a recording session.
     */
    suspend fun deleteSession(session: RecordingSession) {
        dao.deleteSession(session)
    }

    /**
     * Export session to CSV and create share intent.
     */
    suspend fun exportSession(sessionId: Long): Intent? {
        val session = dao.getSession(sessionId) ?: return null
        val samples = dao.getSamplesForSession(sessionId)
        return csvExporter.exportAndShare(session, samples)
    }

    /**
     * Clean up resources.
     */
    fun close() {
        recordingJob?.cancel()
        bleManager.close()
    }
}
