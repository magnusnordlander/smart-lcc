//
// Created by Magnus Nordlander on 2023-01-05.
//

#ifndef LCC_ESPHOME_LAMBDANUMBER_H
#define LCC_ESPHOME_LAMBDANUMBER_H

#include "esphome.h"

class LambdaNumber : public esphome::number::Number {
public:
    void control(float value) override {
        ESP_LOGD("custom", "Changing number");
    }
};

#endif //LCC_ESPHOME_LAMBDANUMBER_H
