/**
 * @file ble_service.cpp
 * @brief BLE GATT server implementation for gSENSOR
 */

#include "ble_service.h"

BleService::BleService()
    : pServer_(nullptr)
    , pService_(nullptr)
    , pAccelChar_(nullptr)
    , pPeakChar_(nullptr)
    , pControlChar_(nullptr)
    , pConfigChar_(nullptr)
    , pAdvertising_(nullptr)
    , deviceConnected_(false)
    , bleEnabled_(false)
    , notificationRateHz_(BLE_DEFAULT_NOTIFY_RATE_HZ)
    , commandCallback_(nullptr) {
}

bool BleService::begin(const char* deviceName) {
    if (DEBUG_ENABLED) {
        Serial.println("Initializing BLE...");
    }

    // Initialize NimBLE
    NimBLEDevice::init(deviceName);

    // Set power level (ESP32-C3 supports up to +20 dBm)
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // +9 dBm - good balance of range and power

    // Create GATT server
    pServer_ = NimBLEDevice::createServer();
    pServer_->setCallbacks(this);

    // Create service
    pService_ = pServer_->createService(BLE_SERVICE_UUID);

    // Create accelerometer data characteristic (read + notify)
    pAccelChar_ = pService_->createCharacteristic(
        BLE_CHAR_ACCEL_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    pAccelChar_->setCallbacks(this);

    // Create peak value characteristic (read + notify)
    pPeakChar_ = pService_->createCharacteristic(
        BLE_CHAR_PEAK_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    pPeakChar_->setCallbacks(this);

    // Create control characteristic (write)
    pControlChar_ = pService_->createCharacteristic(
        BLE_CHAR_CONTROL_UUID,
        NIMBLE_PROPERTY::WRITE
    );
    pControlChar_->setCallbacks(this);

    // Create configuration characteristic (read + write)
    pConfigChar_ = pService_->createCharacteristic(
        BLE_CHAR_CONFIG_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
    );
    pConfigChar_->setCallbacks(this);

    // Set initial config value
    pConfigChar_->setValue(&notificationRateHz_, 1);

    // Start service
    pService_->start();

    // Start advertising
    pAdvertising_ = NimBLEDevice::getAdvertising();
    pAdvertising_->addServiceUUID(BLE_SERVICE_UUID);
    pAdvertising_->setScanResponse(true);
    pAdvertising_->setMinPreferred(0x06);  // For iOS compatibility
    pAdvertising_->setMaxPreferred(0x12);
    pAdvertising_->start();

    if (DEBUG_ENABLED) {
        Serial.print("BLE advertising as: ");
        Serial.println(deviceName);
    }

    bleEnabled_ = true;
    return true;
}

void BleService::stop() {
    if (pAdvertising_) {
        pAdvertising_->stop();
    }
    NimBLEDevice::deinit(true);
    deviceConnected_ = false;
    bleEnabled_ = false;

    // Reset pointers after deinit
    pServer_ = nullptr;
    pService_ = nullptr;
    pAccelChar_ = nullptr;
    pPeakChar_ = nullptr;
    pControlChar_ = nullptr;
    pConfigChar_ = nullptr;
    pAdvertising_ = nullptr;

    if (DEBUG_ENABLED) {
        Serial.println("BLE stopped");
    }
}

void BleService::setEnabled(bool enabled) {
    if (enabled == bleEnabled_) {
        return;  // No change needed
    }

    if (enabled) {
        // Re-initialize BLE
        begin(BLE_DEVICE_NAME);
    } else {
        // Stop BLE
        stop();
    }
}

bool BleService::isEnabled() const {
    return bleEnabled_;
}

bool BleService::isConnected() const {
    return deviceConnected_;
}

void BleService::notifyAccelData(uint32_t timestamp, const AccelData& data, float magnitude) {
    if (!deviceConnected_ || !pAccelChar_) {
        return;
    }

    // Pack data into 20-byte binary format
    // Format: timestamp(4) + x(4) + y(4) + z(4) + magnitude(4) = 20 bytes
    uint8_t buffer[20];

    packUint32(&buffer[0], timestamp);
    packFloat(&buffer[4], data.x);
    packFloat(&buffer[8], data.y);
    packFloat(&buffer[12], data.z);
    packFloat(&buffer[16], magnitude);

    pAccelChar_->setValue(buffer, sizeof(buffer));
    pAccelChar_->notify();
}

void BleService::notifyPeak(uint32_t timestamp, float peak) {
    if (!deviceConnected_ || !pPeakChar_) {
        return;
    }

    // Pack peak data into 8-byte format
    uint8_t buffer[8];

    packUint32(&buffer[0], timestamp);
    packFloat(&buffer[4], peak);

    pPeakChar_->setValue(buffer, sizeof(buffer));
    pPeakChar_->notify();
}

void BleService::setNotificationRate(uint8_t rateHz) {
    // Clamp to valid range
    if (rateHz < BLE_MIN_NOTIFY_RATE_HZ) {
        rateHz = BLE_MIN_NOTIFY_RATE_HZ;
    } else if (rateHz > BLE_MAX_NOTIFY_RATE_HZ) {
        rateHz = BLE_MAX_NOTIFY_RATE_HZ;
    }

    notificationRateHz_ = rateHz;

    // Update characteristic value
    if (pConfigChar_) {
        pConfigChar_->setValue(&notificationRateHz_, 1);
    }

    if (DEBUG_ENABLED) {
        Serial.print("BLE notification rate set to: ");
        Serial.print(rateHz);
        Serial.println(" Hz");
    }
}

uint8_t BleService::getNotificationRate() const {
    return notificationRateHz_;
}

void BleService::setCommandCallback(CommandCallback callback) {
    commandCallback_ = callback;
}

// Server callbacks
void BleService::onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
    deviceConnected_ = true;

    if (DEBUG_ENABLED) {
        Serial.println("BLE client connected");
    }

    // Continue advertising to allow multiple connections (optional)
    // pAdvertising_->start();
}

void BleService::onDisconnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
    deviceConnected_ = false;

    if (DEBUG_ENABLED) {
        Serial.println("BLE client disconnected");
    }

    // Restart advertising
    pAdvertising_->start();
}

// Characteristic callbacks
void BleService::onWrite(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc) {
    std::string uuid = pCharacteristic->getUUID().toString();

    if (uuid == BLE_CHAR_CONTROL_UUID) {
        // Handle control commands
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            uint8_t cmd = value[0];

            if (DEBUG_ENABLED) {
                Serial.print("BLE command received: 0x");
                Serial.println(cmd, HEX);
            }

            if (commandCallback_) {
                commandCallback_(cmd);
            }
        }
    } else if (uuid == BLE_CHAR_CONFIG_UUID) {
        // Handle configuration updates
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            uint8_t newRate = value[0];
            setNotificationRate(newRate);
        }
    }
}

void BleService::onRead(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc) {
    // Optional: Log read operations
    if (DEBUG_ENABLED) {
        Serial.print("BLE characteristic read: ");
        Serial.println(pCharacteristic->getUUID().toString().c_str());
    }
}

// Helper functions for packing data (little-endian)
void BleService::packFloat(uint8_t* buffer, float value) {
    uint8_t* p = reinterpret_cast<uint8_t*>(&value);
    buffer[0] = p[0];
    buffer[1] = p[1];
    buffer[2] = p[2];
    buffer[3] = p[3];
}

void BleService::packUint32(uint8_t* buffer, uint32_t value) {
    buffer[0] = value & 0xFF;
    buffer[1] = (value >> 8) & 0xFF;
    buffer[2] = (value >> 16) & 0xFF;
    buffer[3] = (value >> 24) & 0xFF;
}
