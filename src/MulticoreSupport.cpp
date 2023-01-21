//
// Created by Magnus Nordlander on 2022-12-21.
//

#include "MulticoreSupport.h"

// Support nested IRQ disable/re-enable
#define maxIRQs 15
static uint32_t _irqStackTop[2] = { 0, 0 };
static uint32_t _irqStack[2][maxIRQs];

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
