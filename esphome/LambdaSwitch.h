//
// Created by Magnus Nordlander on 2023-01-05.
//

#ifndef LCC_ESPHOME_LAMBDASWITCH_H
#define LCC_ESPHOME_LAMBDASWITCH_H

#include "esphome.h"
#include <functional>

class LambdaSwitch : public esphome::switch_::Switch {
public:
    explicit LambdaSwitch(std::function<void(bool)> write_state_f): write_state_f(write_state_f) {}

    void write_state(bool state) override {
        this->write_state_f(state);
    }

private:
    std::function<void(bool)> write_state_f;
};

#endif //LCC_ESPHOME_LAMBDASWITCH_H
