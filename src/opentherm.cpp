#include "opentherm.hpp"
#include "opentherm_protocol.hpp"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "opentherm_write.pio.h"
#include "opentherm_read.pio.h"
#include <cstdio>

// OpenTherm Interface Implementation
namespace OpenTherm
{

    Interface::Interface(unsigned int tx_pin, unsigned int rx_pin, PIO pio_tx, PIO pio_rx)
        : pio_tx_(pio_tx ? pio_tx : pio0),
          pio_rx_(pio_rx ? pio_rx : pio1),
          tx_pin_(tx_pin),
          rx_pin_(rx_pin),
          timeout_ms_(1000)
    { // Default 1 second timeout

        // Load and initialize TX PIO program
        uint offset_tx = pio_add_program(pio_tx_, &opentherm_tx_program);
        sm_tx_ = pio_claim_unused_sm(pio_tx_, true);
        opentherm_tx_program_init(pio_tx_, sm_tx_, offset_tx, tx_pin_);

        // Load and initialize RX PIO program
        uint offset_rx = pio_add_program(pio_rx_, &opentherm_rx_program);
        sm_rx_ = pio_claim_unused_sm(pio_rx_, true);
        opentherm_rx_program_init(pio_rx_, sm_rx_, offset_rx, rx_pin_);

        printf("OpenTherm initialized: TX=GPIO%u, RX=GPIO%u\n", tx_pin_, rx_pin_);
    }

    void Interface::send(uint32_t frame)
    {
        opentherm_tx_send_frame(pio_tx_, sm_tx_, frame);
    }

    bool Interface::receive(uint32_t &frame)
    {
        if (!opentherm_rx_available(pio_rx_, sm_rx_))
        {
            return false;
        }

        // Get raw Manchester-encoded data
        uint64_t raw_data = opentherm_rx_get_raw(pio_rx_, sm_rx_);

        // Decode Manchester encoding
        if (!opentherm_manchester_decode(raw_data, &frame))
        {
            printf("Manchester decode error\n");
            return false;
        }

        // Verify parity
        if (!opentherm_verify_parity(frame))
        {
            printf("Parity error\n");
            return false;
        }

        return true;
    }

    void Interface::printFrame(uint32_t frame_data)
    {
        opentherm_frame_t frame;
        opentherm_unpack_frame(frame_data, &frame);

        printf("Frame: 0x%08lX\n", frame_data);
        printf("  Parity: %u\n", frame.parity);
        printf("  MsgType: %u ", frame.msg_type);

        switch (frame.msg_type)
        {
        case OT_MSGTYPE_READ_DATA:
            printf("(READ-DATA)\n");
            break;
        case OT_MSGTYPE_WRITE_DATA:
            printf("(WRITE-DATA)\n");
            break;
        case OT_MSGTYPE_INVALID_DATA:
            printf("(INVALID-DATA)\n");
            break;
        case OT_MSGTYPE_READ_ACK:
            printf("(READ-ACK)\n");
            break;
        case OT_MSGTYPE_WRITE_ACK:
            printf("(WRITE-ACK)\n");
            break;
        case OT_MSGTYPE_DATA_INVALID:
            printf("(DATA-INVALID)\n");
            break;
        case OT_MSGTYPE_UNKNOWN_DATAID:
            printf("(UNKNOWN-DATAID)\n");
            break;
        default:
            printf("(RESERVED)\n");
            break;
        }

        printf("  DataID: %u\n", frame.data_id);
        printf("  DataValue: 0x%04X (%u)\n", frame.data_value, frame.data_value);

        // Decode common data values based on data ID
        switch (frame.data_id)
        {
        case OT_DATA_ID_STATUS:
        {
            opentherm_status_t status;
            opentherm_decode_status(frame.data_value, &status);
            printf("    -> Status Flags:\n");
            printf("       CH Enable: %d, DHW Enable: %d, Cooling: %d\n",
                   status.ch_enable, status.dhw_enable, status.cooling_enable);
            printf("       Fault: %d, CH Mode: %d, DHW Mode: %d, Flame: %d\n",
                   status.fault, status.ch_mode, status.dhw_mode, status.flame);
            break;
        }
        case OT_DATA_ID_CONTROL_SETPOINT:
        case OT_DATA_ID_ROOM_SETPOINT:
        case OT_DATA_ID_ROOM_SETPOINT_CH2:
        case OT_DATA_ID_BOILER_WATER_TEMP:
        case OT_DATA_ID_DHW_TEMP:
        case OT_DATA_ID_OUTSIDE_TEMP:
        case OT_DATA_ID_RETURN_WATER_TEMP:
        case OT_DATA_ID_SOLAR_STORAGE_TEMP:
        case OT_DATA_ID_SOLAR_COLL_TEMP:
        case OT_DATA_ID_FLOW_TEMP_CH2:
        case OT_DATA_ID_DHW2_TEMP:
        case OT_DATA_ID_DHW_SETPOINT:
        case OT_DATA_ID_MAX_CH_SETPOINT:
        case OT_DATA_ID_ROOM_TEMP:
        {
            float temp = opentherm_f8_8_to_float(frame.data_value);
            printf("    -> Temperature: %.2f°C\n", temp);
            break;
        }
        case OT_DATA_ID_EXHAUST_TEMP:
        {
            int16_t temp = opentherm_decode_s16(frame.data_value);
            printf("    -> Exhaust Temperature: %d°C\n", temp);
            break;
        }
        case OT_DATA_ID_REL_MOD_LEVEL:
        case OT_DATA_ID_MAX_REL_MOD:
        {
            float level = opentherm_f8_8_to_float(frame.data_value);
            printf("    -> Modulation Level: %.1f%%\n", level);
            break;
        }
        case OT_DATA_ID_CH_WATER_PRESS:
        {
            float pressure = opentherm_f8_8_to_float(frame.data_value);
            printf("    -> CH Water Pressure: %.2f bar\n", pressure);
            break;
        }
        case OT_DATA_ID_DHW_FLOW_RATE:
        {
            float flow = opentherm_f8_8_to_float(frame.data_value);
            printf("    -> DHW Flow Rate: %.2f l/min\n", flow);
            break;
        }
        case OT_DATA_ID_MASTER_CONFIG:
        case OT_DATA_ID_SLAVE_CONFIG:
        {
            opentherm_config_t config;
            if (frame.data_id == OT_DATA_ID_MASTER_CONFIG)
            {
                opentherm_decode_master_config(frame.data_value, &config);
                printf("    -> Master Config:\n");
            }
            else
            {
                opentherm_decode_slave_config(frame.data_value, &config);
                printf("    -> Slave Config:\n");
            }
            printf("       DHW Present: %d, Control Type: %d, Cooling: %d\n",
                   config.dhw_present, config.control_type, config.cooling_config);
            printf("       CH2 Present: %d, Pump Control: %d\n",
                   config.ch2_present, config.master_pump_control);
            break;
        }
        case OT_DATA_ID_FAULT_FLAGS:
        {
            opentherm_fault_t fault;
            opentherm_decode_fault(frame.data_value, &fault);
            printf("    -> Fault Flags:\n");
            printf("       OEM Code: %u\n", fault.code);
            printf("       Service Request: %d, Lockout: %d, Low Water: %d\n",
                   fault.service_request, fault.lockout_reset, fault.low_water_pressure);
            printf("       Gas/Flame: %d, Air Pressure: %d, Overtemp: %d\n",
                   fault.gas_flame_fault, fault.air_pressure_fault, fault.water_overtemp);
            break;
        }
        case OT_DATA_ID_OEM_DIAGNOSTIC_CODE:
        {
            uint16_t diag_code = frame.data_value;
            printf("    -> OEM Diagnostic Code: %u\n", diag_code);
            break;
        }
        case OT_DATA_ID_REMOTE_PARAMS:
        {
            opentherm_remote_params_t params;
            opentherm_decode_remote_params(frame.data_value, &params);
            printf("    -> Remote Parameters:\n");
            printf("       DHW Setpoint: Enable=%d, R/W=%d\n",
                   params.dhw_setpoint_enable, params.dhw_setpoint_rw);
            printf("       Max CH Setpoint: Enable=%d, R/W=%d\n",
                   params.max_ch_setpoint_enable, params.max_ch_setpoint_rw);
            break;
        }
        case OT_DATA_ID_DAY_TIME:
        {
            opentherm_time_t time;
            opentherm_decode_time(frame.data_value, &time);
            printf("    -> Day/Time: Day %u, %02u:%02u\n",
                   time.day_of_week, time.hours, time.minutes);
            break;
        }
        case OT_DATA_ID_DATE:
        {
            opentherm_date_t date;
            opentherm_decode_date(frame.data_value, &date);
            printf("    -> Date: %02u/%02u\n", date.month, date.day);
            break;
        }
        case OT_DATA_ID_YEAR:
            printf("    -> Year: %u\n", frame.data_value);
            break;
        case OT_DATA_ID_DHW_BOUNDS:
        case OT_DATA_ID_CH_BOUNDS:
        {
            uint8_t min_val, max_val;
            opentherm_decode_u8_u8(frame.data_value, &max_val, &min_val);
            printf("    -> Bounds: Min=%u°C, Max=%u°C\n", min_val, max_val);
            break;
        }
        case OT_DATA_ID_BURNER_STARTS:
        case OT_DATA_ID_CH_PUMP_STARTS:
        case OT_DATA_ID_DHW_PUMP_STARTS:
        case OT_DATA_ID_DHW_BURNER_STARTS:
            printf("    -> Start Count: %u\n", frame.data_value);
            break;
        case OT_DATA_ID_BURNER_HOURS:
        case OT_DATA_ID_CH_PUMP_HOURS:
        case OT_DATA_ID_DHW_PUMP_HOURS:
        case OT_DATA_ID_DHW_BURNER_HOURS:
            printf("    -> Operating Hours: %u\n", frame.data_value);
            break;
        case OT_DATA_ID_OPENTHERM_VERSION:
        {
            float version = opentherm_f8_8_to_float(frame.data_value);
            printf("    -> OpenTherm Version: %.2f\n", version);
            break;
        }
        case OT_DATA_ID_SLAVE_VERSION:
        case OT_DATA_ID_MASTER_VERSION:
        {
            uint8_t product_type, version;
            opentherm_decode_u8_u8(frame.data_value, &product_type, &version);
            printf("    -> Product Type: %u, Version: %u\n", product_type, version);
            break;
        }
        case OT_DATA_ID_SLAVE_PRODUCT:
            printf("    -> Product/Version: %u\n", frame.data_value);
            break;
        case OT_DATA_ID_MAX_CAPACITY:
        {
            uint8_t min_val, max_val;
            opentherm_decode_u8_u8(frame.data_value, &min_val, &max_val);
            printf("    -> Min Mod Level: %u%%, Max Capacity: %u kW\n", min_val, max_val);
            break;
        }
        default:
            // No specific decoding for this data ID
            break;
        }
    }

    // Helper function to send request and wait for response
    bool Interface::sendAndReceive(uint32_t request, uint32_t *response)
    {
        send(request);

        absolute_time_t start = get_absolute_time();
        while (absolute_time_diff_us(start, get_absolute_time()) < (timeout_ms_ * 1000))
        {
            if (receive(*response))
            {
                return true;
            }
            sleep_ms(10);
        }
        return false; // Timeout
    }

    // Status and configuration reads
    bool Interface::readStatus(opentherm_status_t *status)
    {
        uint32_t request = opentherm_read_status();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        uint16_t value = opentherm_get_u16(response);
        opentherm_decode_status(value, status);
        return true;
    }

    bool Interface::readSlaveConfig(opentherm_config_t *config)
    {
        uint32_t request = opentherm_read_slave_config();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        uint16_t value = opentherm_get_u16(response);
        opentherm_decode_slave_config(value, config);
        return true;
    }

    bool Interface::readFaultFlags(opentherm_fault_t *fault)
    {
        uint32_t request = opentherm_read_fault_flags();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        uint16_t value = opentherm_get_u16(response);
        opentherm_decode_fault(value, fault);
        return true;
    }

    bool Interface::readOemDiagnosticCode(uint16_t *diag_code)
    {
        uint32_t request = opentherm_read_oem_diagnostic_code();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *diag_code = opentherm_get_u16(response);
        return true;
    }

    // Temperature sensor reads
    bool Interface::readBoilerTemperature(float *temp)
    {
        uint32_t request = opentherm_read_boiler_water_temp();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *temp = opentherm_get_f8_8(response);
        return true;
    }

    bool Interface::readDHWTemperature(float *temp)
    {
        uint32_t request = opentherm_read_dhw_temp();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *temp = opentherm_get_f8_8(response);
        return true;
    }

    bool Interface::readOutsideTemperature(float *temp)
    {
        uint32_t request = opentherm_read_outside_temp();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *temp = opentherm_get_f8_8(response);
        return true;
    }

    bool Interface::readReturnWaterTemperature(float *temp)
    {
        uint32_t request = opentherm_read_return_water_temp();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *temp = opentherm_get_f8_8(response);
        return true;
    }

    bool Interface::readRoomTemperature(float *temp)
    {
        uint32_t request = opentherm_read_room_temp();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *temp = opentherm_get_f8_8(response);
        return true;
    }

    bool Interface::readExhaustTemperature(int16_t *temp)
    {
        uint32_t request = opentherm_read_exhaust_temp();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *temp = opentherm_get_s16(response);
        return true;
    }

    // Pressure and flow reads
    bool Interface::readCHWaterPressure(float *pressure)
    {
        uint32_t request = opentherm_read_ch_water_pressure();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *pressure = opentherm_get_f8_8(response);
        return true;
    }

    bool Interface::readDHWFlowRate(float *flow_rate)
    {
        uint32_t request = opentherm_read_dhw_flow_rate();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *flow_rate = opentherm_get_f8_8(response);
        return true;
    }

    // Modulation level read
    bool Interface::readModulationLevel(float *level)
    {
        uint32_t request = opentherm_read_rel_mod_level();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *level = opentherm_get_f8_8(response);
        return true;
    }

    // Setpoint reads
    bool Interface::readControlSetpoint(float *setpoint)
    {
        uint32_t request = opentherm_read_control_setpoint();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *setpoint = opentherm_get_f8_8(response);
        return true;
    }

    bool Interface::readDHWSetpoint(float *setpoint)
    {
        uint32_t request = opentherm_read_dhw_setpoint();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *setpoint = opentherm_get_f8_8(response);
        return true;
    }

    bool Interface::readMaxCHSetpoint(float *setpoint)
    {
        uint32_t request = opentherm_read_max_ch_setpoint();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *setpoint = opentherm_get_f8_8(response);
        return true;
    }

    // Counter/statistics reads
    bool Interface::readBurnerStarts(uint16_t *count)
    {
        uint32_t request = opentherm_read_burner_starts();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *count = opentherm_get_u16(response);
        return true;
    }

    bool Interface::readCHPumpStarts(uint16_t *count)
    {
        uint32_t request = opentherm_read_ch_pump_starts();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *count = opentherm_get_u16(response);
        return true;
    }

    bool Interface::readDHWPumpStarts(uint16_t *count)
    {
        uint32_t request = opentherm_read_dhw_pump_starts();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *count = opentherm_get_u16(response);
        return true;
    }

    bool Interface::readBurnerHours(uint16_t *hours)
    {
        uint32_t request = opentherm_read_burner_hours();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *hours = opentherm_get_u16(response);
        return true;
    }

    bool Interface::readCHPumpHours(uint16_t *hours)
    {
        uint32_t request = opentherm_read_ch_pump_hours();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *hours = opentherm_get_u16(response);
        return true;
    }

    bool Interface::readDHWPumpHours(uint16_t *hours)
    {
        uint32_t request = opentherm_read_dhw_pump_hours();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *hours = opentherm_get_u16(response);
        return true;
    }

    // Version information reads
    bool Interface::readOpenThermVersion(float *version)
    {
        uint32_t request = opentherm_read_opentherm_version();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        *version = opentherm_get_f8_8(response);
        return true;
    }

    bool Interface::readSlaveVersion(uint8_t *type, uint8_t *version)
    {
        uint32_t request = opentherm_read_slave_version();
        uint32_t response;
        if (!sendAndReceive(request, &response))
        {
            return false;
        }
        opentherm_get_u8_u8(response, type, version);
        return true;
    }

    // Write functions
    bool Interface::writeControlSetpoint(float temperature)
    {
        uint32_t request = opentherm_write_control_setpoint(temperature);
        uint32_t response;
        return sendAndReceive(request, &response);
    }

    bool Interface::writeRoomSetpoint(float temperature)
    {
        uint32_t request = opentherm_write_room_setpoint(temperature);
        uint32_t response;
        return sendAndReceive(request, &response);
    }

    bool Interface::writeDHWSetpoint(float temperature)
    {
        uint32_t request = opentherm_write_dhw_setpoint(temperature);
        uint32_t response;
        return sendAndReceive(request, &response);
    }

    bool Interface::writeMaxCHSetpoint(float temperature)
    {
        uint32_t request = opentherm_write_max_ch_setpoint(temperature);
        uint32_t response;
        return sendAndReceive(request, &response);
    }

} // namespace OpenTherm
