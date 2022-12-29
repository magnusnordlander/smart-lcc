/*
    Main handler for the Raspberry Pi Pico RP2040
    Copyright (c) 2021 Earle F. Philhower, III <earlephilhower@yahoo.com>
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef SMART_LCC_MULTICORESUPPORT_H
#define SMART_LCC_MULTICORESUPPORT_H

#include <hardware/clocks.h>
#include <hardware/irq.h>
#include <pico/unique_id.h>
#include <hardware/watchdog.h>
#include <hardware/structs/rosc.h>
#include <hardware/structs/systick.h>
#include <pico/multicore.h>
#include <pico/util/queue.h>
#include <malloc.h>

extern "C" volatile bool __otherCoreIdled;

// Support nested IRQ disable/re-enable
#define maxIRQs 15
static uint32_t _irqStackTop[2] = { 0, 0 };
static uint32_t _irqStack[2][maxIRQs];

extern "C" void interrupts();

extern "C" void noInterrupts();


class MulticoreSupport {
public:
    MulticoreSupport() { /* noop */ };
    ~MulticoreSupport() { /* noop */ };

    void begin(int cores) {
        constexpr int FIFOCNT = 8;
        if (cores == 1) {
            _multicore = false;
            return;
        }
        mutex_init(&_idleMutex);
        queue_init(&_queue[0], sizeof(uint32_t), FIFOCNT);
        queue_init(&_queue[1], sizeof(uint32_t), FIFOCNT);
        _multicore = true;
    }

    void registerCore() {
            multicore_fifo_clear_irq();
            irq_set_exclusive_handler(SIO_IRQ_PROC0 + get_core_num(), _irq);
            irq_set_enabled(SIO_IRQ_PROC0 + get_core_num(), true);
    }

    void push(uint32_t val) {
        while (!push_nb(val)) { /* noop */ }
    }

    bool push_nb(uint32_t val) {
        // Push to the other FIFO
        return queue_try_add(&_queue[get_core_num() ^ 1], &val);
    }

    uint32_t pop() {
        uint32_t ret;
        while (!pop_nb(&ret)) { /* noop */ }
        return ret;
    }

    bool pop_nb(uint32_t *val) {
        // Pop from my own FIFO
        return queue_try_remove(&_queue[get_core_num()], val);
    }

    int available() {
        return queue_get_level(&_queue[get_core_num()]);
    }

    void idleOtherCore() {
        if (!_multicore) {
            return;
        }
        mutex_enter_blocking(&_idleMutex);
        __otherCoreIdled = false;
        multicore_fifo_push_blocking(_GOTOSLEEP);
        while (!__otherCoreIdled) { /* noop */ }
    }

    void resumeOtherCore() {
        if (!_multicore) {
            return;
        }
        mutex_exit(&_idleMutex);
        __otherCoreIdled = false;

        // Other core will exit busy-loop and return to operation
        // once __otherCoreIdled == false.
    }

    void clear() {
        uint32_t val;

        while (queue_try_remove(&_queue[0], &val)) {
            tight_loop_contents();
        }

        while (queue_try_remove(&_queue[1], &val)) {
            tight_loop_contents();
        }
    }

private:
    static void __no_inline_not_in_flash_func(_irq)() {
            multicore_fifo_clear_irq();
            noInterrupts(); // We need total control, can't run anything
            while (multicore_fifo_rvalid()) {
                if (_GOTOSLEEP == multicore_fifo_pop_blocking()) {
                    __otherCoreIdled = true;
                    while (__otherCoreIdled) { /* noop */ }
                    break;
                }
            }
            interrupts();
    }

    bool _multicore = false;
    mutex_t _idleMutex;
    queue_t _queue[2];
    static constexpr uint32_t _GOTOSLEEP = 0xC0DED02E;
};


#endif //SMART_LCC_MULTICORESUPPORT_H
