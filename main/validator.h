#pragma once

#include <inttypes.h>
#include <stdbool.h>

#include <driver/gpio.h>

enum DCP_Errors_e {
    ERROR_none = 0UL,
    ERROR_sync_inf = 1UL,
    ERROR_sync_tooLong = 1UL << 1,
    ERROR_sync_tooShort = 1UL << 2,
    ERROR_bitSync_inf = 1UL << 3,
    ERROR_bitSync_tooLong = 1UL << 4,
    ERROR_bitSync_tooShort = 1UL << 5,
    ERROR_bitSync_invalidLow = 1UL << 6,
    ERROR_invalidSize = 1UL << 7,
    ERROR_noTransmission = 1UL << 8,
    ERROR_message_invalidGeneric = 1UL << 9,
    ERROR_message_invalidL3_header = 1UL << 10,
    ERROR_message_invalidL3_sID = 1UL << 11,
    ERROR_message_invalidL3_padding = 1UL << 12,
    ERROR_message_invalidL3_CRC = 1UL << 13,
    ERROR_internal = 1UL << 32
};

struct DCP_Transmission_t {
    uint8_t type;
    enum DCP_Errors_e errors;
};

enum Collision_e {COL_null, COL_false, COL_true};

struct DCP_timings_t {
    enum DCP_Speed_e speed;
    float sync;
    float bitSync_low;
    float bitSync_high;
    float bit0;
    float bit1;
};

struct DCP_electrical_t {
    float VIH;
    float VIL;
    float rise;
    float falling;
    float cycle;
    float speed;
};

///////////////////////////////////////////////////////////////

enum DCP_Errors_e TestConnection(const gpio_num_t pin);
struct DCP_timings_t GetTimes(const gpio_num_t pin);
struct DCP_electrical_t MeasureElectrical(const gpio_num_t pin);

uint32_t ValidL3(uint8_t* data);
uint32_t ValidGeneric(uint8_t* data);

enum Collision_e DoesYield(const gpio_num_t pin);
