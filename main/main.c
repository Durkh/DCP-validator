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

    DCP_MODE mode;
    mode.addr = 0xA;
    mode.flags.flags = FLAG_Instant;
    mode.isController = true;
    mode.speed = SLOW;

    if (!DCPInit(1, mode)){
        ESP_LOGE("MAIN", "Error initializing bus");
        return;
    }

    struct DCP_Message_t* Rx = NULL;

    struct DCP_Message_t msg = {
        .type = 0,
        .L3 = (struct DCP_Message_L3_t){
            .IDS = 0xA,
            .IDD = 0xF0,
            .COD = 0x1,
            .data = {0xC, 0xA, 0xF, 0xE},
            .PAD = 0x0
        }
    };

    while(1){

        DCP_Data_t Tx = (DCP_Data_t){.data = malloc(sizeof (struct DCP_Message_t))};
        memcpy((void*)Tx.message, &msg, sizeof msg);
        SendMessage(Tx);

        vTaskDelay(pdMS_TO_TICKS(100));

        Rx = ReadMessage();

        if (Rx) {
            DCP_Data_t debug = {.message = Rx};
            SanityCheck(Rx->type, debug.data);
            free(Rx);
            Rx = NULL;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}
