//
// Created by Magnus Nordlander on 2021-08-20.
//

#ifndef FIRMWARE_PICOQUEUE_H
#define FIRMWARE_PICOQUEUE_H

#include <pico/util/queue.h>
#include <hardware/sync.h>

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

template<class T>
PicoQueue<T>::PicoQueue(uint count) {
    spinlock_num = spin_lock_claim_unused(true);
    queue_init_with_spinlock(&_queue, sizeof(T), count, spinlock_num);
}

template<class T>
PicoQueue<T>::PicoQueue(uint count, int _spinlock_num): spinlock_num(-1) {
    queue_init_with_spinlock(&_queue, sizeof(T), count, _spinlock_num);
}

template<class T>
PicoQueue<T>::~PicoQueue() {
    queue_free(&_queue);

    if (spinlock_num != -1) {
        spin_lock_unclaim(spinlock_num);
    }
}

template<class T>
uint PicoQueue<T>::getLevelUnsafe() {
    return queue_get_level_unsafe(&_queue);
}

template<class T>
uint PicoQueue<T>::getLevel() {
    return queue_get_level(&_queue);
}

template<class T>
bool PicoQueue<T>::isEmpty() {
    return queue_is_empty(&_queue);
}

template<class T>
bool PicoQueue<T>::isFull() {
    return queue_is_full(&_queue);
}

template<class T>
bool PicoQueue<T>::tryAdd(T *element) {
    return queue_try_add(&_queue, (void*)element);
}

template<class T>
bool PicoQueue<T>::tryRemove(T *element) {
    return queue_try_remove(&_queue, (void*)element);
}

template<class T>
bool PicoQueue<T>::tryPeek(T *element) {
    return queue_try_peek(&_queue, (void*)element);
}

template<class T>
void PicoQueue<T>::addBlocking(T *element) {
    queue_add_blocking(&_queue, (void*)element);
}

template<class T>
void PicoQueue<T>::removeBlocking(T *element) {
    queue_remove_blocking(&_queue, (void*)element);
}

template<class T>
void PicoQueue<T>::peekBlocking(T *element) {
    queue_peek_blocking(&_queue, (void*)element);
}


#endif //FIRMWARE_PICOQUEUE_H
