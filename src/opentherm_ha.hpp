#ifndef OPENTHERM_HA_HPP
#define OPENTHERM_HA_HPP

#include "opentherm_base.hpp"
#include <string>
#include <functional>

// Forward declaration for MQTT callback
namespace MQTT
{
    typedef std::function<void(const char *topic, const char *payload)> MessageCallback;
}

namespace OpenTherm
{
    namespace HomeAssistant
    {

        // Configuration structure
        struct Config
        {
            const char *device_name;        // e.g., "OpenTherm Gateway"
            const char *device_id;          // e.g., "opentherm_gw"
            const char *mqtt_prefix;        // e.g., "homeassistant"
            const char *topic_base;         // e.g., "opentherm"
            const char *state_topic_base;   // e.g., "state"
            const char *command_topic_base; // e.g., "cmd"
            bool auto_discovery;            // Enable MQTT auto-discovery
            uint32_t update_interval_ms;    // How often to poll sensors (default: 60000ms)
        };

        // Entity types
        enum EntityType
        {
            ENTITY_SENSOR,
            ENTITY_BINARY_SENSOR,
            ENTITY_SWITCH,
            ENTITY_NUMBER,
            ENTITY_SELECT
        };

        // MQTT interface callbacks
        struct MQTTCallbacks
        {
            std::function<bool(const char *topic, const char *payload, bool retain)> publish;
            std::function<bool(const char *topic)> subscribe;
        };

        class HAInterface
        {
        public:
            HAInterface(OpenTherm::BaseInterface &ot_interface, const Config &config);

            // Initialize Home Assistant MQTT discovery
            void begin(const MQTTCallbacks &callbacks);

            // Publish all MQTT discovery configs
            void publishDiscoveryConfigs();

            // Main update loop - call periodically
            void update();

            // Handle incoming MQTT messages
            void handleMessage(const char *topic, const char *payload);

            // Manual sensor updates
            void publishStatus();
            void publishTemperatures();
            void publishPressureFlow();
            void publishModulation();
            void publishCounters();
            void publishConfiguration();
            void publishFaults();
            void publishTimeDate();
            void publishTemperatureBounds();
            void publishWiFiStats();

            // Control functions
            bool setControlSetpoint(float temperature);
            bool setRoomSetpoint(float temperature);
            bool setDHWSetpoint(float temperature);
            bool setMaxCHSetpoint(float temperature);
            bool setCHEnable(bool enable);
            bool setDHWEnable(bool enable);

            // Time/Date synchronization - sync time to boiler from ISO 8601 format or epoch
            bool syncTimeToBoiler(const char *iso8601_time);
            bool syncTimeToBoiler(uint32_t unix_timestamp);

            // Configuration functions
            bool setDeviceName(const char *name);
            bool setDeviceID(const char *id);
            bool setOpenThermTxPin(uint8_t pin);
            bool setOpenThermRxPin(uint8_t pin);
            bool setUpdateInterval(uint32_t interval_ms);
            uint32_t getUpdateInterval() const;
            void publishDeviceConfiguration();

            // Metrics functions
            void publishOpenThermMetrics();

        private:
            OpenTherm::BaseInterface &ot_;
            Config config_;
            MQTTCallbacks mqtt_;
            uint32_t last_update_;

            // State tracking
            opentherm_status_t last_status_;
            bool status_valid_;

            // OpenTherm operation metrics
            struct {
                uint32_t total_requests;
                uint32_t failed_requests;
                uint32_t timeout_errors;
                uint32_t invalid_response_errors;
                uint32_t last_error_time_ms;
                char last_error_entity[64];
            } ot_metrics_;

            // Helper functions for MQTT discovery
            // Note: discovery helpers moved to OpenTherm::Discovery.
            // Local publish helpers (delegate to Discovery) remain as member functions
            void publishSensor(const char *topic_suffix, float value);
            void publishSensor(const char *topic_suffix, int value);
            void publishSensor(const char *topic_suffix, const char *value);
            void publishBinarySensor(const char *topic_suffix, bool value);

            // Track OpenTherm operation results for metrics
            void trackOTOperation(const char *entity_name, bool success);
        };

    } // namespace HomeAssistant
} // namespace OpenTherm

#endif // OPENTHERM_HA_HPP
