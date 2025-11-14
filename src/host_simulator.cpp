#include <mosquitto.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <thread>
#include <sstream>
#include <string>
#include <vector>
#include <map>

#include "simulated_opentherm.hpp"
#include "mqtt_topics.hpp"

using namespace OpenTherm;
using namespace OpenTherm::Simulator;

static SimulatedInterface sim_ot;
static std::string device_id = "opentherm_gw";
static struct mosquitto *mosq = nullptr;

void publish_retained(const std::string &topic, const std::string &payload)
{
    mosquitto_publish(mosq, nullptr, topic.c_str(), (int)payload.size(), payload.c_str(), 0, true);
}

void publish_now(const std::string &topic, const std::string &payload)
{
    mosquitto_publish(mosq, nullptr, topic.c_str(), (int)payload.size(), payload.c_str(), 0, false);
}

std::string make_discovery_payload(const std::string &device, const std::string &object_id, const std::string &component, const std::string &state_topic, const std::string &command_topic)
{
    std::ostringstream payload;
    payload << "{";
    payload << "\"name\":\"" << device << " " << object_id << "\",";
    payload << "\"unique_id\":\"" << device << "_" << object_id << "\",";
    payload << "\"state_topic\":\"" << state_topic << "\",";
    if (!command_topic.empty())
        payload << "\"command_topic\":\"" << command_topic << "\",";
    payload << "\"device\":{\"identifiers\":[\"" << device << "\"],\"name\":\"" << device << "\",\"model\":\"OpenTherm Gateway\",\"manufacturer\":\"PicoOpenTherm\"}";
    payload << "}";
    return payload.str();
}

void on_message(struct mosquitto *mosq_, void *userdata, const struct mosquitto_message *message)
{
    if (!message || !message->payload)
        return;
    std::string topic(message->topic);
    std::string payload((const char *)message->payload, message->payloadlen);
    if (topic.find("/room_setpoint") != std::string::npos)
    {
        try
        {
            float t = std::stof(payload);
            sim_ot.writeRoomSetpoint(t);
            printf("Host Simulator: room_setpoint -> %.2f\n", t);
        }
        catch (...) {}
    }
    else if (topic.find("/dhw_setpoint") != std::string::npos)
    {
        try
        {
            float t = std::stof(payload);
            sim_ot.writeDHWSetpoint(t);
            printf("Host Simulator: dhw_setpoint -> %.2f\n", t);
        }
        catch (...) {}
    }
}

int main(int argc, char **argv)
{
    const char *host = "mosquitto";
    int port = 1883;
    if (argc >= 2)
        host = argv[1];
    if (argc >= 3)
        port = atoi(argv[2]);
    if (argc >= 4)
        device_id = argv[3];

    mosquitto_lib_init();
    mosq = mosquitto_new(nullptr, true, nullptr);
    if (!mosq)
    {
        fprintf(stderr, "Failed to create mosquitto instance\n");
        return 1;
    }
    mosquitto_message_callback_set(mosq, on_message);
    if (mosquitto_connect(mosq, host, port, 60) != MOSQ_ERR_SUCCESS)
    {
        fprintf(stderr, "Failed to connect to broker %s:%d\n", host, port);
        return 2;
    }

    // Build and publish discovery payloads retained
    std::map<std::string, std::vector<std::string>> expected_map = {
        {"binary_sensor", {"fault", "ch_mode", "dhw_mode", "flame", "cooling", "diagnostic", "dhw_present", "cooling_supported", "ch2_present"}},
        {"switch", {"ch_enable", "dhw_enable"}},
        {"sensor", {"boiler_temp", "dhw_temp", "return_temp", "outside_temp", "room_temp", "exhaust_temp", "modulation", "max_modulation", "pressure", "dhw_flow", "burner_starts", "ch_pump_starts", "dhw_pump_starts", "burner_hours", "ch_pump_hours", "dhw_pump_hours", "fault_code", "diagnostic_code", "opentherm_version"}},
        {"number", {"control_setpoint", "room_setpoint", "dhw_setpoint", "max_ch_setpoint", "opentherm_tx_pin", "opentherm_rx_pin"}},
        {"text", {"device_name", "device_id"}}
    };

    std::string base_state = std::string("opentherm/state/") + device_id;
    std::string base_cmd = std::string("opentherm/cmd/") + device_id;

    // publish discovery
    for (auto &kv : expected_map)
    {
        const std::string &component = kv.first;
        for (const auto &oid : kv.second)
        {
            std::string state_topic = base_state + "/" + oid;
            std::string cmd_topic = (component == "switch" || component == "number" || component == "text") ? (base_cmd + "/" + oid) : std::string();
            std::string topic = std::string("homeassistant/") + component + "/" + device_id + "/" + oid + "/config";
            std::string payload = make_discovery_payload(device_id, oid, component, state_topic, cmd_topic);
            publish_retained(topic, payload);
        }
    }

    // Subscribe to command topics for setpoints
    std::string sub = base_cmd + "/#";
    mosquitto_subscribe(mosq, nullptr, sub.c_str(), 0);

    // Start loop thread
    mosquitto_loop_start(mosq);

    // publish initial state retained and then periodic updates
    auto last = std::chrono::steady_clock::now();
    while (true)
    {
        sim_ot.update();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last).count() >= 10)
        {
            // publish set of state topics
            publish_retained(base_state + "/room_temp", std::to_string(sim_ot.readRoomTemperature()));
            publish_retained(base_state + "/boiler_temp", std::to_string(sim_ot.readBoilerTemperature()));
            publish_retained(base_state + "/dhw_temp", std::to_string(sim_ot.readDHWTemperature()));
            publish_retained(base_state + "/return_temp", std::to_string(sim_ot.readReturnWaterTemperature()));
            publish_retained(base_state + "/outside_temp", std::to_string(sim_ot.readOutsideTemperature()));
            publish_retained(base_state + "/modulation", std::to_string(sim_ot.readModulationLevel()));
            publish_retained(base_state + "/max_modulation", std::to_string(sim_ot.readMaxModulationLevel()));
            publish_retained(base_state + "/pressure", std::to_string(sim_ot.readCHWaterPressure()));
            publish_retained(base_state + "/flame", sim_ot.readFlameStatus() ? "ON" : "OFF");
            publish_retained(base_state + "/fault", "OFF");
            publish_retained(base_state + "/room_setpoint", std::to_string(sim_ot.readRoomSetpoint()));
            publish_retained(base_state + "/dhw_setpoint", std::to_string(sim_ot.readDHWSetpoint()));

            last = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    mosquitto_loop_stop(mosq, true);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return 0;
}
