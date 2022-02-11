//
// Created by Magnus Nordlander on 2021-08-20.
//

#ifndef FIRMWARE_MOVINGAVERAGE_H
#define FIRMWARE_MOVINGAVERAGE_H



template <class T> class MovingAverage {
public:
    explicit MovingAverage(uint16_t num);
    ~MovingAverage();

    void addValue(T);
    double average();
private:
    uint16_t _limit;
    uint16_t _head = 0;
    T* _array;
    bool _wrapped = false;
};

template<class T>
MovingAverage<T>::MovingAverage(uint16_t num): _limit(num) {
    _array = (T*)calloc(num, sizeof(T));
}

template<class T>
MovingAverage<T>::~MovingAverage() {
    free(_array);
}

template<class T>
void MovingAverage<T>::addValue(T val) {
    _array[_head++] = val;
    if (_head >= _limit) {
        _head = 0;
        _wrapped = true;
    }
}

template<class T>
double MovingAverage<T>::average() {
    double sum = 0.f;

    uint16_t limit = _limit;
    if (!_wrapped) {
        limit = _head;
    }

    if (limit == 0) {
        return 0.f;
    }

    for (uint16_t i = 0; i < limit; ++i) {
        sum += (double)_array[i];
    }

    return sum / limit;
}


#endif //FIRMWARE_MOVINGAVERAGE_H
