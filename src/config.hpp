#pragma once

#include <cstdint>
#include <string>

namespace Config
{
    // Configuration keys
    constexpr const char *KEY_WIFI_SSID = "wifi.ssid";
    constexpr const char *KEY_WIFI_PASSWORD = "wifi.password";
    constexpr const char *KEY_MQTT_SERVER_IP = "mqtt.server_ip";
    constexpr const char *KEY_MQTT_SERVER_PORT = "mqtt.server_port";
    constexpr const char *KEY_MQTT_CLIENT_ID = "mqtt.client_id";
    constexpr const char *KEY_DEVICE_NAME = "device.name";
    constexpr const char *KEY_DEVICE_ID = "device.id";
    constexpr const char *KEY_OPENTHERM_TX_PIN = "opentherm.tx_pin";
    constexpr const char *KEY_OPENTHERM_RX_PIN = "opentherm.rx_pin";

    // Default values
    constexpr const char *DEFAULT_WIFI_SSID = "your_wifi_ssid";
    constexpr const char *DEFAULT_WIFI_PASSWORD = "your_wifi_password";
    constexpr const char *DEFAULT_MQTT_SERVER_IP = "192.168.1.100";
    constexpr uint16_t DEFAULT_MQTT_SERVER_PORT = 1883;
    constexpr const char *DEFAULT_MQTT_CLIENT_ID = "pico_opentherm";
    constexpr const char *DEFAULT_DEVICE_NAME = "OpenTherm Gateway";
    constexpr const char *DEFAULT_DEVICE_ID = "opentherm_gw";
    constexpr uint8_t DEFAULT_OPENTHERM_TX_PIN = 16;
    constexpr uint8_t DEFAULT_OPENTHERM_RX_PIN = 17;

    // Initialize configuration system
    bool init();

    // WiFi configuration
    bool getWiFiSSID(char *buffer, size_t buffer_size);
    bool getWiFiPassword(char *buffer, size_t buffer_size);
    bool setWiFiSSID(const char *ssid);
    bool setWiFiPassword(const char *password);

    // MQTT configuration
    bool getMQTTServerIP(char *buffer, size_t buffer_size);
    uint16_t getMQTTServerPort();
    bool getMQTTClientID(char *buffer, size_t buffer_size);
    bool setMQTTServerIP(const char *ip);
    bool setMQTTServerPort(uint16_t port);
    bool setMQTTClientID(const char *client_id);

    // Device configuration
    bool getDeviceName(char *buffer, size_t buffer_size);
    bool getDeviceID(char *buffer, size_t buffer_size);
    bool setDeviceName(const char *name);
    bool setDeviceID(const char *id);

    // OpenTherm pin configuration
    uint8_t getOpenThermTxPin();
    uint8_t getOpenThermRxPin();
    bool setOpenThermTxPin(uint8_t pin);
    bool setOpenThermRxPin(uint8_t pin);

    // Reset to defaults
    bool resetToDefaults();

    // Print current configuration
    void printConfig();

} // namespace Config
