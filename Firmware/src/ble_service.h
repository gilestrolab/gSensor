/**
 * @file ble_service.h
 * @brief BLE GATT server for gSENSOR accelerometer data streaming
 *
 * Provides BLE connectivity using NimBLE library.
 * Streams accelerometer data via notifications to connected clients.
 */

#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <functional>
#include "config.h"
#include "signal_processing.h"

/**
 * @brief BLE GATT server for accelerometer data streaming
 *
 * Implements a custom GATT service with characteristics for:
 * - Accelerometer data (notify)
 * - Peak value (notify)
 * - Control commands (write)
 * - Configuration (read/write)
 */
class BleService : public NimBLEServerCallbacks, public NimBLECharacteristicCallbacks {
public:
    /**
     * @brief Command callback type
     *
     * Called when a control command is received via BLE
     */
    using CommandCallback = std::function<void(uint8_t cmd)>;

    /**
     * @brief Construct a new BLE Service
     */
    BleService();

    /**
     * @brief Initialize BLE and start advertising
     *
     * @param deviceName Name visible during BLE scanning
     * @return true if initialization successful
     */
    bool begin(const char* deviceName = BLE_DEVICE_NAME);

    /**
     * @brief Stop BLE advertising and disconnect
     */
    void stop();

    /**
     * @brief Enable or disable BLE at runtime
     *
     * When disabled, stops advertising and disconnects clients.
     * When re-enabled, reinitializes and starts advertising.
     *
     * @param enabled True to enable, false to disable
     */
    void setEnabled(bool enabled);

    /**
     * @brief Check if BLE is enabled
     *
     * @return true if enabled
     */
    bool isEnabled() const;

    /**
     * @brief Check if a client is connected
     *
     * @return true if connected
     */
    bool isConnected() const;

    /**
     * @brief Send accelerometer data notification
     *
     * Packs data into 20-byte binary format and sends via notify.
     * Only sends if a client is connected and subscribed.
     *
     * @param timestamp Timestamp in milliseconds
     * @param data Filtered accelerometer data
     * @param magnitude Calculated magnitude
     */
    void notifyAccelData(uint32_t timestamp, const AccelData& data, float magnitude);

    /**
     * @brief Send peak value notification
     *
     * @param timestamp Timestamp when peak occurred
     * @param peak Peak magnitude value
     */
    void notifyPeak(uint32_t timestamp, float peak);

    /**
     * @brief Set notification rate
     *
     * @param rateHz Rate in Hz (clamped to BLE_MIN/MAX_NOTIFY_RATE_HZ)
     */
    void setNotificationRate(uint8_t rateHz);

    /**
     * @brief Get current notification rate
     *
     * @return uint8_t Rate in Hz
     */
    uint8_t getNotificationRate() const;

    /**
     * @brief Register callback for control commands
     *
     * @param callback Function to call when command received
     */
    void setCommandCallback(CommandCallback callback);

    // NimBLEServerCallbacks overrides
    void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override;
    void onDisconnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override;

    // NimBLECharacteristicCallbacks overrides
    void onWrite(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc) override;
    void onRead(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc) override;

private:
    NimBLEServer* pServer_;
    NimBLEService* pService_;
    NimBLECharacteristic* pAccelChar_;
    NimBLECharacteristic* pPeakChar_;
    NimBLECharacteristic* pControlChar_;
    NimBLECharacteristic* pConfigChar_;
    NimBLEAdvertising* pAdvertising_;

    bool deviceConnected_;
    bool bleEnabled_;
    uint8_t notificationRateHz_;
    CommandCallback commandCallback_;

    /**
     * @brief Pack float into little-endian byte array
     */
    void packFloat(uint8_t* buffer, float value);

    /**
     * @brief Pack uint32_t into little-endian byte array
     */
    void packUint32(uint8_t* buffer, uint32_t value);
};

#endif // BLE_SERVICE_H
