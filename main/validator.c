#include "DCP.h"
#include "validator.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/portmacro.h>

#include <driver/gpio.h>
#include <esp_private/esp_clk.h>
#include <esp_log.h>
#include <rom/ets_sys.h>
#include "esp_cpu.h"

///////////////////////////////////////////////////////////////

extern portMUX_TYPE criticalMutex;

struct DCP_electrical_t MeasureElectrical(const gpio_num_t pin){
    struct DCP_electrical_t ret = {0};

    //TODO use oneshot ADC
    ret.VIH = 3.3;
    ret.VIL = 0;

    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_set_level(pin, 0);
    
    taskENTER_CRITICAL(&criticalMutex);

    esp_cpu_set_cycle_count(0);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    while(gpio_get_level(pin) == 1);
    const esp_cpu_cycle_count_t th = esp_cpu_get_cycle_count();

    esp_cpu_set_cycle_count(0);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    while(gpio_get_level(pin) == 0);
    const esp_cpu_cycle_count_t tl = esp_cpu_get_cycle_count();

    taskEXIT_CRITICAL(&criticalMutex);

    ret.rise = (double)th/esp_clk_cpu_freq();
    ret.falling = (double)tl/esp_clk_cpu_freq();

    ret.cycle = ret.rise + ret.falling;

    if (ret.cycle != 0){
        ret.speed = 1/ret.cycle;
    }

    return ret;
}

///////////////////////////////////////////////////////////////

extern volatile struct {
    float delta;    //transmission time unit
    float moe;      //transmission margin of error
    esp_cpu_cycle_count_t limits[2];
} configParam;

static DCP_MODE targetParams;

extern bool s_SendBytes(gpio_num_t const pin, uint8_t const size, uint8_t const data[size], unsigned const delays[restrict 3]);
extern uint8_t s_ReadByte(const gpio_num_t pin);

///////////////////////////////////////////////////////////////

volatile static struct rawCycles_t{
    esp_cpu_cycle_count_t sync;
    esp_cpu_cycle_count_t bitSync_low;
    esp_cpu_cycle_count_t bitSync_high;
    esp_cpu_cycle_count_t bit0;
    esp_cpu_cycle_count_t bit1;
} rawCycles;

bool ReadBit(const gpio_num_t pin){
    
    while (gpio_get_level(pin) == 0)
        continue;

    //reading high time
    esp_cpu_set_cycle_count(0);
    for (esp_cpu_cycle_count_t lim = configParam.limits[1] << 1; gpio_get_level(pin) == 1 && esp_cpu_get_cycle_count() < lim;)
        continue;

    const esp_cpu_cycle_count_t t = esp_cpu_get_cycle_count(); 

    if (t <= configParam.limits[1]){
        rawCycles.bit0 = t;
        return 0;
    }else {
        rawCycles.bit1 = t;
        return 1;
    }
}

uint8_t ReadByte(const gpio_num_t pin){

    uint8_t byte = 0;

    for (int i = 7; i >= 0; --i){
        byte |= ReadBit(pin) << i;
    }

    return byte;
}

uint32_t ValidL3(uint8_t* data){return 0;}
uint32_t ValidGeneric(uint8_t* data){return 0;}

struct DCP_Transmission_t TestConnection(const gpio_num_t pin){

    assert(configParam.limits[0] != 0 && configParam.limits[1] != 0);

    gpio_set_direction(pin, GPIO_MODE_INPUT);

    struct DCP_Transmission_t ret = {};
    static uint8_t data[0xFF];

    rawCycles.sync = 0;
    rawCycles.bitSync_low = 0;
    rawCycles.bitSync_high = 0;
    rawCycles.bit0 = 0;
    rawCycles.bit1 = 0;

    //only leave trap if no data is on bus
    //wait for at least 15delta of idle
    esp_cpu_set_cycle_count(0);
    while (esp_cpu_get_cycle_count() < 15*configParam.limits[1]){
        if (gpio_get_level(pin) == 0) esp_cpu_set_cycle_count(0);
    }

    for(esp_cpu_set_cycle_count(0); esp_cpu_get_cycle_count() < 10UL*esp_clk_cpu_freq();){
        if(gpio_get_level(pin) == 0){

            for (esp_cpu_set_cycle_count(0); gpio_get_level(pin) == 0; rawCycles.sync = esp_cpu_get_cycle_count()){
                //TODO will 100*lim overflow?
                if (rawCycles.sync > 100*configParam.limits[1]){
                   ret.errors |= ERROR_sync_inf;
                }
            }

            esp_cpu_set_cycle_count(0);

            //TODO change the hardcoded 25 to target param
            //check sync limit
            if(rawCycles.sync > 25*configParam.limits[1]){
               ret.errors |= ERROR_sync_tooLong;
            }else if(rawCycles.sync < 25*configParam.limits[0]){
               ret.errors |= ERROR_sync_tooShort;
            }

            //wait bitsync
            for (;gpio_get_level(pin) == 1; rawCycles.bitSync_high = esp_cpu_get_cycle_count()){
                if(rawCycles.bitSync_high > 10*configParam.limits[1]){
                   ret.errors |= ERROR_bitSync_inf;
                }
            }

            esp_cpu_set_cycle_count(0);
            //check bitsync limit
            if(rawCycles.bitSync_high > 7.5*configParam.limits[1]){
               ret.errors |= ERROR_bitSync_tooLong;
            }else if(rawCycles.bitSync_high < 7.5*configParam.limits[0]){
               ret.errors |= ERROR_bitSync_tooShort;
            }

            for (; gpio_get_level(pin) == 0; rawCycles.bitSync_low = esp_cpu_get_cycle_count()){
                if(rawCycles.bitSync_low > 10*configParam.limits[1]){
                   ret.errors |= ERROR_bitSync_invalidLow;
                }
            }

            data[0] = ReadByte(pin);
            const uint8_t flag = data[0]? data[0]: sizeof(struct DCP_Message_L3_t)+1;

            for (int i = 1; i < flag; ++i){
                data[i] = ReadByte(pin);
            }

            //is there any data being transmitted? 
            uint8_t end = ReadByte(pin);
            //empty bus is read as FF, anything else is extra data 
            if(end != 0xFF){
               ret.errors |= ERROR_invalidSize;
            }

            ret.type = data[0];

            const DCP_Data_t message = {.data = data};
            ret.errors |= message.message->type? ValidGeneric(message.data): ValidL3(message.data);

            return ret;
        }
    }

    return ERROR_noTransmission;
}

///////////////////////////////////////////////////////////////

struct DCP_timings_t GetTimes(const gpio_num_t pin){

    struct DCP_timings_t ret;
    const uint32_t freqMHz = esp_clk_cpu_freq()/1e6; //should be 180

    ESP_LOGV("times", "sync: %lu\t BSH: %lu\t BSL: %lu\tB0: %lu\tB1: %lu", 
        rawCycles.sync,
        rawCycles.bitSync_high,
        rawCycles.bitSync_low,
        rawCycles.bit0,
        rawCycles.bit1
    );

    ret.speed = 0xFF;
    ret.sync = rawCycles.sync/freqMHz;
    ret.bitSync_low = rawCycles.bitSync_low/freqMHz;
    ret.bitSync_high = rawCycles.bitSync_high/freqMHz;
    ret.bit0 = rawCycles.bit0/freqMHz;
    ret.bit1 = rawCycles.bit1/freqMHz;

    if(ret.bit0 < 2){
        ret.speed = ULTRA;
    }else if(ret.bit0 < 3){
        ret.speed = FAST2;
    }else if(ret.bit0 < 6){
        ret.speed = FAST1;
    }else if(ret.bit0 < 23){
        ret.speed = SLOW;
    }

    return ret;
}

///////////////////////////////////////////////////////////////

static const struct DCP_Message_t yieldMessage = (struct DCP_Message_t){
    .type = 5,
    .generic = {
        .addr = 0x0, //highest priority
        .payload = "test"
    }
};

enum Collision_e DoesYield(const gpio_num_t pin){
    enum Collision_e collisionFlag = COL_null;
    const uint32_t freqMHz = esp_clk_cpu_freq()/1e6;

    gpio_set_direction(pin, GPIO_MODE_INPUT);

    for(esp_cpu_set_cycle_count(0); esp_cpu_get_cycle_count() < 10UL*esp_clk_cpu_freq();){
        if(gpio_get_level(pin) == 0){
            //wait for SYNC to end
            while(gpio_get_level(pin) == 0) continue;

            for (esp_cpu_set_cycle_count(0); gpio_get_level(pin) == 1; ){
                if(esp_cpu_get_cycle_count() > 10*configParam.limits[1]){
                    return collisionFlag;
                }
            }

            if(esp_cpu_get_cycle_count() <= 6*configParam.limits[0]){
                    return collisionFlag;
            }

            (void)s_ReadByte(pin);

            //let's read one byte and interrupt the transmission
            (void)s_ReadByte(pin);

            DCP_Data_t message = {.message = &yieldMessage};
            bool collision = s_SendBytes(pin, message.message->type, message.data,
                    (unsigned[3]){(configParam.delta-3)*freqMHz, (2*configParam.delta-3)*freqMHz, 150});

            gpio_set_direction(pin, GPIO_MODE_INPUT);
            //assert(collisionFlag == COL_null);

            collisionFlag = collision? COL_true: COL_false;
        }
    }

    return collisionFlag;
}
