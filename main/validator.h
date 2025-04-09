
#include <inttypes.h>
#include <stdbool.h>


MeasureTimes();

enum DCP_Errors_e {
    ERROR_none,
    ERROR_sync_inf,
    ERROR_sync_tooLong,
    ERROR_sync_tooShort,
    ERROR_bitSync_inf,
    ERROR_bitSync_tooLong,
    ERROR_bitSync_tooShort,
    ERROR_bitSync_invalidLow,
    ERROR_invalidSize,
    ERROR_internal,
    ERROR_noTransmission,
    ERROR_message_invalidGeneric,
    ERROR_message_invalidL3
};

enum Collision_e {COL_null, COL_false, COL_true};

struct DCP_timings_t {
    float sync;
    float bitSync_low;
    float bitSync_high;
    float delta;
};

enum DCP_Errors_e TestConnection();

bool ValidL3(uint8_t* data);
bool ValidGeneric(uint8_t* data);

Collision_e DoesYield();
