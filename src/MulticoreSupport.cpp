//
// Created by Magnus Nordlander on 2022-12-21.
//

#include "MulticoreSupport.h"

extern "C" void interrupts() {
    auto core = get_core_num();
    if (!_irqStackTop[core]) {
        // ERROR
        return;
    }
    restore_interrupts(_irqStack[core][--_irqStackTop[core]]);
}

extern "C" void noInterrupts() {
    auto core = get_core_num();
    if (_irqStackTop[core] == maxIRQs) {
        // ERROR
        panic("IRQ stack overflow");
    }

    _irqStack[core][_irqStackTop[core]++] = save_and_disable_interrupts();
}
