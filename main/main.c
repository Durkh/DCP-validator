#include <stdint.h>
#include <stdio.h>
#include "DCP.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <string.h>

#define READ 1

void SanityCheck(unsigned const int size, uint8_t const msg[]){
    for (int i = 0; i < size; ++i)
        for (int j = 7; j >= 0; --j){
            printf("%d", ((msg[i] >> j) & 0x1) == 1 );
        }
    printf("\r\n");
}

void app_main(void){

    //unsigned char msg[] = "egidio neto da computacao";

    DCP_MODE mode;
    mode.addr = 0xFF;
    mode.flags.flags = FLAG_Instant;
    mode.isController = true;
    mode.speed = SLOW;

    if (!DCPInit(1, mode)){
        ESP_LOGE("MAIN", "Error initializing bus");
        return;
    }

    struct DCP_Message_t* Rx = NULL;

    const unsigned char msg[] = "egidio";

    while(1){

        DCP_Data_t Tx = {.data = (uint8_t*)malloc(sizeof(struct DCP_Message_Generic_t) + strlen((char*)msg)+1 + sizeof(uint8_t))};

        Tx.message->type = strlen((char*)msg)+3;
        Tx.message->generic.addr = mode.addr;
        memcpy(Tx.message->generic.payload, msg, strlen((char*)msg)+1);
        SanityCheck(Tx.message->type, Tx.data);

        SendMessage(Tx);

        vTaskDelay(pdMS_TO_TICKS(100));

        Rx = ReadMessage();

        if (Rx) {
            DCP_Data_t debug = {.message = Rx};
            SanityCheck(Rx->type, debug.data);
            free(Rx);
            Rx = NULL;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }

}
