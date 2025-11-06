#include "config.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>

extern "C"
{
#define delete delete_func // Workaround for C++ keyword conflict
#include "kvstore.h"
#undef delete
}

#include "hardware/flash.h"
#include "hardware/sync.h"

namespace Config
{
    bool init()
    {
        // Initialize kvstore
        if (!kvs_init())
        {
            printf("Failed to initialize kvstore\n");
            return false;
        }

        printf("Configuration storage initialized\n");

        // Check if this is first boot - if no WiFi SSID configured, set defaults
        char test_buffer[64];
        int rc = kvs_get_str(KEY_WIFI_SSID, test_buffer, sizeof(test_buffer));
        if (rc != KVSTORE_SUCCESS)
        {
            printf("First boot detected, initializing with default configuration\n");
            resetToDefaults();
        }

        return true;
    }

    bool getWiFiSSID(char *buffer, size_t buffer_size)
    {
        int rc = kvs_get_str(KEY_WIFI_SSID, buffer, buffer_size);
        if (rc == KVSTORE_SUCCESS)
        {
            return true;
        }

        // Return default if not found
        strncpy(buffer, DEFAULT_WIFI_SSID, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return false;
    }

    bool getWiFiPassword(char *buffer, size_t buffer_size)
    {
        int rc = kvs_get_str(KEY_WIFI_PASSWORD, buffer, buffer_size);
        if (rc == KVSTORE_SUCCESS)
        {
            return true;
        }

        // Return default if not found
        strncpy(buffer, DEFAULT_WIFI_PASSWORD, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return false;
    }

    bool setWiFiSSID(const char *ssid)
    {
        return kvs_set(KEY_WIFI_SSID, ssid, strlen(ssid) + 1) == KVSTORE_SUCCESS;
    }

    bool setWiFiPassword(const char *password)
    {
        return kvs_set(KEY_WIFI_PASSWORD, password, strlen(password) + 1) == KVSTORE_SUCCESS;
    }

    bool getMQTTServerIP(char *buffer, size_t buffer_size)
    {
        int rc = kvs_get_str(KEY_MQTT_SERVER_IP, buffer, buffer_size);
        if (rc == KVSTORE_SUCCESS)
        {
            return true;
        }

        // Return default if not found
        strncpy(buffer, DEFAULT_MQTT_SERVER_IP, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return false;
    }

    uint16_t getMQTTServerPort()
    {
        char buffer[16];
        int rc = kvs_get_str(KEY_MQTT_SERVER_PORT, buffer, sizeof(buffer));
        if (rc == KVSTORE_SUCCESS)
        {
            return (uint16_t)atoi(buffer);
        }

        return DEFAULT_MQTT_SERVER_PORT;
    }

    bool getMQTTClientID(char *buffer, size_t buffer_size)
    {
        int rc = kvs_get_str(KEY_MQTT_CLIENT_ID, buffer, buffer_size);
        if (rc == KVSTORE_SUCCESS)
        {
            return true;
        }

        // Return default if not found
        strncpy(buffer, DEFAULT_MQTT_CLIENT_ID, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return false;
    }

    bool setMQTTServerIP(const char *ip)
    {
        return kvs_set(KEY_MQTT_SERVER_IP, ip, strlen(ip) + 1) == KVSTORE_SUCCESS;
    }

    bool setMQTTServerPort(uint16_t port)
    {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%u", port);
        return kvs_set(KEY_MQTT_SERVER_PORT, buffer, strlen(buffer) + 1) == KVSTORE_SUCCESS;
    }

    bool setMQTTClientID(const char *client_id)
    {
        return kvs_set(KEY_MQTT_CLIENT_ID, client_id, strlen(client_id) + 1) == KVSTORE_SUCCESS;
    }

    bool getDeviceName(char *buffer, size_t buffer_size)
    {
        int rc = kvs_get_str(KEY_DEVICE_NAME, buffer, buffer_size);
        if (rc == KVSTORE_SUCCESS)
        {
            return true;
        }

        // Return default if not found
        strncpy(buffer, DEFAULT_DEVICE_NAME, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return false;
    }

    bool getDeviceID(char *buffer, size_t buffer_size)
    {
        int rc = kvs_get_str(KEY_DEVICE_ID, buffer, buffer_size);
        if (rc == KVSTORE_SUCCESS)
        {
            return true;
        }

        // Return default if not found
        strncpy(buffer, DEFAULT_DEVICE_ID, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return false;
    }

    bool setDeviceName(const char *name)
    {
        return kvs_set(KEY_DEVICE_NAME, name, strlen(name) + 1) == KVSTORE_SUCCESS;
    }

    bool setDeviceID(const char *id)
    {
        return kvs_set(KEY_DEVICE_ID, id, strlen(id) + 1) == KVSTORE_SUCCESS;
    }

    uint8_t getOpenThermTxPin()
    {
        char buffer[16];
        int rc = kvs_get_str(KEY_OPENTHERM_TX_PIN, buffer, sizeof(buffer));
        if (rc == KVSTORE_SUCCESS)
        {
            return (uint8_t)atoi(buffer);
        }

        return DEFAULT_OPENTHERM_TX_PIN;
    }

    uint8_t getOpenThermRxPin()
    {
        char buffer[16];
        int rc = kvs_get_str(KEY_OPENTHERM_RX_PIN, buffer, sizeof(buffer));
        if (rc == KVSTORE_SUCCESS)
        {
            return (uint8_t)atoi(buffer);
        }

        return DEFAULT_OPENTHERM_RX_PIN;
    }

    bool setOpenThermTxPin(uint8_t pin)
    {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%u", pin);
        return kvs_set(KEY_OPENTHERM_TX_PIN, buffer, strlen(buffer) + 1) == KVSTORE_SUCCESS;
    }

    bool setOpenThermRxPin(uint8_t pin)
    {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%u", pin);
        return kvs_set(KEY_OPENTHERM_RX_PIN, buffer, strlen(buffer) + 1) == KVSTORE_SUCCESS;
    }

    bool resetToDefaults()
    {
        printf("Resetting configuration to defaults...\n");

        // WiFi defaults
        setWiFiSSID(DEFAULT_WIFI_SSID);
        setWiFiPassword(DEFAULT_WIFI_PASSWORD);

        // MQTT defaults
        setMQTTServerIP(DEFAULT_MQTT_SERVER_IP);
        setMQTTServerPort(DEFAULT_MQTT_SERVER_PORT);
        setMQTTClientID(DEFAULT_MQTT_CLIENT_ID);

        // Device defaults
        setDeviceName(DEFAULT_DEVICE_NAME);
        setDeviceID(DEFAULT_DEVICE_ID);

        // OpenTherm defaults
        setOpenThermTxPin(DEFAULT_OPENTHERM_TX_PIN);
        setOpenThermRxPin(DEFAULT_OPENTHERM_RX_PIN);

        printf("Configuration reset complete\n");
        return true;
    }

    void printConfig()
    {
        char buffer[128];

        printf("\n=== Current Configuration ===\n");

        printf("WiFi:\n");
        getWiFiSSID(buffer, sizeof(buffer));
        printf("  SSID: %s\n", buffer);
        getWiFiPassword(buffer, sizeof(buffer));
        printf("  Password: %s\n", buffer[0] ? "***" : "(not set)");

        printf("MQTT:\n");
        getMQTTServerIP(buffer, sizeof(buffer));
        printf("  Server IP: %s\n", buffer);
        printf("  Server Port: %u\n", getMQTTServerPort());
        getMQTTClientID(buffer, sizeof(buffer));
        printf("  Client ID: %s\n", buffer);

        printf("Device:\n");
        getDeviceName(buffer, sizeof(buffer));
        printf("  Name: %s\n", buffer);
        getDeviceID(buffer, sizeof(buffer));
        printf("  ID: %s\n", buffer);

        printf("OpenTherm:\n");
        printf("  TX Pin: GPIO%u\n", getOpenThermTxPin());
        printf("  RX Pin: GPIO%u\n", getOpenThermRxPin());

        printf("===========================\n\n");
    }

} // namespace Config
