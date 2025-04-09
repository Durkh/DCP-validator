#include "DCP.h"

#include <driver/gpio.h>
#include <esp_private/esp_clk.h>
#include <esp_log.h>
#include <rom/ets_sys.h>
#include "esp_cpu.h"

///////////////////////////////////////////////////////////////

extern volatile struct {
    float delta;    //transmission time unit
    float moe;      //transmission margin of error
    esp_cpu_cycle_count_t limits[2];
} configParam;

static DCP_MODE targetParams;

extern volatile struct {
    float delta;    //transmission time unit
    float moe;      //transmission margin of error
    esp_cpu_cycle_count_t limits[2];
} configParam;

//ISR returns
volatile static enum DCP_Errors_e busError = ERROR_none;
volatile static uint8_t data[0xFF];

///////////////////////////////////////////////////////////////

uint8_t s_ReadByte(const gpio_num_t pin);

static void ValidationISR(void* arg){
    const gpio_num_t pin = (gpio_num_t)arg;

    esp_cpu_set_cycle_count time;

    //reading incoming data
    gpio_set_direction(pin, GPIO_MODE_INPUT);

    for (esp_cpu_set_cycle_count(0); gpio_get_level(pin) == 0; time = esp_cpu_get_cycle_count()){
        //TODO will 100*lim overflow?
        if (esp_cpu_get_cycle_count() > 100*configParam.limits[1]){
            //SYNC error
            busError = ERROR_sync_inf;
            return;
        }
    }

    esp_cpu_set_cycle_count(0);

    //check sync limit
    if(time > 50*limits[1]){
        busError = ERROR_sync_tooLong;
        return;
    }else if(time < 50*limits[0]){
        busError = ERROR_sync_tooShort;
        return;
    }

    //wait bitsync
    for (; gpio_get_level(pin) == 1; time = esp_cpu_get_cycle_count()){
        if(esp_cpu_get_cycle_count() > 10*configParam.limits[1]){
            busError = ERROR_bitSync_inf;
            return;
        }
    }

    esp_cpu_set_cycle_count(0);
    //check bitsync limit
    if(time > 7.5*limits[1]){
        busError = ERROR_bitSync_tooLong;
        return;
    }else if(time < 7.5*limits[0]){
        busError = ERROR_bitSync_tooShort;
        return;
    }

    for (; gpio_get_level(pin) == 0; time = esp_cpu_get_cycle_count()){
        if(time > 10*limits[1]){
            busError = ERROR_bitSync_invalidLow;
            return;
        }
    }

    data[0] = s_ReadByte(pin);
    const uint8_t flag = data[0]? data[0]: sizeof(struct DCP_Message_L3_t)+1;

    for (int i = 1; i < flag; ++i){
        data[i] = s_ReadByte(pin);
    }

    //is there any data being transmitted? 
    uint8_t end = s_ReadByte(pin);
    //empty bus is read as FF, anything else is extra data 
    if(end == 0xFF) return ERROR_invalidSize;

    busError = ERROR_none;
}

bool ValidL3(uint8_t* data){}
bool ValidGeneric(uint8_t* data){}

enum DCP_Errors_e TestConnection(){

    assert(limits[0] != 0 && limits[1] != 0);

    enum DCP_Errors_e ret = ERROR_noTransmission;

    if(gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3) || gpio_isr_handler_add(pin, ValidationISR, (void*)pin)){
        ESP_LOGE(TAG, "could not register gpio ISR");
        return ERROR_internal;
    }

    esp_cpu_set_cycle_count(0);
    //wait for the transmission for 10 seconds
    while (esp_cpu_get_cycle_count() < 10UL*esp_clk_cpu_freq()){
        if (busError != ERROR_none){
            ret = busError;
            break;
        }
    }

    gpio_uninstall_isr_service();

    if (ret != ERROR_noTransmission){

        const DCP_Data_t message = {.data = data};

        if (message.message->type){ //generic packet
            if (ValidGeneric(message.data)){
                return ERROR_message_invalidGeneric;
            }
        } else { //L3 packet
            if (ValidL3(message.data)){
                return ERROR_message_invalidL3;
            }
        }
    }

    return ret;
}

///////////////////////////////////////////////////////////////

static const DCP_Message_t yeldMessage = {
    .type = 5
    .generic = {
        .addr = 0x0, //highest priority
        .payload = "test"
    }
};

const uint32_t freqMHz = esp_clk_cpu_freq()/1e6;

volatile Collision_e collisionFlag;

void ForceYieldISR(void* arg){
    const gpio_num_t pin = (gpio_num_t)arg;
    static uint8_t data[0xFF];

    //reading incoming data
    gpio_set_direction(pin, GPIO_MODE_INPUT);

    //wait for SYNC to end
    while(gpio_get_level(pin) == 0) continue;

    for (esp_cpu_set_cycle_count(0); gpio_get_level(pin) == 1; ){
        if(esp_cpu_get_cycle_count() > 10*configParam.limits[1]){
            return;
        }
    }

    (void)s_ReadByte(pin);

    //let's read one byte and interrupt the transmission
    (void)s_ReadByte(pin);

    bool collision = s_SendBytes(pin, message.message->type, message.data,
            (unsigned[3]){(configParam.delta-3)*freqMHz, (2*configParam.delta-3)*freqMHz, 150});

    assert(collisionFlag == COL_null);

    collisionFlag = collision? COL_true: COL_false;
}

Collision_e DoesYield(){

    if(gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3) || gpio_isr_handler_add(pin, ForceYieldISR, (void*)pin)){
        ESP_LOGE(TAG, "could not register gpio ISR");
        return ERROR_internal;
    }

    esp_cpu_set_cycle_count(0);
    //wait for the transmission for 10 seconds
    while (esp_cpu_get_cycle_count() < 10UL*esp_clk_cpu_freq()){
        if(collisionFlag != COL_null) break;
    }

    gpio_uninstall_isr_service();

    return collisionFlag;
}
