/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_chip_info.h"
#include "esp_random.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"

#include "DCP.h"
#include "validator.h"

static const char *REST_TAG = "esp-rest";
#define REST_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(REST_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0)

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];

    rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/') {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
        ESP_LOGE(REST_TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1) {
            ESP_LOGE(REST_TAG, "Failed to read file : %s", filepath);
        } else if (read_bytes > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                close(fd);
                ESP_LOGE(REST_TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(REST_TAG, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

void AddToJSON(cJSON* root, char const * name, const float value){
    cJSON *JSONvalue = cJSON_CreateNumber(value);
    cJSON_AddItemToObject(root, name, JSONvalue);
}

/* Simple handler for getting system handler */
static esp_err_t system_info_get_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;

    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;

    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }

    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post device data");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if(!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid request");
        return ESP_FAIL;
    }

    cJSON_Delete(root);

    bool isController = cJSON_IsTrue(cJSON_GetObjectItem(root, "isController"));
    int deviceSpeed = cJSON_GetObjectItem(root, "deviceSpeed")->valueint;

    unsigned busSpeed = SLOW;
    switch(deviceSpeed){
        case 4:  busSpeed = SLOW;  break;
        case 20: busSpeed = FAST1; break;
        case 32: busSpeed = FAST2; break;
        case 64: busSpeed = ULTRA; break;
        default: break;
    }

    const DCP_MODE mode = {.addr = 0xFF, .flags.flags = FLAG_Instant, .isController = isController, .speed = busSpeed};
    const gpio_num_t pin = 1;

    if (!DCPInit(pin, mode)){
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not init bus");
        return ESP_FAIL;
    }

    ESP_LOGI(REST_TAG, "Performing validation");

    //performing validation
    struct DCP_electrical_t electrical = MeasureElectrical(pin);
    ESP_LOGV("[validation]", "VIH: %f\tVIL: %f\trise: %f\tfall: %f\tcycle: %f\tspeed: %f",
        electrical.VIH, electrical.VIL, electrical.rise, electrical.falling, electrical.cycle, electrical.speed);

    struct DCP_Transmission_t transmission = TestConnection(pin);
    ESP_LOGV("[validation]", "error: 0x%X", transmission.errors);
    struct DCP_timings_t timings = GetTimes(pin);
    ESP_LOGV("[validation]", "sync: %f\tBS_low: %f\tBS_high: %f\tbit0: %f\tbit1: %f",
        timings.sync, timings.bitSync_low, timings.bitSync_high, timings.bit0, timings.bit1);

    enum Collision_e yield = DoesYield(pin);
    ESP_LOGV("[validation]", "yield: %d", yield);

    //sending result
    httpd_resp_set_type(req, "application/json");

    root = cJSON_CreateObject();

    //populate specConformity
    cJSON* JSON_specConf = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "specConformity", JSON_specConf);

    AddToJSON(JSON_specConf, "Speed", timings.speed);
    AddToJSON(JSON_specConf, "Bit High Time", timings.bit1);
    AddToJSON(JSON_specConf, "Bit Low Time", timings.bit0);
    AddToJSON(JSON_specConf, "Sync Time", timings.sync);
    AddToJSON(JSON_specConf, "Bit Sync Time", timings.bitSync_low+timings.bitSync_high);
    AddToJSON(JSON_specConf, "Bit Sync High", timings.bitSync_high);
    AddToJSON(JSON_specConf, "Bit Sync Low", timings.bitSync_low);
    AddToJSON(JSON_specConf, "Bus Yield", yield == COL_false);

    //populate electricalInfo
    cJSON* JSON_elec = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "electricalInfo", JSON_elec);

    AddToJSON(JSON_elec, "VIH", electrical.VIH);
    AddToJSON(JSON_elec, "VIL", electrical.VIL);
    AddToJSON(JSON_elec, "Rise Time", electrical.rise);
    AddToJSON(JSON_elec, "Falling Time", electrical.falling);
    AddToJSON(JSON_elec, "Cycle Time", electrical.rise + electrical.falling);
    AddToJSON(JSON_elec, "Bus Max Speed", electrical.speed);

    //populate transmissionInfo
    cJSON* item;
    cJSON* JSON_transmission = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "transmissionInfo", JSON_transmission);
    AddToJSON(JSON_transmission, "Transmission Type", transmission.type);
    
    //sync bitsync size
    if(transmission.errors & (ERROR_sync_inf | ERROR_sync_tooLong | ERROR_sync_tooShort)){
        item  = cJSON_CreateObject();
        cJSON_AddItemToObject(JSON_transmission, "Sync", item);
        cJSON_AddItemToObject(item, "status", cJSON_CreateFalse());

        switch (transmission.errors){
            case ERROR_sync_inf:
            cJSON_AddItemToObject(item, "reason", cJSON_CreateString("Infinite sync signal"));
            break;
            case ERROR_sync_tooLong:
            cJSON_AddItemToObject(item, "reason", cJSON_CreateString("Sync signal too long"));
            break;
            case ERROR_sync_tooShort:
            cJSON_AddItemToObject(item, "reason", cJSON_CreateString("Sync signal too short"));
            break;
            default:
        }
    }else {
        item  = cJSON_CreateObject();
        cJSON_AddItemToObject(JSON_transmission, "BitSync", item);
        cJSON_AddItemToObject(item, "status", cJSON_CreateTrue());
    }

    if(transmission.errors & (ERROR_bitSync_inf | ERROR_bitSync_tooLong | ERROR_bitSync_tooShort | ERROR_bitSync_invalidLow)){
        item  = cJSON_CreateObject();
        cJSON_AddItemToObject(JSON_transmission, "BitSync", item);
        cJSON_AddItemToObject(item, "status", cJSON_CreateFalse());

        switch (transmission.errors){
            case ERROR_bitSync_inf:
            cJSON_AddItemToObject(item, "reason", cJSON_CreateString("Infinite bitsync signal"));
            break;
            case ERROR_bitSync_tooLong:
            cJSON_AddItemToObject(item, "reason", cJSON_CreateString("BitSync signal too long"));
            break;
            case ERROR_bitSync_tooShort:
            cJSON_AddItemToObject(item, "reason", cJSON_CreateString("BitSync signal too short"));
            break;
            case ERROR_bitSync_invalidLow:
            cJSON_AddItemToObject(item, "reason", cJSON_CreateString("BitSync signal with invalid low"));
            break;
            default:
        }
    }else {
        item  = cJSON_CreateObject();
        cJSON_AddItemToObject(JSON_transmission, "Sync", item);
        cJSON_AddItemToObject(item, "status", cJSON_CreateTrue());
    }

    item  = cJSON_CreateObject();
    cJSON_AddItemToObject(JSON_transmission, "Size", item);
    cJSON_AddItemToObject(item, "status", cJSON_CreateBool(!(transmission.errors & ERROR_invalidSize)));

    if(transmission.errors & (ERROR_message_invalidL3_header | ERROR_message_invalidL3_sID | ERROR_message_invalidL3_padding | ERROR_message_invalidL3_CRC)){
        item  = cJSON_CreateObject();
        cJSON_AddItemToObject(JSON_transmission, "L3", item);
        cJSON_AddItemToObject(item, "status", cJSON_CreateFalse());

        switch (transmission.errors){
            case ERROR_sync_inf:
            cJSON_AddItemToObject(item, "reason", cJSON_CreateString("Infinite sync signal"));
            break;
            case ERROR_sync_tooLong:
            cJSON_AddItemToObject(item, "reason", cJSON_CreateString("Sync signal too long"));
            break;
            case ERROR_sync_tooShort:
            cJSON_AddItemToObject(item, "reason", cJSON_CreateString("Sync signal too short"));
            break;
            default:
        }
    }else {
        item  = cJSON_CreateObject();
        cJSON_AddItemToObject(JSON_transmission, "Sync", item);
        cJSON_AddItemToObject(item, "status", cJSON_CreateTrue());
    }

    const char *validationResult = cJSON_Print(root);
    httpd_resp_sendstr(req, validationResult);
    free((void *)validationResult);

    cJSON_Delete(root);

    return ESP_OK;
}

esp_err_t start_rest_server(const char *base_path)
{
    REST_CHECK(base_path, "wrong base path", err);
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    REST_CHECK(rest_context, "No memory for rest context", err);
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(REST_TAG, "Starting HTTP Server");
    REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed", err_start);

    /* URI handler for fetching system info */
    httpd_uri_t system_info_get_uri = {
        .uri = "/api/v1/validation",
        .method = HTTP_POST,
        .handler = system_info_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &system_info_get_uri);

    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &common_get_uri);

    return ESP_OK;
err_start:
    free(rest_context);
err:
    return ESP_FAIL;
}
