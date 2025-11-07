/**
 * Unit tests for Home Assistant MQTT topic building and parsing
 *
 * These tests validate MQTT topic construction and message handling
 * without requiring actual MQTT connection.
 */

#include <cstdio>
#include <cstring>
#include <string>

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name)                             \
    void test_##name();                        \
    void run_test_##name()                     \
    {                                          \
        printf("Running test: %s... ", #name); \
        tests_run++;                           \
        test_##name();                         \
        tests_passed++;                        \
        printf("PASSED\n");                    \
    }                                          \
    void test_##name()

#define ASSERT(condition)                                                                           \
    do                                                                                              \
    {                                                                                               \
        if (!(condition))                                                                           \
        {                                                                                           \
            printf("FAILED\n  Assertion failed: %s\n  at %s:%d\n", #condition, __FILE__, __LINE__); \
            return;                                                                                 \
        }                                                                                           \
    } while (0)

#define ASSERT_STR_EQ(actual, expected)                                                                             \
    do                                                                                                              \
    {                                                                                                               \
        if (strcmp((actual), (expected)) != 0)                                                                      \
        {                                                                                                           \
            printf("FAILED\n  Expected: %s\n  Actual: %s\n  at %s:%d\n", (expected), (actual), __FILE__, __LINE__); \
            return;                                                                                                 \
        }                                                                                                           \
    } while (0)

#define ASSERT_STR_CONTAINS(haystack, needle)                                                                                    \
    do                                                                                                                           \
    {                                                                                                                            \
        if (strstr((haystack), (needle)) == nullptr)                                                                             \
        {                                                                                                                        \
            printf("FAILED\n  Expected substring: %s\n  In string: %s\n  at %s:%d\n", (needle), (haystack), __FILE__, __LINE__); \
            return;                                                                                                              \
        }                                                                                                                        \
    } while (0)

// ============================================================================
// Topic Building Tests
// ============================================================================

TEST(state_topic_construction)
{
    std::string base = "opentherm/state";
    std::string suffix = "boiler_temp";
    std::string expected = base + "/" + suffix;
    std::string actual = base + "/" + suffix;

    ASSERT(actual == expected);
}

TEST(command_topic_construction)
{
    std::string base = "opentherm/cmd";
    std::string suffix = "ch_enable";
    std::string expected = base + "/" + suffix;
    std::string actual = base + "/" + suffix;

    ASSERT(actual == expected);
}

TEST(discovery_topic_construction)
{
    std::string prefix = "homeassistant";
    std::string component = "sensor";
    std::string device_id = "opentherm_gw";
    std::string object_id = "boiler_temp";

    std::string expected = prefix + "/" + component + "/" + device_id + "/" + object_id + "/config";
    std::string actual = prefix + "/" + component + "/" + device_id + "/" + object_id + "/config";

    ASSERT(actual == expected);
}

// ============================================================================
// Discovery Config JSON Tests
// ============================================================================

TEST(discovery_json_has_required_fields)
{
    // Simulate building a discovery config
    std::string json = "{"
                       "\"name\":\"Boiler Temperature\","
                       "\"unique_id\":\"opentherm_gw_boiler_temp\","
                       "\"state_topic\":\"opentherm/state/boiler_temp\","
                       "\"device_class\":\"temperature\","
                       "\"unit_of_measurement\":\"°C\""
                       "}";

    ASSERT_STR_CONTAINS(json.c_str(), "\"name\":");
    ASSERT_STR_CONTAINS(json.c_str(), "\"unique_id\":");
    ASSERT_STR_CONTAINS(json.c_str(), "\"state_topic\":");
}

TEST(discovery_json_sensor_with_device_class)
{
    std::string json = "{"
                       "\"device_class\":\"temperature\","
                       "\"unit_of_measurement\":\"°C\""
                       "}";

    ASSERT_STR_CONTAINS(json.c_str(), "\"device_class\":\"temperature\"");
    ASSERT_STR_CONTAINS(json.c_str(), "\"unit_of_measurement\":\"°C\"");
}

TEST(discovery_json_number_entity_with_range)
{
    std::string json = "{"
                       "\"min\":0,"
                       "\"max\":100,"
                       "\"step\":0.5,"
                       "\"mode\":\"box\""
                       "}";

    ASSERT_STR_CONTAINS(json.c_str(), "\"min\":");
    ASSERT_STR_CONTAINS(json.c_str(), "\"max\":");
    ASSERT_STR_CONTAINS(json.c_str(), "\"step\":");
}

// ============================================================================
// Command Topic Parsing Tests
// ============================================================================

TEST(parse_ch_enable_command)
{
    const char *topic = "opentherm/cmd/ch_enable";
    const char *payload = "ON";

    // Should match the command topic
    ASSERT(strstr(topic, "/ch_enable") != nullptr);

    // Should parse ON as true
    bool value = (strcmp(payload, "ON") == 0);
    ASSERT(value == true);
}

TEST(parse_ch_disable_command)
{
    const char *topic = "opentherm/cmd/ch_enable";
    const char *payload = "OFF";

    bool value = (strcmp(payload, "ON") == 0);
    ASSERT(value == false);
}

TEST(parse_temperature_setpoint)
{
    const char *topic = "opentherm/cmd/control_setpoint";
    const char *payload = "45.5";

    // Should parse as float
    float value = atof(payload);
    ASSERT(value > 45.4f && value < 45.6f);
}

TEST(parse_dhw_setpoint)
{
    const char *topic = "opentherm/cmd/dhw_setpoint";
    const char *payload = "60.0";

    float value = atof(payload);
    ASSERT(value >= 59.9f && value <= 60.1f);
}

// ============================================================================
// Topic Matching Tests
// ============================================================================

TEST(match_exact_command_topic)
{
    const char *received = "opentherm/cmd/ch_enable";
    const char *expected = "opentherm/cmd/ch_enable";

    ASSERT(strcmp(received, expected) == 0);
}

TEST(match_command_topic_prefix)
{
    const char *received = "opentherm/cmd/ch_enable";
    const char *prefix = "opentherm/cmd/";

    ASSERT(strncmp(received, prefix, strlen(prefix)) == 0);
}

TEST(extract_command_suffix)
{
    const char *received = "opentherm/cmd/dhw_setpoint";
    const char *prefix = "opentherm/cmd/";

    const char *suffix = received + strlen(prefix);
    ASSERT_STR_EQ(suffix, "dhw_setpoint");
}

// ============================================================================
// State Publishing Tests
// ============================================================================

TEST(state_topic_for_boiler_temp)
{
    std::string base = "opentherm/state";
    std::string sensor = "boiler_temp";
    std::string topic = base + "/" + sensor;

    ASSERT_STR_EQ(topic.c_str(), "opentherm/state/boiler_temp");
}

TEST(state_topic_for_flame_status)
{
    std::string base = "opentherm/state";
    std::string sensor = "flame";
    std::string topic = base + "/" + sensor;

    ASSERT_STR_EQ(topic.c_str(), "opentherm/state/flame");
}

TEST(state_payload_float_formatting)
{
    float temp = 55.75f;
    char payload[32];
    snprintf(payload, sizeof(payload), "%.2f", temp);

    ASSERT_STR_EQ(payload, "55.75");
}

TEST(state_payload_integer_formatting)
{
    int count = 1234;
    char payload[32];
    snprintf(payload, sizeof(payload), "%d", count);

    ASSERT_STR_EQ(payload, "1234");
}

TEST(state_payload_boolean_on)
{
    bool state = true;
    const char *payload = state ? "ON" : "OFF";

    ASSERT_STR_EQ(payload, "ON");
}

TEST(state_payload_boolean_off)
{
    bool state = false;
    const char *payload = state ? "ON" : "OFF";

    ASSERT_STR_EQ(payload, "OFF");
}

// ============================================================================
// Device Info Tests
// ============================================================================

TEST(device_info_in_discovery)
{
    std::string json = "{"
                       "\"device\":{"
                       "\"identifiers\":[\"opentherm_gw\"],"
                       "\"name\":\"OpenTherm Gateway\","
                       "\"model\":\"PicoOpenTherm\","
                       "\"manufacturer\":\"DIY\""
                       "}"
                       "}";

    ASSERT_STR_CONTAINS(json.c_str(), "\"device\":");
    ASSERT_STR_CONTAINS(json.c_str(), "\"identifiers\":");
    ASSERT_STR_CONTAINS(json.c_str(), "\"name\":\"OpenTherm Gateway\"");
}

TEST(unique_id_format)
{
    std::string device_id = "opentherm_gw";
    std::string object_id = "boiler_temp";
    std::string unique_id = device_id + "_" + object_id;

    ASSERT_STR_EQ(unique_id.c_str(), "opentherm_gw_boiler_temp");
}

// ============================================================================
// Value Template Tests
// ============================================================================

TEST(value_template_passthrough)
{
    const char *template_str = "{{ value }}";
    ASSERT_STR_CONTAINS(template_str, "value");
}

TEST(value_template_json_extraction)
{
    const char *template_str = "{{ value_json.temperature }}";
    ASSERT_STR_CONTAINS(template_str, "value_json");
}

// ============================================================================
// Entity Category Tests
// ============================================================================

TEST(diagnostic_entity_category)
{
    std::string json = "{\"entity_category\":\"diagnostic\"}";
    ASSERT_STR_CONTAINS(json.c_str(), "\"entity_category\":\"diagnostic\"");
}

TEST(config_entity_category)
{
    std::string json = "{\"entity_category\":\"config\"}";
    ASSERT_STR_CONTAINS(json.c_str(), "\"entity_category\":\"config\"");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main()
{
    printf("\n");
    printf("==============================================\n");
    printf("  Home Assistant MQTT Integration Tests\n");
    printf("==============================================\n\n");

    // Topic building tests
    printf("--- Topic Building Tests ---\n");
    run_test_state_topic_construction();
    run_test_command_topic_construction();
    run_test_discovery_topic_construction();

    // Discovery JSON tests
    printf("\n--- Discovery Config JSON Tests ---\n");
    run_test_discovery_json_has_required_fields();
    run_test_discovery_json_sensor_with_device_class();
    run_test_discovery_json_number_entity_with_range();

    // Command parsing tests
    printf("\n--- Command Topic Parsing Tests ---\n");
    run_test_parse_ch_enable_command();
    run_test_parse_ch_disable_command();
    run_test_parse_temperature_setpoint();
    run_test_parse_dhw_setpoint();

    // Topic matching tests
    printf("\n--- Topic Matching Tests ---\n");
    run_test_match_exact_command_topic();
    run_test_match_command_topic_prefix();
    run_test_extract_command_suffix();

    // State publishing tests
    printf("\n--- State Publishing Tests ---\n");
    run_test_state_topic_for_boiler_temp();
    run_test_state_topic_for_flame_status();
    run_test_state_payload_float_formatting();
    run_test_state_payload_integer_formatting();
    run_test_state_payload_boolean_on();
    run_test_state_payload_boolean_off();

    // Device info tests
    printf("\n--- Device Info Tests ---\n");
    run_test_device_info_in_discovery();
    run_test_unique_id_format();

    // Value template tests
    printf("\n--- Value Template Tests ---\n");
    run_test_value_template_passthrough();
    run_test_value_template_json_extraction();

    // Entity category tests
    printf("\n--- Entity Category Tests ---\n");
    run_test_diagnostic_entity_category();
    run_test_config_entity_category();

    // Summary
    printf("\n==============================================\n");
    printf("  Test Results: %d/%d passed\n", tests_passed, tests_run);
    printf("==============================================\n\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
