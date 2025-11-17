// Compact topic suffix constants for Home Assistant discovery/state topics
#ifndef MQTT_TOPICS_HPP
#define MQTT_TOPICS_HPP

namespace OpenTherm
{
    namespace MQTTTopics
    {
        // Binary / status
        constexpr const char *FAULT = "fault";
        constexpr const char *CH_MODE = "ch_mode";
        constexpr const char *DHW_MODE = "dhw_mode";
        constexpr const char *FLAME = "flame";
        constexpr const char *COOLING = "cooling";
        constexpr const char *DIAGNOSTIC = "diagnostic";

        // Switches
        constexpr const char *CH_ENABLE = "ch_enable";
        constexpr const char *DHW_ENABLE = "dhw_enable";

        // Temperatures
        constexpr const char *BOILER_TEMP = "boiler_temp";
        constexpr const char *DHW_TEMP = "dhw_temp";
        constexpr const char *RETURN_TEMP = "return_temp";
        constexpr const char *OUTSIDE_TEMP = "outside_temp";
        constexpr const char *ROOM_TEMP = "room_temp";
        constexpr const char *EXHAUST_TEMP = "exhaust_temp";

        // Setpoints / numbers
        constexpr const char *CONTROL_SETPOINT = "control_setpoint";
        constexpr const char *ROOM_SETPOINT = "room_setpoint";
        constexpr const char *DHW_SETPOINT = "dhw_setpoint";
        constexpr const char *MAX_CH_SETPOINT = "max_ch_setpoint";

        // Modulation / pressure
        constexpr const char *MODULATION = "modulation";
        constexpr const char *MAX_MODULATION = "max_modulation";
        constexpr const char *PRESSURE = "pressure";
        constexpr const char *DHW_FLOW = "dhw_flow";

        // Counters / stats
        constexpr const char *BURNER_STARTS = "burner_starts";
        constexpr const char *CH_PUMP_STARTS = "ch_pump_starts";
        constexpr const char *DHW_PUMP_STARTS = "dhw_pump_starts";
        constexpr const char *BURNER_HOURS = "burner_hours";
        constexpr const char *CH_PUMP_HOURS = "ch_pump_hours";
        constexpr const char *DHW_PUMP_HOURS = "dhw_pump_hours";

        // Fault / diagnostic codes
        constexpr const char *FAULT_CODE = "fault_code";
        constexpr const char *DIAGNOSTIC_CODE = "diagnostic_code";

        // Presence / features
        constexpr const char *DHW_PRESENT = "dhw_present";
        constexpr const char *COOLING_SUPPORTED = "cooling_supported";
        constexpr const char *CH2_PRESENT = "ch2_present";

        // Meta / config
        constexpr const char *OPENTHERM_VERSION = "opentherm_version";
        constexpr const char *DEVICE_NAME = "device_name";
        constexpr const char *DEVICE_ID = "device_id";
        constexpr const char *OPENTHERM_TX_PIN = "opentherm_tx_pin";
        constexpr const char *OPENTHERM_RX_PIN = "opentherm_rx_pin";

        // Time/Date synchronization
        constexpr const char *DAY_OF_WEEK = "day_of_week";
        constexpr const char *TIME_OF_DAY = "time_of_day";
        constexpr const char *DATE = "date";
        constexpr const char *YEAR = "year";
        constexpr const char *SYNC_TIME = "sync_time"; // Command topic for time sync

        // Temperature bounds
        constexpr const char *DHW_SETPOINT_MIN = "dhw_setpoint_min";
        constexpr const char *DHW_SETPOINT_MAX = "dhw_setpoint_max";
        constexpr const char *CH_SETPOINT_MIN = "ch_setpoint_min";
        constexpr const char *CH_SETPOINT_MAX = "ch_setpoint_max";

        // WiFi statistics
        constexpr const char *WIFI_RSSI = "wifi_rssi";
        constexpr const char *WIFI_LINK_STATUS = "wifi_link_status";
        constexpr const char *IP_ADDRESS = "ip_address";
        constexpr const char *WIFI_SSID = "wifi_ssid";
        constexpr const char *UPTIME = "uptime";
        constexpr const char *FREE_HEAP = "free_heap";
    }
}

#endif // MQTT_TOPICS_HPP
