package com.gsensor.app.data.ble

import android.annotation.SuppressLint
import android.bluetooth.*
import android.bluetooth.le.*
import android.content.Context
import android.os.Build
import com.gsensor.app.data.model.AccelData
import com.gsensor.app.data.model.PeakData
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.*
import java.util.concurrent.ConcurrentHashMap

/**
 * Connection state for BLE device.
 */
sealed class ConnectionState {
    object Disconnected : ConnectionState()
    object Connecting : ConnectionState()
    object Connected : ConnectionState()
    data class Error(val message: String) : ConnectionState()
}

/**
 * Scan result wrapper.
 */
data class ScannedDevice(
    val device: BluetoothDevice,
    val rssi: Int,
    val name: String?
)

/**
 * BLE Manager for gSENSOR device communication.
 *
 * Handles scanning, connection, and data notifications.
 */
@SuppressLint("MissingPermission")
class BleManager(private val context: Context) {

    private val bluetoothManager: BluetoothManager =
        context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager

    private val bluetoothAdapter: BluetoothAdapter? = bluetoothManager.adapter

    private var bluetoothGatt: BluetoothGatt? = null
    private var scanner: BluetoothLeScanner? = null

    private val _connectionState = MutableStateFlow<ConnectionState>(ConnectionState.Disconnected)
    val connectionState: StateFlow<ConnectionState> = _connectionState.asStateFlow()

    private val _accelData = MutableSharedFlow<AccelData>(replay = 0, extraBufferCapacity = 100)
    val accelData: SharedFlow<AccelData> = _accelData.asSharedFlow()

    private val _peakData = MutableSharedFlow<PeakData>(replay = 1, extraBufferCapacity = 10)
    val peakData: SharedFlow<PeakData> = _peakData.asSharedFlow()

    private val scannedDevices = ConcurrentHashMap<String, ScannedDevice>()

    /**
     * Check if Bluetooth is available and enabled.
     */
    fun isBluetoothEnabled(): Boolean = bluetoothAdapter?.isEnabled == true

    /**
     * Scan for BLE devices.
     *
     * @return Flow of discovered devices
     */
    fun scanForDevices(): Flow<List<ScannedDevice>> = callbackFlow {
        scannedDevices.clear()

        // Check if Bluetooth is available
        if (bluetoothAdapter == null) {
            close(Exception("Bluetooth not available"))
            return@callbackFlow
        }

        if (!bluetoothAdapter.isEnabled) {
            close(Exception("Bluetooth is disabled"))
            return@callbackFlow
        }

        val bleScanner = bluetoothAdapter.bluetoothLeScanner
        if (bleScanner == null) {
            close(Exception("BLE Scanner not available"))
            return@callbackFlow
        }

        val scanCallback = object : ScanCallback() {
            override fun onScanResult(callbackType: Int, result: ScanResult) {
                val device = result.device
                val name = try {
                    result.scanRecord?.deviceName ?: device.name
                } catch (e: SecurityException) {
                    null
                }

                // Filter for gSENSOR devices or show all with names
                if (name != null) {
                    scannedDevices[device.address] = ScannedDevice(
                        device = device,
                        rssi = result.rssi,
                        name = name
                    )
                    trySend(scannedDevices.values.toList())
                }
            }

            override fun onScanFailed(errorCode: Int) {
                val errorMessage = when (errorCode) {
                    SCAN_FAILED_ALREADY_STARTED -> "Scan already started"
                    SCAN_FAILED_APPLICATION_REGISTRATION_FAILED -> "App registration failed"
                    SCAN_FAILED_INTERNAL_ERROR -> "Internal error"
                    SCAN_FAILED_FEATURE_UNSUPPORTED -> "Feature unsupported"
                    else -> "Unknown error: $errorCode"
                }
                close(Exception("Scan failed: $errorMessage"))
            }
        }

        scanner = bleScanner
        val scanSettings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()

        try {
            bleScanner.startScan(null, scanSettings, scanCallback)
        } catch (e: SecurityException) {
            close(Exception("Missing Bluetooth permission"))
            return@callbackFlow
        }

        awaitClose {
            try {
                bleScanner.stopScan(scanCallback)
            } catch (e: Exception) {
                // Ignore errors when stopping scan
            }
        }
    }

    /**
     * Stop scanning for devices.
     */
    fun stopScan() {
        // Scanner will be stopped when the flow is cancelled
    }

    /**
     * Connect to a gSENSOR device.
     */
    fun connect(device: BluetoothDevice) {
        _connectionState.value = ConnectionState.Connecting

        bluetoothGatt = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            device.connectGatt(context, false, gattCallback, BluetoothDevice.TRANSPORT_LE)
        } else {
            device.connectGatt(context, false, gattCallback)
        }
    }

    /**
     * Disconnect from the device.
     */
    fun disconnect() {
        bluetoothGatt?.disconnect()
    }

    /**
     * Send command to reset peak value.
     */
    fun resetPeak() {
        sendCommand(BleConstants.CMD_RESET_PEAK)
    }

    /**
     * Send command to reset filters.
     */
    fun resetFilters() {
        sendCommand(BleConstants.CMD_RESET_FILTERS)
    }

    private fun sendCommand(command: Byte) {
        val gatt = bluetoothGatt ?: return
        val service = gatt.getService(BleConstants.SERVICE_UUID) ?: return
        val characteristic = service.getCharacteristic(BleConstants.CHAR_CONTROL_UUID) ?: return

        characteristic.value = byteArrayOf(command)
        gatt.writeCharacteristic(characteristic)
    }

    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            when (newState) {
                BluetoothProfile.STATE_CONNECTED -> {
                    gatt.discoverServices()
                }
                BluetoothProfile.STATE_DISCONNECTED -> {
                    _connectionState.value = ConnectionState.Disconnected
                    bluetoothGatt?.close()
                    bluetoothGatt = null
                }
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                val service = gatt.getService(BleConstants.SERVICE_UUID)
                if (service != null) {
                    // Enable notifications for accelerometer data
                    enableNotifications(gatt, service.getCharacteristic(BleConstants.CHAR_ACCEL_UUID))
                    // Enable notifications for peak data
                    enableNotifications(gatt, service.getCharacteristic(BleConstants.CHAR_PEAK_UUID))

                    _connectionState.value = ConnectionState.Connected
                } else {
                    _connectionState.value = ConnectionState.Error("gSENSOR service not found")
                }
            } else {
                _connectionState.value = ConnectionState.Error("Service discovery failed")
            }
        }

        override fun onCharacteristicChanged(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic
        ) {
            val data = characteristic.value ?: return

            when (characteristic.uuid) {
                BleConstants.CHAR_ACCEL_UUID -> {
                    AccelData.fromByteArray(data)?.let { accelData ->
                        _accelData.tryEmit(accelData)
                    }
                }
                BleConstants.CHAR_PEAK_UUID -> {
                    PeakData.fromByteArray(data)?.let { peakData ->
                        _peakData.tryEmit(peakData)
                    }
                }
            }
        }
    }

    private fun enableNotifications(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic?) {
        if (characteristic == null) return

        gatt.setCharacteristicNotification(characteristic, true)

        val descriptor = characteristic.getDescriptor(BleConstants.CCCD_UUID)
        descriptor?.let {
            it.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
            gatt.writeDescriptor(it)
        }
    }

    /**
     * Clean up resources.
     */
    fun close() {
        bluetoothGatt?.close()
        bluetoothGatt = null
    }
}
