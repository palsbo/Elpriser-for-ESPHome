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
#include "config.h"
#include "energi.h"

//#define RESET_CONFIG
#define STORAGE_NAMESPACE "config"
#define SAFE_PTR(ptr) ((uintptr_t) &(ptr) != 0 ? ptr : nullptr)
#define ARRSIZE 6

namespace esphome {
namespace elpriser {

class ELPRISER : public Component {
    public:
        void setup() override;
        void loop() override;
        void dump_config()  override {};
        //void set_time(time::RealTimeClock *time) { time_ = time;  }
        //void set_time(time::RealTimeClock *time) {  }
        void set_timezone(const std::string &timezone) { timezone_ = timezone; }
        std::string get_timezone() const { return timezone_; }
        CONFIG config;
        ENERGI energi;
        void updateVat(float value);
        void updateDiv(float value);
        void updateTax(float value);
        void updateNet(float value);
        void updateLimit(int index, float value);
        void updateRegion(const std::string &value);
        void updateDistributor(const std::string &value);
        void refreshOnPress();
        int colorIndex(float price);
    private:
        time::RealTimeClock *time_;
        std::string timezone_ = "Europe/Copenhagen";  // Standardværdi
        std::string makeDataUrl();
        std::string makeTarifUrl();

    protected:
        bool started = false;
        bool getDataFlag = true;
        void onDataAvailable();
};


}  // namespace elpriser
}  // namespace esphome
