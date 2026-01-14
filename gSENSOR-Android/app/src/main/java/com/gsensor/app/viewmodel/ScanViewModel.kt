package com.gsensor.app.viewmodel

import android.bluetooth.BluetoothDevice
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.gsensor.app.data.ble.BleConstants
import com.gsensor.app.data.ble.ConnectionState
import com.gsensor.app.data.ble.ScannedDevice
import com.gsensor.app.data.repository.GSensorRepository
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

/**
 * ViewModel for the device scanning screen.
 */
class ScanViewModel(private val repository: GSensorRepository) : ViewModel() {

    private val _devices = MutableStateFlow<List<ScannedDevice>>(emptyList())
    val devices: StateFlow<List<ScannedDevice>> = _devices.asStateFlow()

    private val _isScanning = MutableStateFlow(false)
    val isScanning: StateFlow<Boolean> = _isScanning.asStateFlow()

    private val _errorMessage = MutableStateFlow<String?>(null)
    val errorMessage: StateFlow<String?> = _errorMessage.asStateFlow()

    private val _scanComplete = MutableStateFlow(false)
    val scanComplete: StateFlow<Boolean> = _scanComplete.asStateFlow()

    val connectionState: StateFlow<ConnectionState> = repository.connectionState

    private var scanJob: Job? = null

    /**
     * Start scanning for BLE devices.
     */
    fun startScan() {
        if (_isScanning.value) return

        _isScanning.value = true
        _devices.value = emptyList()
        _errorMessage.value = null
        _scanComplete.value = false

        scanJob = viewModelScope.launch {
            try {
                repository.bleManager.scanForDevices().collect { deviceList ->
                    // Sort to show gSENSOR devices first
                    _devices.value = deviceList.sortedByDescending {
                        it.name == BleConstants.DEVICE_NAME
                    }
                }
            } catch (e: CancellationException) {
                // Normal cancellation (timeout or user stopped) - not an error
                _scanComplete.value = true
            } catch (e: Exception) {
                _errorMessage.value = e.message ?: "Scan failed"
            } finally {
                _isScanning.value = false
            }
        }

        // Auto-stop scan after timeout
        viewModelScope.launch {
            delay(BleConstants.SCAN_TIMEOUT_MS)
            stopScan()
        }
    }

    /**
     * Clear error message.
     */
    fun clearError() {
        _errorMessage.value = null
    }

    /**
     * Stop scanning for devices.
     */
    fun stopScan() {
        scanJob?.cancel()
        scanJob = null
        _isScanning.value = false
    }

    /**
     * Connect to a device.
     */
    fun connect(device: BluetoothDevice) {
        stopScan()
        repository.bleManager.connect(device)
    }

    /**
     * Check if Bluetooth is enabled.
     */
    fun isBluetoothEnabled(): Boolean = repository.bleManager.isBluetoothEnabled()

    override fun onCleared() {
        super.onCleared()
        stopScan()
    }

    companion object {
        fun provideFactory(repository: GSensorRepository): ViewModelProvider.Factory {
            return object : ViewModelProvider.Factory {
                @Suppress("UNCHECKED_CAST")
                override fun <T : ViewModel> create(modelClass: Class<T>): T {
                    return ScanViewModel(repository) as T
                }
            }
        }
    }
}
