//
// Created by Magnus Nordlander on 2021-08-20.
//

#ifndef FIRMWARE_PICOQUEUE_H
#define FIRMWARE_PICOQUEUE_H

extern "C" {
#include "pico/util/queue.h"
}

template <class T> class PicoQueue {
public:
    explicit PicoQueue(uint count);
    PicoQueue(uint count, int spinlock_num);
    ~PicoQueue();

    uint getLevelUnsafe();
    uint getLevel();
    bool isEmpty();
    bool isFull();
    bool tryAdd(T *element);
    bool tryRemove(T *element);
    bool tryPeek(T *element);
    void addBlocking(T *element);
    void removeBlocking(T *element);
    void peekBlocking(T *element);
private:
    queue_t _queue{};
    int spinlock_num;
};

#include "PicoQueue.inl"

#endif //FIRMWARE_PICOQUEUE_H
