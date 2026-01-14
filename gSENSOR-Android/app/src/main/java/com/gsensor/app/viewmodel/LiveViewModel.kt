package com.gsensor.app.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.gsensor.app.data.ble.ConnectionState
import com.gsensor.app.data.model.AccelData
import com.gsensor.app.data.model.PeakData
import com.gsensor.app.data.repository.GSensorRepository
import kotlinx.coroutines.flow.*
import kotlinx.coroutines.launch

/**
 * ViewModel for the live data display screen.
 */
class LiveViewModel(private val repository: GSensorRepository) : ViewModel() {

    // Connection state
    val connectionState: StateFlow<ConnectionState> = repository.connectionState

    // Latest accelerometer data
    private val _latestData = MutableStateFlow<AccelData?>(null)
    val latestData: StateFlow<AccelData?> = _latestData.asStateFlow()

    // Chart data (last 100 samples = 5 seconds at 20 Hz)
    private val _chartData = MutableStateFlow<List<AccelData>>(emptyList())
    val chartData: StateFlow<List<AccelData>> = _chartData.asStateFlow()

    // Peak value
    private val _peakValue = MutableStateFlow(0f)
    val peakValue: StateFlow<Float> = _peakValue.asStateFlow()

    // Recording state
    val isRecording: StateFlow<Boolean> = repository.isRecording
    val recordingSampleCount: StateFlow<Int> = repository.recordingSampleCount
    val recordingStartTime: StateFlow<Long?> = repository.recordingStartTime

    private val chartBuffer = ArrayDeque<AccelData>(CHART_BUFFER_SIZE)

    init {
        // Collect accelerometer data
        viewModelScope.launch {
            repository.accelData.collect { data ->
                _latestData.value = data

                // Update chart buffer
                chartBuffer.addLast(data)
                while (chartBuffer.size > CHART_BUFFER_SIZE) {
                    chartBuffer.removeFirst()
                }
                _chartData.value = chartBuffer.toList()

                // Track local peak
                if (data.magnitude > _peakValue.value) {
                    _peakValue.value = data.magnitude
                }
            }
        }

        // Collect peak data from device
        viewModelScope.launch {
            repository.peakData.collect { peak ->
                _peakValue.value = peak.peak
            }
        }
    }

    /**
     * Start recording.
     */
    fun startRecording() {
        viewModelScope.launch {
            repository.startRecording()
        }
    }

    /**
     * Stop recording.
     */
    fun stopRecording() {
        viewModelScope.launch {
            repository.stopRecording()
        }
    }

    /**
     * Reset peak value on device.
     */
    fun resetPeak() {
        repository.bleManager.resetPeak()
        _peakValue.value = _latestData.value?.magnitude ?: 0f
    }

    /**
     * Disconnect from device.
     */
    fun disconnect() {
        repository.bleManager.disconnect()
    }

    companion object {
        private const val CHART_BUFFER_SIZE = 100 // 5 seconds at 20 Hz

        fun provideFactory(repository: GSensorRepository): ViewModelProvider.Factory {
            return object : ViewModelProvider.Factory {
                @Suppress("UNCHECKED_CAST")
                override fun <T : ViewModel> create(modelClass: Class<T>): T {
                    return LiveViewModel(repository) as T
                }
            }
        }
    }
}
