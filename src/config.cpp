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
        printf("Initializing configuration system...\n");

        // Initialize kvstore
        if (!kvs_init())
        {
            printf("ERROR: Failed to initialize kvstore\n");
            return false;
        }

        printf("KVStore initialized successfully\n");

        // Check if this is first boot - if no WiFi SSID configured, set defaults
        char test_buffer[64];
        int rc = kvs_get_str(KEY_WIFI_SSID, test_buffer, sizeof(test_buffer));
        printf("First boot check: kvs_get_str returned %d (%s)\n", rc, kvs_strerror(rc));

        if (rc != KVSTORE_SUCCESS)
        {
            printf("First boot detected or error reading config, initializing with default configuration\n");
            resetToDefaults();
        }
        else
        {
            printf("Found existing configuration: SSID='%s'\n", test_buffer);
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

    uint32_t getUpdateIntervalMs()
    {
        char buffer[16];
        int rc = kvs_get_str(KEY_UPDATE_INTERVAL_MS, buffer, sizeof(buffer));
        if (rc == KVSTORE_SUCCESS)
        {
            uint32_t interval = (uint32_t)atoi(buffer);
            // Validate range: 1 second to 5 minutes
            if (interval >= 1000 && interval <= 300000)
            {
                return interval;
            }
        }

        return DEFAULT_UPDATE_INTERVAL_MS;
    }

    bool setUpdateIntervalMs(uint32_t interval_ms)
    {
        // Validate range: 1 second to 5 minutes
        if (interval_ms < 1000 || interval_ms > 300000)
        {
            return false;
        }

        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%u", (unsigned int)interval_ms);
        return kvs_set(KEY_UPDATE_INTERVAL_MS, buffer, strlen(buffer) + 1) == KVSTORE_SUCCESS;
    }

    bool resetToDefaults()
    {
        printf("Resetting configuration to defaults...\n");

        // WiFi defaults
        printf("  Setting WiFi SSID: %s\n", DEFAULT_WIFI_SSID);
        if (!setWiFiSSID(DEFAULT_WIFI_SSID))
        {
            printf("  ERROR: Failed to set WiFi SSID\n");
            return false;
        }

        printf("  Setting WiFi password\n");
        if (!setWiFiPassword(DEFAULT_WIFI_PASSWORD))
        {
            printf("  ERROR: Failed to set WiFi password\n");
            return false;
        }

        // MQTT defaults
        printf("  Setting MQTT server: %s:%u\n", DEFAULT_MQTT_SERVER_IP, DEFAULT_MQTT_SERVER_PORT);
        if (!setMQTTServerIP(DEFAULT_MQTT_SERVER_IP))
        {
            printf("  ERROR: Failed to set MQTT server IP\n");
            return false;
        }

        if (!setMQTTServerPort(DEFAULT_MQTT_SERVER_PORT))
        {
            printf("  ERROR: Failed to set MQTT server port\n");
            return false;
        }

        if (!setMQTTClientID(DEFAULT_MQTT_CLIENT_ID))
        {
            printf("  ERROR: Failed to set MQTT client ID\n");
            return false;
        }

        // Device defaults
        if (!setDeviceName(DEFAULT_DEVICE_NAME))
        {
            printf("  ERROR: Failed to set device name\n");
            return false;
        }

        if (!setDeviceID(DEFAULT_DEVICE_ID))
        {
            printf("  ERROR: Failed to set device ID\n");
            return false;
        }

        // OpenTherm defaults
        if (!setOpenThermTxPin(DEFAULT_OPENTHERM_TX_PIN))
        {
            printf("  ERROR: Failed to set OpenTherm TX pin\n");
            return false;
        }

        if (!setOpenThermRxPin(DEFAULT_OPENTHERM_RX_PIN))
        {
            printf("  ERROR: Failed to set OpenTherm RX pin\n");
            return false;
        }

        // Update interval default
        if (!setUpdateIntervalMs(DEFAULT_UPDATE_INTERVAL_MS))
        {
            printf("  ERROR: Failed to set update interval\n");
            return false;
        }

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

        printf("Update:\n");
        printf("  Interval: %u ms (%.1f seconds)\n", getUpdateIntervalMs(), getUpdateIntervalMs() / 1000.0f);

        printf("===========================\n\n");
    }

} // namespace Config
