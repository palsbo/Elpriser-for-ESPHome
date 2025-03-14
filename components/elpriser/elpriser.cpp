#include "elpriser.h"
//  Externs are defined as entities in elpriser.yaml

extern esphome::time::RealTimeClock *time_ __attribute__((weak));

extern esphome::text_sensor::TextSensor *time_stamp_ __attribute__((weak));
extern esphome::text_sensor::TextSensor *next_six_json_ __attribute__((weak));
extern esphome::text_sensor::TextSensor *best_six_json_ __attribute__((weak));

extern esphome::number::Number *vat_ __attribute__((weak));
extern esphome::number::Number *div_ __attribute__((weak));
extern esphome::number::Number *net_ __attribute__((weak));
extern esphome::number::Number *tax_ __attribute__((weak));
extern esphome::number::Number *limit1_ __attribute__((weak));
extern esphome::number::Number *limit2_ __attribute__((weak));
extern esphome::number::Number *limit3_ __attribute__((weak));
extern esphome::number::Number *limit4_ __attribute__((weak));

extern esphome::select::Select *region_ __attribute__((weak));
extern esphome::select::Select *distributor_ __attribute__((weak));

extern esphome::sensor::Sensor *current_hour_ __attribute__((weak));
extern esphome::sensor::Sensor *current_price_ __attribute__((weak));
extern esphome::sensor::Sensor *color_index_ __attribute__((weak));


namespace esphome {
namespace elpriser {


//---------------------utils---------------

const char *iso = "%Y-%m-%dT%H:00";

std::string posix_timezone(const std::string &timezone_name) {
    const std::map<std::string, std::string> timezone_map = {
        {"Europe/Copenhagen", "CET-1CEST,M3.5.0,M10.5.0/3"},
        {"Europe/Berlin", "CET-1CEST,M3.5.0,M10.5.0/3"},
        {"America/New_York", "EST5EDT,M3.2.0,M11.1.0"},
        {"UTC", "UTC0"},
        {"Asia/Tokyo", "JST-9"}
    };
    auto it = timezone_map.find(timezone_name);
    if (it != timezone_map.end()) {
        return it->second;
    }
    return "UTC0";  // Fallback hvis ikke fundet
}

std::string getDateTimeStringByParams(tm * timeinfo, const char* pattern = "%d/%m/%Y %H:%M:%S") {
    char buffer[30];
    strftime(buffer, sizeof(buffer), pattern, timeinfo);
    return std::string(buffer);
}
std::string getLocalDateTime(const char* pattern = "%d/%m/%Y %H:%M:%S", long addHour = 0) {
    time_t now = time_->now().timestamp;
    tm newtime;
    localtime_r(&now, &newtime);
    newtime.tm_hour += addHour;
    mktime(&newtime);
    return getDateTimeStringByParams(&newtime, pattern);
}

std::string startYear() { 
    time_t now = time_->now().timestamp;
    tm newtime;
    localtime_r(&now, &newtime);
    newtime.tm_year -= 1;
    mktime(&newtime);
    return getDateTimeStringByParams(&newtime, "%Y-01-01T00:00");
}
std::string start() { return getLocalDateTime(iso, 0); }
std::string end() { return getLocalDateTime(iso, 24); }

bool onSecond(tm * timeinfo) {
    static int oldValue = -1;
    if (oldValue == timeinfo->tm_sec) return false;
    oldValue = timeinfo->tm_sec;
    return true;
}

bool onMinute(tm * timeinfo) {
    static int oldValue = -1;
    if (oldValue == timeinfo->tm_min) return false;
    oldValue = timeinfo->tm_min;
    return true;
}

bool onHour(tm * timeinfo) {
    static int oldValue = -1;
    if (oldValue == timeinfo->tm_hour) return false;
    oldValue = timeinfo->tm_hour;
    return true;
}

std::string makeArr(cJSON *jsonArray) {
    if (!jsonArray || !cJSON_IsArray(jsonArray)) { return "[]"; }
    std::string result = "[";
    cJSON *item = jsonArray->child;
    while (item) {
        if (cJSON_IsString(item)) {
            result += "\"" + std::string(item->valuestring) + "\"";
        } else if (cJSON_IsNumber(item)) {
            result += std::to_string(item->valuedouble);
        }
        item = item->next;
        if (item) {
            result += ",";
        }
    }
    result += "]";
    return result;
}

std::string timePriceArrayToJson(const CONFIG::TimePrice (&timeArray)[ARRSIZE]) {
    cJSON *jsonArray = cJSON_CreateArray();
    if (!jsonArray) { ESP_LOGE("JSON", "❌ Kunne ikke oprette JSON-array!"); return "[]"; }
    for (int i = 0; i < ARRSIZE; i++) {
        cJSON *jsonObj = cJSON_CreateObject();
        if (!jsonObj) { ESP_LOGE("JSON", "❌ Kunne ikke oprette JSON-objekt!"); cJSON_Delete(jsonArray); return "[]"; }
        cJSON_AddNumberToObject(jsonObj, "month", timeArray[i].month);
        cJSON_AddNumberToObject(jsonObj, "day", timeArray[i].day);
        cJSON_AddNumberToObject(jsonObj, "hour", timeArray[i].hour);
        cJSON_AddNumberToObject(jsonObj, "price", timeArray[i].price);
        cJSON_AddNumberToObject(jsonObj, "colorIndex", timeArray[i].colorIndex);
        cJSON_AddItemToArray(jsonArray, jsonObj);
    }
    char *jsonStr = cJSON_PrintUnformatted(jsonArray); // Minified version
    std::string result = jsonStr ? std::string(jsonStr) : "[]";
    cJSON_free(jsonStr);
    cJSON_Delete(jsonArray);
    return result;
}

long colorTab[5][2] = {
    {0x0000FF, 0xFFFFFF}, // Blå baggrund, hvid tekst
    {0x00FF00, 0x000000}, // Grøn baggrund, sort tekst
    {0xFFFF00, 0x000000}, // Gul baggrund, sort tekst
    {0xFF0000, 0xFFFFFF}, // Rød baggrund, hvid tekst
    {0x800080, 0xFFFFFF}  // Lilla baggrund, hvid tekst
};

//---------------------utils---------------

void ELPRISER::setup() {
    std::string tz = posix_timezone(timezone_);
    ESP_LOGI("Setup", "✅ Timezone:...%s", tz.c_str());
    //time.set_timezone(timezone_);
    config.set_selector(distributor_);
    region_->publish_state(config.pricearea);
    distributor_->publish_state(config.chargeOwner);
    vat_->publish_state(config.vat);
    tax_->publish_state(config.tax);
    net_->publish_state(config.netfee);
    div_->publish_state(config.divfee);
    limit1_->publish_state(config.limittab[0]);
    limit2_->publish_state(config.limittab[1]);
    limit3_->publish_state(config.limittab[2]);
    limit4_->publish_state(config.limittab[3]);
    current_price_->publish_state(0);
    current_hour_->publish_state(0.00);
    started = true;
    ESP_LOGI("Setup", "✅ Done...");
}

void ELPRISER::loop() {
    auto time = time_->now();
    if (!time.is_valid()) return;
    tm timeinfo;
    time_t now = time.timestamp;
    localtime_r(&now, &timeinfo);
        if (getDataFlag) {
        getDataFlag = false;
        energi.getData(makeDataUrl());
        energi.getTarif(makeTarifUrl());
    }
    if (energi.waiting) {
        energi.waiting = false;
//        ESP_LOGI("ELPRISER", "Calling onDataAvailable()");
        onDataAvailable();
    }
    if (onSecond(&timeinfo)) {
        time_stamp_->publish_state(getDateTimeStringByParams(&timeinfo, "%H:%M:%S"));
    }
    if (onMinute(&timeinfo)) {
//        ESP_LOGD("TIME", "onTime called on Minute");
//        ESP_LOGD("URL", "DataURL: %s", makeDataUrl().c_str());
//        ESP_LOGD("URL", "TarifURL: %s", makeTarifUrl().c_str());
    }
    if (onHour(&timeinfo)) {
        getDataFlag = true;
//        ESP_LOGD("TIME", "onTime called on Hour");
    }
}

void ELPRISER::updateVat(float value) {
    if (started) {
//        ESP_LOGD("VAT", "------------------->new value: %0.2f", value);
        config.vat = value;
        config.saveConfig();
        onDataAvailable();
    }
}

void ELPRISER::updateDiv(float value) {
    if (started) {
//        ESP_LOGD("DIV", "------------------->new value: %0.4f", value);
        config.divfee = value;
        config.saveConfig();
        onDataAvailable();
    }
}

void ELPRISER::updateNet(float value) {
    if (started) {
//        ESP_LOGD("NET", "------------------->new value: %0.4f", value);
        config.netfee = value;
        config.saveConfig();
        onDataAvailable();
    }
}

void ELPRISER::updateTax(float value) {
    if (started) {
//        ESP_LOGD("TAX", "------------------->new value: %0.4sf", value);
        config.tax = value;
        config.saveConfig();
        onDataAvailable();
    }
}

void ELPRISER::updateLimit(int index, float value) {
    if (started) {
//        ESP_LOGD("Limit%d", "------------------->new value: %0.4sf", index, value);
        config.limittab[index] = value;
        config.saveConfig();
        onDataAvailable();
    }
}

void ELPRISER::updateRegion(const std::string &value) {
    if (started) {
//        ESP_LOGD("REGION", "Region changed: %s", value.c_str());
        config.pricearea = value;
        config.saveConfig();
        getDataFlag = true;
    }
};

void ELPRISER::updateDistributor(const std::string &value) {
    if (started) {
//        ESP_LOGD("DISTRIBUTOR", "Distributor changed: %s", value.c_str());
        config.chargeOwner = value;
        config.saveConfig();
        getDataFlag = true;
    }
};

std::string ELPRISER::makeDataUrl() {
//    std::string pricearea = config.pricearea;
    return "https://api.energidataservice.dk/dataset/Elspotprices?start=" + start() + "&end=" + end() + "&sort=HourDK&filter={\"PriceArea\":\"" + config.pricearea + "\"}";
}

std::string ELPRISER::makeTarifUrl() {
    cJSON *root = config.jDoc();
    if (!root) { ESP_LOGE("Elpriser", "❌ JSON parsing failed!"); return ""; }
    cJSON *ownerData = cJSON_GetObjectItem(root, config.chargeOwner.c_str());
    if (!ownerData) { ESP_LOGE("Elpriser", "❌ ChargeOwner not found in JSON!"); cJSON_Delete(root); return ""; }
    cJSON *glnItem = cJSON_GetObjectItem(ownerData, "gln");
    std::string gln = (glnItem && cJSON_IsString(glnItem)) ? glnItem->valuestring : "";
    cJSON *typeArray = cJSON_GetObjectItem(ownerData, "type");
    std::string chargeTypeCode = (typeArray && cJSON_IsArray(typeArray)) ? makeArr(typeArray) : "[]";
    cJSON *chargeTypeArray = cJSON_GetObjectItem(ownerData, "chargetype");
    std::string chargeType = (chargeTypeArray && cJSON_IsArray(chargeTypeArray)) ? makeArr(chargeTypeArray) : "[]";
    cJSON_Delete(root);
    return "https://api.energidataservice.dk/dataset/DatahubPricelist?offset=0&start=" +
        startYear() + "&end=" + end() + "&limit=1&filter=%7B%22GLN_Number%22:[%22" + gln +
        "%22],%22ChargeType%22:" + chargeType + ",%22ChargeTypeCode%22:" + chargeTypeCode + "%7D&sort=ChargeOwner%20DESC";
}

void ELPRISER::onDataAvailable() {
    cJSON *tarifDoc = cJSON_Parse(energi.tarifBuf.c_str());
    if (!tarifDoc) {
        ESP_LOGE("ELPRISER", "❌ Parseerror tarifBuf Length %d Data %s", energi.tarifBuf.length(), energi.tarifBuf.c_str());
        return;
    }
    cJSON *statusCodeItem = cJSON_GetObjectItem(tarifDoc, "statusCode");
    int sc = (statusCodeItem && cJSON_IsNumber(statusCodeItem)) ? statusCodeItem->valueint : 0;
    if (sc) {
        ESP_LOGE("ELPRISER", "❌ tarifBuf - StatusCode %d", sc);
        cJSON_Delete(tarifDoc);
        return;
    }
    cJSON *records = cJSON_GetObjectItem(tarifDoc, "records");
    if (!cJSON_IsArray(records)) {
        ESP_LOGE("ELPRISER", "❌ tarifBuf - Ingen records fundet!");
        cJSON_Delete(tarifDoc);
        return;
    }
    cJSON *record = cJSON_GetArrayItem(records, 0);
    if (!record) {
        ESP_LOGE("ELPRISER", "❌ tarifBuf - Mangler første record!");
        cJSON_Delete(tarifDoc);
        return;
    }
    for (int i = 0; i < 24; i++) {
        char priceKey[10];
        snprintf(priceKey, sizeof(priceKey), "Price%d", i + 1);
        cJSON *priceItem = cJSON_GetObjectItem(record, priceKey);
        if (cJSON_IsNumber(priceItem)) {
            config.ownerfee[i] = (float)priceItem->valuedouble;
        } else {
            ESP_LOGE("ELPRISER", "❌ tarifBuf - Mangler nøgle %s", priceKey);
            config.ownerfee[i] = 0.0;
        }
    }
    cJSON_Delete(tarifDoc);  // Frigør hukommelse for tarifDoc
    cJSON *dataDoc = cJSON_Parse(energi.dataBuf.c_str());
    if (!dataDoc) {
        ESP_LOGE("ELPRISER", "❌ Parseerror dataBuf. Length %d Data %s", energi.dataBuf.length(), energi.dataBuf.c_str());
        return;
    }
    cJSON *dataRecords = cJSON_GetObjectItem(dataDoc, "records");
    if (!cJSON_IsArray(dataRecords)) {
        ESP_LOGE("ELPRISER", "❌ dataBuf - Ingen records fundet!");
        cJSON_Delete(dataDoc);
        return;
    }
    std::vector<Record> recordArray;
    recordArray.resize(cJSON_GetArraySize(dataRecords));
    int index = 0;
    myTimeElement_t tm;
    cJSON *rec;
    cJSON_ArrayForEach(rec, dataRecords) {
        cJSON *hourItem = cJSON_GetObjectItem(rec, "HourDK");
        cJSON *priceItem = cJSON_GetObjectItem(rec, "SpotPriceDKK");
        if (!cJSON_IsString(hourItem) || !cJSON_IsNumber(priceItem)) {
            ESP_LOGE("ELPRISER", "❌ Ugyldigt dataformat i record!");
            continue;
        }
        const char *HourDK = hourItem->valuestring;
        sscanf(HourDK, "%4d-%02d-%02dT%02d:%02d:%02d", &tm.Year, &tm.Month, &tm.Day, &tm.Hour, &tm.Minute, &tm.Second);
        char Date[10];
        sprintf(Date, "%d/%d", tm.Day, tm.Month);
        int Hour = tm.Hour;
        recordArray[index++] = {
            strdup(HourDK),
            ((float)(priceItem->valuedouble / 1000) + config.tax + config.netfee + config.divfee + (float)(config.ownerfee[Hour])) * ((100.0f + config.vat) / 100.0f),
            tm.Hour,
            tm.Day,
            tm.Month,
            strdup(Date)
        };
    }
    std::sort(recordArray.begin(), recordArray.end(), [](const Record &a, const Record &b) {
        return strcmp(a.HourDK, b.HourDK) < 0;
    });
    for (int i = 0; i < ARRSIZE; i++) {
        config.nextfive[i].month = recordArray[i].Month;
        config.nextfive[i].day = recordArray[i].Day;
        config.nextfive[i].hour = recordArray[i].Hour;
        config.nextfive[i].price = recordArray[i].SpotPriceDKK;
        config.nextfive[i].colorIndex = colorIndex(recordArray[i].SpotPriceDKK);
    }

    std::sort(recordArray.begin(), recordArray.end(), [](const Record &a, const Record &b) {
        return a.SpotPriceDKK < b.SpotPriceDKK;
    });
    #define min(a, b) ((a) <= (b) ? (a) : (b))
    std::sort(recordArray.begin(), recordArray.begin() + min(ARRSIZE, recordArray.size()), [](const Record &a, const Record &b) {
        return strcmp(a.HourDK, b.HourDK) < 0;
    });
    for (int i = 0; i < ARRSIZE; i++) {
        config.bestfive[i].month = recordArray[i].Month;
        config.bestfive[i].day = recordArray[i].Day;
        config.bestfive[i].hour = recordArray[i].Hour;
        config.bestfive[i].price = recordArray[i].SpotPriceDKK;
        config.bestfive[i].colorIndex = colorIndex(recordArray[i].SpotPriceDKK);

    }
    next_six_json_->publish_state(timePriceArrayToJson(config.nextfive));
    best_six_json_->publish_state(timePriceArrayToJson(config.bestfive));
    color_index_->publish_state(colorIndex(config.nextfive[0].price));
    current_price_->publish_state(config.nextfive[0].price);
    current_hour_->publish_state(config.nextfive[0].hour);

    for (auto &rec : recordArray) {
        free((void*)rec.HourDK);
        free((void*)rec.Date);
    }
    cJSON_Delete(dataDoc);
}

int ELPRISER::colorIndex(float price) {
    return (price < config.limittab[0]) ? 0 : (price < config.limittab[1]) ? 1 : (price < config.limittab[2]) ? 2 : (price < config.limittab[3]) ? 3 : 4;
}

void ELPRISER::refreshOnPress() {
    ESP_LOGD("BUTTON", "✅ Refresh Button Pressed!");
    getDataFlag = true;
}

}  // namespace elpriser
}  // namespace esphome
