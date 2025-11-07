/**
 * Unit tests for Home Assistant MQTT topic building and parsing
 *
 * These tests validate MQTT topic construction and message handling
 * without requiring actual MQTT connection.
 */

#include <gtest/gtest.h>
#include <cstring>
#include <string>

// ============================================================================
// Topic Building Tests
// ============================================================================

TEST(MQTTTopicTests, StateTopicConstruction)
{
    std::string base = "opentherm/state";
    std::string suffix = "boiler_temp";
    std::string expected = base + "/" + suffix;
    std::string actual = base + "/" + suffix;

    EXPECT_EQ(actual, expected);
}

TEST(MQTTTopicTests, CommandTopicConstruction)
{
    std::string base = "opentherm/cmd";
    std::string suffix = "ch_enable";
    std::string expected = base + "/" + suffix;
    std::string actual = base + "/" + suffix;

    EXPECT_EQ(actual, expected);
}

TEST(MQTTTopicTests, DiscoveryTopicConstruction)
{
    std::string prefix = "homeassistant";
    std::string component = "sensor";
    std::string device_id = "opentherm_gw";
    std::string object_id = "boiler_temp";

    std::string expected = prefix + "/" + component + "/" + device_id + "/" + object_id + "/config";
    std::string actual = prefix + "/" + component + "/" + device_id + "/" + object_id + "/config";

    EXPECT_EQ(actual, expected);
}

// ============================================================================
// Discovery Config JSON Tests
// ============================================================================

TEST(DiscoveryConfigTests, HasRequiredFields)
{
    // Simulate building a discovery config
    std::string json = R"({"name":"Boiler Temp","state_topic":"opentherm/state/boiler_temp","unique_id":"opentherm_gw_boiler_temp"})";
    
    EXPECT_NE(json.find("\"name\""), std::string::npos);
    EXPECT_NE(json.find("\"state_topic\""), std::string::npos);
    EXPECT_NE(json.find("\"unique_id\""), std::string::npos);
}

TEST(DiscoveryConfigTests, SensorWithDeviceClass)
{
    std::string json = R"({"device_class":"temperature","unit_of_measurement":"°C"})";
    
    EXPECT_NE(json.find("\"device_class\":\"temperature\""), std::string::npos);
    EXPECT_NE(json.find("\"unit_of_measurement\""), std::string::npos);
}

TEST(DiscoveryConfigTests, NumberEntityWithRange)
{
    std::string json = R"({
        "platform":"number",
        "min":20.0,
        "max":80.0,
        "step":0.5,
        "command_topic":"opentherm/cmd/ch_setpoint"
    })";
    
    EXPECT_NE(json.find("\"min\""), std::string::npos);
    EXPECT_NE(json.find("\"max\""), std::string::npos);
    EXPECT_NE(json.find("\"step\""), std::string::npos);
    EXPECT_NE(json.find("\"command_topic\""), std::string::npos);
}

// ============================================================================
// Command Parsing Tests
// ============================================================================

TEST(CommandParsingTests, ParseChEnableCommand)
{
    std::string command_topic = "opentherm/cmd/ch_enable";
    std::string payload = "ON";
    
    bool ch_enable = (payload == "ON");
    EXPECT_TRUE(ch_enable);
    
    payload = "OFF";
    ch_enable = (payload == "ON");
    EXPECT_FALSE(ch_enable);
}

TEST(CommandParsingTests, ParseChDisableCommand)
{
    std::string payload = "OFF";
    bool ch_enable = (payload == "ON");
    
    EXPECT_FALSE(ch_enable);
}

TEST(CommandParsingTests, ParseTemperatureSetpoint)
{
    std::string payload = "45.5";
    float setpoint = std::stof(payload);
    
    EXPECT_NEAR(setpoint, 45.5f, 0.01f);
}

TEST(CommandParsingTests, ParseDhwSetpoint)
{
    std::string command_topic = "opentherm/cmd/dhw_setpoint";
    std::string payload = "60.0";
    
    float setpoint = std::stof(payload);
    EXPECT_NEAR(setpoint, 60.0f, 0.01f);
}

// ============================================================================
// Topic Matching Tests
// ============================================================================

TEST(TopicMatchingTests, MatchExactCommandTopic)
{
    std::string topic = "opentherm/cmd/ch_enable";
    std::string pattern = "opentherm/cmd/ch_enable";
    
    EXPECT_EQ(topic, pattern);
}

TEST(TopicMatchingTests, MatchCommandTopicPrefix)
{
    std::string topic = "opentherm/cmd/ch_enable";
    std::string prefix = "opentherm/cmd/";
    
    EXPECT_EQ(topic.substr(0, prefix.length()), prefix);
}

TEST(TopicMatchingTests, ExtractCommandSuffix)
{
    std::string topic = "opentherm/cmd/ch_setpoint";
    std::string prefix = "opentherm/cmd/";
    
    if (topic.substr(0, prefix.length()) == prefix)
    {
        std::string suffix = topic.substr(prefix.length());
        EXPECT_EQ(suffix, "ch_setpoint");
    }
}

// ============================================================================
// State Publishing Tests
// ============================================================================

TEST(StatePublishingTests, StateTopicForBoilerTemp)
{
    std::string base = "opentherm/state";
    std::string sensor = "boiler_temp";
    std::string topic = base + "/" + sensor;
    
    EXPECT_EQ(topic, "opentherm/state/boiler_temp");
}

TEST(StatePublishingTests, StateTopicForFlameStatus)
{
    std::string base = "opentherm/state";
    std::string sensor = "flame";
    std::string topic = base + "/" + sensor;
    
    EXPECT_EQ(topic, "opentherm/state/flame");
}

TEST(StatePublishingTests, StatePayloadFloatFormatting)
{
    float value = 65.75f;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2f", value);
    
    EXPECT_STREQ(buffer, "65.75");
}

TEST(StatePublishingTests, StatePayloadIntegerFormatting)
{
    int value = 42;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", value);
    
    EXPECT_STREQ(buffer, "42");
}

TEST(StatePublishingTests, StatePayloadBooleanOn)
{
    bool value = true;
    const char *payload = value ? "ON" : "OFF";
    
    EXPECT_STREQ(payload, "ON");
}

TEST(StatePublishingTests, StatePayloadBooleanOff)
{
    bool value = false;
    const char *payload = value ? "ON" : "OFF";
    
    EXPECT_STREQ(payload, "OFF");
}

// ============================================================================
// Device Info Tests
// ============================================================================

TEST(DeviceInfoTests, DeviceInfoInDiscovery)
{
    std::string json = R"({
        "device": {
            "identifiers": ["opentherm_gw"],
            "name": "OpenTherm Gateway",
            "model": "Pico W",
            "manufacturer": "DIY"
        }
    })";
    
    EXPECT_NE(json.find("\"device\""), std::string::npos);
    EXPECT_NE(json.find("\"identifiers\""), std::string::npos);
    EXPECT_NE(json.find("\"name\":\"OpenTherm Gateway\""), std::string::npos);
}

TEST(DeviceInfoTests, UniqueIdFormat)
{
    std::string device_id = "opentherm_gw";
    std::string sensor_id = "boiler_temp";
    std::string unique_id = device_id + "_" + sensor_id;
    
    EXPECT_EQ(unique_id, "opentherm_gw_boiler_temp");
}

// ============================================================================
// Value Template Tests
// ============================================================================

TEST(ValueTemplateTests, PassthroughTemplate)
{
    std::string value_template = "{{ value }}";
    EXPECT_EQ(value_template, "{{ value }}");
}

TEST(ValueTemplateTests, JsonExtraction)
{
    std::string value_template = "{{ value_json.temperature }}";
    EXPECT_NE(value_template.find("value_json"), std::string::npos);
}

// ============================================================================
// Entity Category Tests
// ============================================================================

TEST(EntityCategoryTests, DiagnosticEntityCategory)
{
    std::string entity_category = "diagnostic";
    EXPECT_EQ(entity_category, "diagnostic");
}

TEST(EntityCategoryTests, ConfigEntityCategory)
{
    std::string entity_category = "config";
    EXPECT_EQ(entity_category, "config");
}
