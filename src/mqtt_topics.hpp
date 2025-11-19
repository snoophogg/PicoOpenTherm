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
        constexpr const char *RESTART = "restart"; // Command topic for restart

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

        // Configuration / Settings
        constexpr const char *UPDATE_INTERVAL = "update_interval";
    }

    namespace MQTTDiscovery
    {
        // JSON keys
        constexpr const char *JSON_NAME = "name";
        constexpr const char *JSON_OBJECT_ID = "object_id";
        constexpr const char *JSON_UNIQUE_ID = "unique_id";
        constexpr const char *JSON_DEFAULT_ENTITY_ID = "default_entity_id";
        constexpr const char *JSON_STATE_TOPIC = "state_topic";
        constexpr const char *JSON_COMMAND_TOPIC = "command_topic";
        constexpr const char *JSON_DEVICE_CLASS = "device_class";
        constexpr const char *JSON_UNIT_OF_MEASUREMENT = "unit_of_measurement";
        constexpr const char *JSON_ICON = "icon";
        constexpr const char *JSON_VALUE_TEMPLATE = "value_template";
        constexpr const char *JSON_MIN = "min";
        constexpr const char *JSON_MAX = "max";
        constexpr const char *JSON_STEP = "step";
        constexpr const char *JSON_MODE = "mode";
        constexpr const char *JSON_DEVICE = "device";
        constexpr const char *JSON_IDENTIFIERS = "identifiers";
        constexpr const char *JSON_MODEL = "model";
        constexpr const char *JSON_MANUFACTURER = "manufacturer";

        // Component types
        constexpr const char *COMPONENT_BINARY_SENSOR = "binary_sensor";
        constexpr const char *COMPONENT_SENSOR = "sensor";
        constexpr const char *COMPONENT_SWITCH = "switch";
        constexpr const char *COMPONENT_NUMBER = "number";
        constexpr const char *COMPONENT_TEXT = "text";
        constexpr const char *COMPONENT_BUTTON = "button";

        // Device classes
        constexpr const char *DEVICE_CLASS_PROBLEM = "problem";
        constexpr const char *DEVICE_CLASS_HEAT = "heat";
        constexpr const char *DEVICE_CLASS_COLD = "cold";
        constexpr const char *DEVICE_CLASS_TEMPERATURE = "temperature";
        constexpr const char *DEVICE_CLASS_PRESSURE = "pressure";
        constexpr const char *DEVICE_CLASS_DURATION = "duration";
        constexpr const char *DEVICE_CLASS_SWITCH = "switch";
        constexpr const char *DEVICE_CLASS_SIGNAL_STRENGTH = "signal_strength";
        constexpr const char *DEVICE_CLASS_DATA_SIZE = "data_size";

        // Units of measurement
        constexpr const char *UNIT_CELSIUS = "°C";
        constexpr const char *UNIT_BAR = "bar";
        constexpr const char *UNIT_LITERS_PER_MIN = "l/min";
        constexpr const char *UNIT_STARTS = "starts";
        constexpr const char *UNIT_HOURS = "h";
        constexpr const char *UNIT_PERCENT = "%";
        constexpr const char *UNIT_DBM = "dBm";
        constexpr const char *UNIT_SECONDS = "s";
        constexpr const char *UNIT_BYTES = "B";
        constexpr const char *UNIT_MS = "ms";

        // Icons
        constexpr const char *ICON_ALERT_CIRCLE = "mdi:alert-circle";
        constexpr const char *ICON_RADIATOR = "mdi:radiator";
        constexpr const char *ICON_WATER_BOILER = "mdi:water-boiler";
        constexpr const char *ICON_FIRE = "mdi:fire";
        constexpr const char *ICON_SNOWFLAKE = "mdi:snowflake";
        constexpr const char *ICON_WRENCH = "mdi:wrench";
        constexpr const char *ICON_THERMOMETER = "mdi:thermometer";
        constexpr const char *ICON_HOME_THERMOMETER = "mdi:home-thermometer";
        constexpr const char *ICON_THERMOMETER_LINES = "mdi:thermometer-lines";
        constexpr const char *ICON_HOME_THERMOMETER_OUTLINE = "mdi:home-thermometer-outline";
        constexpr const char *ICON_WATER_THERMOMETER_OUTLINE = "mdi:water-thermometer-outline";
        constexpr const char *ICON_THERMOMETER_HIGH = "mdi:thermometer-high";
        constexpr const char *ICON_THERMOMETER_LOW = "mdi:thermometer-low";
        constexpr const char *ICON_PERCENT = "mdi:percent";
        constexpr const char *ICON_GAUGE = "mdi:gauge";
        constexpr const char *ICON_WATER_PUMP = "mdi:water-pump";
        constexpr const char *ICON_COUNTER = "mdi:counter";
        constexpr const char *ICON_CLOCK_OUTLINE = "mdi:clock-outline";
        constexpr const char *ICON_ALERT_OCTAGON = "mdi:alert-octagon";
        constexpr const char *ICON_INFORMATION = "mdi:information";
        constexpr const char *ICON_TAG_TEXT = "mdi:tag-text";
        constexpr const char *ICON_IDENTIFIER = "mdi:identifier";
        constexpr const char *ICON_PIN = "mdi:pin";
        constexpr const char *ICON_TIMER = "mdi:timer-cog";
        constexpr const char *ICON_CALENDAR = "mdi:calendar";
        constexpr const char *ICON_CALENDAR_TODAY = "mdi:calendar-today";
        constexpr const char *ICON_CLOCK_SYNC = "mdi:clock-sync";
        constexpr const char *ICON_RESTART = "mdi:restart";
        constexpr const char *ICON_WIFI = "mdi:wifi";
        constexpr const char *ICON_WIFI_CHECK = "mdi:wifi-check";
        constexpr const char *ICON_IP_NETWORK = "mdi:ip-network";
        constexpr const char *ICON_WIFI_MARKER = "mdi:wifi-marker";
        constexpr const char *ICON_CLOCK_START = "mdi:clock-start";
        constexpr const char *ICON_MEMORY = "mdi:memory";

        // Display names
        constexpr const char *NAME_FAULT = "Fault";
        constexpr const char *NAME_CH_MODE = "Central Heating Mode";
        constexpr const char *NAME_DHW_MODE = "Hot Water Mode";
        constexpr const char *NAME_FLAME = "Flame Status";
        constexpr const char *NAME_COOLING = "Cooling Active";
        constexpr const char *NAME_DIAGNOSTIC = "Diagnostic Mode";
        constexpr const char *NAME_CH_ENABLE = "Central Heating Enable";
        constexpr const char *NAME_DHW_ENABLE = "Hot Water Enable";
        constexpr const char *NAME_BOILER_TEMP = "Boiler Temperature";
        constexpr const char *NAME_DHW_TEMP = "Hot Water Temperature";
        constexpr const char *NAME_RETURN_TEMP = "Return Water Temperature";
        constexpr const char *NAME_OUTSIDE_TEMP = "Outside Temperature";
        constexpr const char *NAME_ROOM_TEMP = "Room Temperature";
        constexpr const char *NAME_EXHAUST_TEMP = "Exhaust Temperature";
        constexpr const char *NAME_CONTROL_SETPOINT = "Control Setpoint";
        constexpr const char *NAME_ROOM_SETPOINT = "Room Setpoint";
        constexpr const char *NAME_DHW_SETPOINT = "Hot Water Setpoint";
        constexpr const char *NAME_MAX_CH_SETPOINT = "Max CH Setpoint";
        constexpr const char *NAME_MODULATION = "Modulation Level";
        constexpr const char *NAME_MAX_MODULATION = "Max Modulation Level";
        constexpr const char *NAME_PRESSURE = "CH Water Pressure";
        constexpr const char *NAME_DHW_FLOW = "Hot Water Flow Rate";
        constexpr const char *NAME_BURNER_STARTS = "Burner Starts";
        constexpr const char *NAME_CH_PUMP_STARTS = "CH Pump Starts";
        constexpr const char *NAME_DHW_PUMP_STARTS = "DHW Pump Starts";
        constexpr const char *NAME_BURNER_HOURS = "Burner Operating Hours";
        constexpr const char *NAME_CH_PUMP_HOURS = "CH Pump Operating Hours";
        constexpr const char *NAME_DHW_PUMP_HOURS = "DHW Pump Operating Hours";
        constexpr const char *NAME_FAULT_CODE = "Fault Code";
        constexpr const char *NAME_DIAGNOSTIC_CODE = "Diagnostic Code";
        constexpr const char *NAME_DHW_PRESENT = "DHW Present";
        constexpr const char *NAME_COOLING_SUPPORTED = "Cooling Supported";
        constexpr const char *NAME_CH2_PRESENT = "CH2 Present";
        constexpr const char *NAME_OPENTHERM_VERSION = "OpenTherm Version";
        constexpr const char *NAME_DEVICE_NAME = "Device Name";
        constexpr const char *NAME_DEVICE_ID = "Device ID";
        constexpr const char *NAME_OPENTHERM_TX_PIN = "OpenTherm TX Pin";
        constexpr const char *NAME_OPENTHERM_RX_PIN = "OpenTherm RX Pin";
        constexpr const char *NAME_UPDATE_INTERVAL = "Update Interval";
        constexpr const char *NAME_DAY_OF_WEEK = "Day of Week";
        constexpr const char *NAME_TIME_OF_DAY = "Time of Day";
        constexpr const char *NAME_DATE = "Date";
        constexpr const char *NAME_YEAR = "Year";
        constexpr const char *NAME_SYNC_TIME = "Sync Time to Boiler";
        constexpr const char *NAME_RESTART = "Restart Gateway";
        constexpr const char *NAME_DHW_SETPOINT_MIN = "DHW Setpoint Min";
        constexpr const char *NAME_DHW_SETPOINT_MAX = "DHW Setpoint Max";
        constexpr const char *NAME_CH_SETPOINT_MIN = "CH Setpoint Min";
        constexpr const char *NAME_CH_SETPOINT_MAX = "CH Setpoint Max";
        constexpr const char *NAME_WIFI_RSSI = "WiFi Signal Strength";
        constexpr const char *NAME_WIFI_LINK_STATUS = "WiFi Link Status";
        constexpr const char *NAME_IP_ADDRESS = "IP Address";
        constexpr const char *NAME_WIFI_SSID = "WiFi SSID";
        constexpr const char *NAME_UPTIME = "Uptime";
        constexpr const char *NAME_FREE_HEAP = "Free Heap Memory";

        // Device information
        constexpr const char *DEVICE_MODEL = "OpenTherm Gateway";
        constexpr const char *DEVICE_MANUFACTURER = "PicoOpenTherm";

        // Other constants
        constexpr const char *MODE_BOX = "box";
        constexpr const char *CONFIG_SUFFIX = "/config";
    }
}

#endif // MQTT_TOPICS_HPP
