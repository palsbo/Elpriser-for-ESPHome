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

struct myTimeElement_t {
    int Second;
    int Minute;
    int Hour;
    int Wday;   // dag i ugen, søndag er dag 1
    int Day;
    int Month;
    int Year;   // offset fra 1970;
};

class CONFIG {
private:
    const char* defaultConfig = R"({
        "pricearea" : "DK1",
        "chargeOwner" : "Cerius",
        "vat" : 25.0,
        "tax" : 0.7608,
        "netfee" : 0.1248,
        "divfee" : 0.1030,
        "ownerfee" : [0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0],
        "limittab" : [1.64, 1.99, 2.38, 3.34],
        "nextfive" : [{"month": 6, "day" : 20, "hour": 11, "price": 1.345},{"month": 6, "day" : 20, "hour": 12, "price": 1.345},{"month": 6, "day" : 20, "hour": 13, "price": 1.345},{"month": 6, "day" : 20, "hour": 14, "price": 1.345},{"month": 6, "day" : 20, "hour": 15, "price": 1.345},{"month": 6, "day" : 20, "hour": 16, "price": 1.345}],
        "bestfive" : [{"month": 6, "day" : 20, "hour": 11, "price": 1.345},{"month": 6, "day" : 20, "hour": 12, "price": 1.345},{"month": 6, "day" : 20, "hour": 13, "price": 1.345},{"month": 6, "day" : 20, "hour": 14, "price": 1.345},{"month": 6, "day" : 20, "hour": 15, "price": 1.345},{"month": 6, "day" : 20, "hour": 16, "price": 1.345}]
    })";

    const std::string chargeOwnersJson = R"({
        "Radius": { "gln": "5790000705689", "company": "Radius Elnet A/S", "type": ["DT_C_01"], "chargetype": ["D03"] },
        "RAH": { "gln": "5790000681327", "company": "RAH Net A/S", "type": ["RAH-C"], "chargetype": ["D03"] },
        "Konstant": { "gln": "5790000704842", "company": "Konstant Net A/S - 151", "type": ["151-NT01T", "151-NRA04T"], "chargetype": ["D03"] },
        "Cerius": { "gln": "5790000705184", "company": "Cerius A/S", "type": ["30TR_C_ET"], "chargetype": ["D03"] },
        "N1": { "gln": "5790001089030", "company": "N1 A/S - 131", "type": ["CD", "CD R"], "chargetype": ["D03"] },
        "Dinel": { "gln": "5790000610099", "company": "Dinel A/S", "type": ["TCL>100_02", "TCL<100_52"], "chargetype": ["D03"] }, 
        "TREFOR El-net": { "gln": "5790000392261", "company": "TREFOR El-net A/S", "type": ["C"], "chargetype": ["D03"] },
        "TREFOR El-net Ost": { "gln": "5790000706686", "company": "TREFOR El-net Ost A/S", "type": ["46"], "chargetype": ["D03"] },
        "Elektrus": { "gln": "5790000836239", "company": "Elektrus A/S", "type": ["6000091"], "chargetype": ["D03"] },
        "Elnet Midt": { "gln": "5790001100520", "company": "Elnet Midt A/S", "type": ["T3001"], "chargetype": ["D03"] },
        "Hurup Elvaerk Net": { "gln": "5790000610839", "company": "Hurup Elvaerk Net A/S", "type": ["HEV-NT-01"], "chargetype": ["D03"] },
        "Veksel": { "gln": "5790001088217", "company": "Veksel A/S", "type": ["NT-10"], "chargetype": ["D03"] }, 
        "Vores Elnet": { "gln": "5790000610976", "company": "Vores Elnet A/S", "type": ["TNT1009"], "chargetype": ["D03"] },
        "Netselskabet Elvaerk": { "gln": "5790000681075", "company": "Netselskabet Elvaerk A/S - 042", "type": ["0NCFF"], "chargetype": ["D03"] },
        "Nord Energi Net": { "gln": "5790000610877", "company": "Nord Energi Net A/S", "type": ["TAC"], "chargetype": ["D03"] },
        "Nordvestjysk Elforsyning NOE Net": { "gln": "5790000395620", "company": "NOE Net A/S", "type": ["30030"], "chargetype": ["D03"] },
        "Ikast El Net": { "gln": "5790000682102", "company": "Ikast El Net A/S", "type": ["IEV-NT-01"], "chargetype": ["D03"] }, 
        "FLOW Elnet": { "gln": "5790000392551", "company": "FLOW Elnet A/S", "type": ["FE2 NT-01"], "chargetype": ["D03"] },
        "Elinord": { "gln": "5790001095277", "company": "Elinord A/S", "type": ["43300"], "chargetype": ["D03"] },
        "Hammel Elforsyning Net": { "gln": "5790001090166", "company": "Hammel Elforsyning Net A/S", "type": ["50001"], "chargetype": ["D03"] },
        "El-net Kongerslev": { "gln": "5790002502699", "company": "El-net Kongerslev A/S", "type": ["C-Tarif"], "chargetype": ["D03"] },
        "Ravdex": { "gln": "5790000836727", "company": "Ravdex A/S", "type": ["NT-C"], "chargetype": ["D03"] },
        "Tarm Elvaerk Net": { "gln": "5790000706419", "company": "Tarm Elvaerk Net A/S", "type": ["TEV-NT-01", "TEV-NT-01R"], "chargetype": ["D03"] },
        "Zeanet": { "gln": "5790001089375", "company": "Zeanet A/S", "type": ["43110"], "chargetype": ["D03"] },
        "NKE-Elnet": { "gln": "5790001088231", "company": "NKE-Elnet A/S", "type": ["94TR_C_ET"], "chargetype": ["D03"] },
        "L-Net": { "gln": "5790001090111", "company": "L-Net A/S", "type": ["4010"], "chargetype": ["D03"] },
        "Midtfyns Elforsyning": { "gln": "5790001089023", "company": "Midtfyns Elforsyning A.m.b.A", "type": ["TNT15000"], "chargetype": ["D03"] },
        "Sunds Net": { "gln": "5790001095444", "company": "Sunds Net A.m.b.A", "type": ["SEF-NT-05", "SEF-NT-05R"], "chargetype": ["D03"] },
        "Aal El-Net": { "gln": "5790001095451", "company": "Aal El-Net A.m.b.A", "type": ["AAL-NT-05", "AAL-NTR05"]  }
    })";

    void saveToNVS(const char* jsonStr) {
        nvs_handle_t my_handle;
        esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
        if (err == ESP_OK) {
            err = nvs_set_str(my_handle, "config", jsonStr);
            if (err == ESP_OK) {
                nvs_commit(my_handle);
                ESP_LOGI("NVS", "Konfiguration gemt.");
            } else {
                ESP_LOGE("NVS", "Fejl ved lagring af konfiguration.");
            }
            nvs_close(my_handle);
        } else {
            ESP_LOGE("NVS", "Kunne ikke åbne NVS.");
        }
    }

    bool loadFromNVS(String& output) {
        nvs_handle_t my_handle;
        esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &my_handle);
        if (err != ESP_OK) {
            return false;
        }
        size_t required_size;
        err = nvs_get_str(my_handle, "config", NULL, &required_size);
        if (err == ESP_OK && required_size > 0) {
            char* buffer = (char*)malloc(required_size);
            if (buffer) {
                err = nvs_get_str(my_handle, "config", buffer, &required_size);
                if (err == ESP_OK) {
                    output = String(buffer);
                    free(buffer);
                    nvs_close(my_handle);
                    return true;
                }
                free(buffer);
            }
        }
        nvs_close(my_handle);
        return false;
    }

    void parseJSON(const String& jsonStr) {
        cJSON* root = cJSON_Parse(jsonStr.c_str());
        if (!root) {
            ESP_LOGE("JSON", "Fejl ved parsing af JSON.");
            return;
        } else {
            ESP_LOGI("JSON", "Parsing af JSON. %s", jsonStr.c_str());
        }
        pricearea = cJSON_GetObjectItem(root, "pricearea")->valuestring;
        chargeOwner = cJSON_GetObjectItem(root, "chargeOwner")->valuestring;
        vat = cJSON_GetObjectItem(root, "vat")->valuedouble;
        tax = cJSON_GetObjectItem(root, "tax")->valuedouble;
        netfee = cJSON_GetObjectItem(root, "netfee")->valuedouble;
        divfee = cJSON_GetObjectItem(root, "divfee")->valuedouble;
        cJSON* ownerfeeArray = cJSON_GetObjectItem(root, "ownerfee");
        for (int i = 0; i < 24; i++) {
            ownerfee[i] = (float)cJSON_GetArrayItem(ownerfeeArray, i)->valuedouble;
        }
        cJSON* limittabArray = cJSON_GetObjectItem(root, "limittab");
        for (int i = 0; i < 4; i++) {
            limittab[i] = (float)cJSON_GetArrayItem(limittabArray, i)->valuedouble;
        }
        cJSON_Delete(root);
    }

public:
    String pricearea;
    String chargeOwner;
    float vat;
    float tax;
    float netfee;
    float divfee;
    float ownerfee[24];
    float limittab[4];

    struct TimePrice {
        int month;
        int day;
        int hour;
        float price;
        long colorIndex;
    };

    TimePrice nextfive[6];
    TimePrice bestfive[6];

    CONFIG() {
        ESP_LOGI("Config", "✅❌✅  Config started");
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            err = nvs_flash_init();
        }
        ESP_ERROR_CHECK(err);
        String storedConfig;
#ifdef RESET_CONFIG
        saveToNVS(defaultConfig);
        parseJSON(defaultConfig);
#else
        if (loadFromNVS(storedConfig)) {
            ESP_LOGI("CONFIG", "✅ Indlæser gemt konfiguration...");
            parseJSON(storedConfig);
        } else {
            ESP_LOGI("CONFIG", "❌ Ingen konfiguration fundet, gemmer defaultConfig...");
            saveToNVS(defaultConfig);
            parseJSON(defaultConfig);
        }
#endif
    }

    void saveConfig() {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "pricearea", pricearea.c_str());
        cJSON_AddStringToObject(root, "chargeOwner", chargeOwner.c_str());
        cJSON_AddNumberToObject(root, "vat", vat);
        cJSON_AddNumberToObject(root, "tax", tax);
        cJSON_AddNumberToObject(root, "netfee", netfee);
        cJSON_AddNumberToObject(root, "divfee", divfee);
        cJSON* ownerfeeArray = cJSON_CreateFloatArray(ownerfee, 24);
        cJSON_AddItemToObject(root, "ownerfee", ownerfeeArray);
        cJSON* limittabArray = cJSON_CreateFloatArray(limittab, 4);
        cJSON_AddItemToObject(root, "limittab", limittabArray);
        char* jsonStr = cJSON_Print(root);
        saveToNVS(jsonStr);
        free(jsonStr);
        cJSON_Delete(root);
    }

    cJSON * jDoc() {
        return cJSON_Parse(chargeOwnersJson.c_str());
    };

    void set_selector(esphome::select::Select *sensor) {
        cJSON *root = jDoc();
        if (!root) { ESP_LOGE("Elpriser", "❌ JSON parsing failed!"); return; }
        std::vector<std::string> options;
        cJSON *item = root->child;
        while (item) {
            if (item->string) {
                options.push_back(std::string(item->string));  // Gem nøglen (distributørnavnet)
            }
            item = item->next;
        }
        if (!options.empty()) {
            sensor->traits.set_options(options);
        }
        cJSON_Delete(root);
    }
};

}  // namespace elpriser
}  // namespace esphome
