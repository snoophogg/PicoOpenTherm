#pragma once
#include <array>
#include <string>
#include <vector>

namespace OpenTherm { namespace DiscoveryList {
static const std::vector<std::string> BINARY_SENSOR = {
    std::string("fault"),
    std::string("ch_mode"),
    std::string("dhw_mode"),
    std::string("flame"),
    std::string("cooling"),
    std::string("diagnostic"),
    std::string("dhw_present"),
    std::string("cooling_supported"),
    std::string("ch2_present"),
};

static const std::vector<std::string> SWITCH = {
    std::string("ch_enable"),
    std::string("dhw_enable"),
};

static const std::vector<std::string> SENSOR = {
    std::string("boiler_temp"),
    std::string("dhw_temp"),
    std::string("return_temp"),
    std::string("outside_temp"),
    std::string("room_temp"),
    std::string("exhaust_temp"),
    std::string("modulation"),
    std::string("max_modulation"),
    std::string("pressure"),
    std::string("dhw_flow"),
    std::string("burner_starts"),
    std::string("ch_pump_starts"),
    std::string("dhw_pump_starts"),
    std::string("burner_hours"),
    std::string("ch_pump_hours"),
    std::string("dhw_pump_hours"),
    std::string("fault_code"),
    std::string("diagnostic_code"),
    std::string("opentherm_version"),
    std::string("ot_total_requests"),
    std::string("ot_failed_requests"),
    std::string("ot_success_rate"),
    std::string("ot_last_error_entity"),
    std::string("ot_time_since_error"),
};

static const std::vector<std::string> NUMBER = {
    std::string("control_setpoint"),
    std::string("room_setpoint"),
    std::string("dhw_setpoint"),
    std::string("max_ch_setpoint"),
    std::string("opentherm_tx_pin"),
    std::string("opentherm_rx_pin"),
};

static const std::vector<std::string> TEXT = {
    std::string("device_name"),
    std::string("device_id"),
};

static const std::vector<std::string> BUTTON = {
    std::string("sync_time"),
    std::string("restart"),
    std::string("republish_discovery"),
    std::string("republish_state"),
    std::string("force_republish_state"),
};

} } // namespace OpenTherm::DiscoveryList
