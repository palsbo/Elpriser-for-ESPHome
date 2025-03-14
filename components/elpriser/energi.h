#pragma once

#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esphome/core/log.h"
#include "esphome.h"
#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/select/select.h"
#include "esphome/components/number/number.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/application.h"
#include "esphome/components/time/real_time_clock.h"
#include "esp_http_client.h"
#include <map>
#include <vector>
#include <string>
#include <ArduinoJson.h>
#include <ctime>
#include "cJSON.h"

//#define RESET_CONFIG
#define STORAGE_NAMESPACE "config"
#define SAFE_PTR(ptr) ((uintptr_t) &(ptr) != 0 ? ptr : nullptr)

namespace esphome {
namespace elpriser {

struct Record {
    const char* HourDK;
    float SpotPriceDKK;
    int Hour;
    int Day;
    int Month;
    const char* Date;
};

class ENERGI {
    private:
        static esp_err_t _data_event_handler(esp_http_client_event_t *evt) {
            ENERGI *energi = static_cast<ENERGI *>(evt->user_data);
            switch (evt->event_id) {
                case HTTP_EVENT_ERROR:
                    ESP_LOGE("ENERGI", "❌ HTTP_EVENT_ERROR");
                    break;
                case HTTP_EVENT_ON_DATA:
                    if (evt->data && evt->data_len > 0) {
                        energi->dataBuf.append(static_cast<char*>(evt->data), evt->data_len);
                    }
                    break;
                case HTTP_EVENT_ON_FINISH:
    //                ESP_LOGI("ENERGI", "HTTP_EVENT_ON_DATA, len=%d, data=%s", energi->dataBuf.length(), energi->dataBuf.c_str());
                    energi->dataBufReady = energi->dataBuf.length()>0;
                    energi->waiting = (energi->tarifBufReady && energi->dataBufReady);
                    break;
                case HTTP_EVENT_DISCONNECTED:
                    break;
            }
            return ESP_OK;
        }

        static esp_err_t _tarif_event_handler(esp_http_client_event_t *evt) {
            ENERGI *energi = static_cast<ENERGI *>(evt->user_data);
            switch (evt->event_id) {
                case HTTP_EVENT_ERROR:
                    ESP_LOGE("ENERGI", "❌ HTTP_EVENT_ERROR");
                    break;
                case HTTP_EVENT_ON_DATA:
                    if (evt->data && evt->data_len > 0) {
                        energi->tarifBuf.append(static_cast<char*>(evt->data), evt->data_len);
                    }
                    break;
                case HTTP_EVENT_ON_FINISH:
    //                ESP_LOGI("ENERGI", "HTTP_EVENT_ON_DATA, len=%d, data=%s", energi->tarifBuf.length(), energi->tarifBuf.c_str());
                    energi->tarifBufReady = energi->tarifBuf.length()>0;
                    energi->waiting = (energi->tarifBufReady && energi->dataBufReady);
                    break;
                case HTTP_EVENT_DISCONNECTED:
                    break;
            }
            return ESP_OK;
        }

    public:
        std::string dataBuf;
        std::string tarifBuf;
        bool dataBufReady = false;
        bool tarifBufReady = false;
        bool waiting = false;
        void getData(std::string url) {
            dataBuf = "";
            dataBufReady = false;
            esp_http_client_config_t config = {
                .url = url.c_str(),
                .auth_type = HTTP_AUTH_TYPE_BASIC,
                .tls_version = ESP_HTTP_CLIENT_TLS_VER_TLS_1_2,
                .event_handler = _data_event_handler,
                .user_data = this  // Passer 'this' pointer til event-handleren
            };
            esp_http_client_handle_t client = esp_http_client_init(&config);
            esp_err_t err = esp_http_client_perform(client);
            if (err != ESP_OK) { ESP_LOGE("ENERGI", "❌ HTTP GET request failed: %s", esp_err_to_name(err)); }
            esp_http_client_cleanup(client);
        };
        void getTarif(std::string url) {
            tarifBuf = "";
            tarifBufReady = false;
            esp_http_client_config_t config = {
                .url = url.c_str(),
                .auth_type = HTTP_AUTH_TYPE_BASIC,
                .tls_version = ESP_HTTP_CLIENT_TLS_VER_TLS_1_2,
                .event_handler = _tarif_event_handler,
                .user_data = this  // Passer 'this' pointer til event-handleren
            };
            esp_http_client_handle_t client = esp_http_client_init(&config);
            esp_err_t err = esp_http_client_perform(client);
            if (err != ESP_OK) { ESP_LOGE("ENERGI", "HTTP GET request failed: %s", esp_err_to_name(err)); }
            esp_http_client_cleanup(client);
        };
};

}  // namespace elpriser
}  // namespace esphome
