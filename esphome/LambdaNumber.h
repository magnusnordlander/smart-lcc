//
// Created by Magnus Nordlander on 2023-01-05.
//

#ifndef LCC_ESPHOME_LAMBDANUMBER_H
#define LCC_ESPHOME_LAMBDANUMBER_H

#include "esphome.h"

class LambdaNumber : public esphome::number::Number {
public:
    explicit LambdaNumber(std::function<void(float)> control_f): control_f(control_f) {}

    void control(float value) override {
        this->control_f(value);
        ESP_LOGD("custom", "Changing number");
    }

private:
    std::function<void(float)> control_f;
};

#endif //LCC_ESPHOME_LAMBDANUMBER_H
