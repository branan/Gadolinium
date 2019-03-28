#pragma once

#include "stddef.h"
#include "stdint.h"

class Sink {
public:
    virtual void writec(char) = 0;

    static Sink* global();
};

class Logger {
public:
    static Logger& global();

    Sink* sink() const { return m_sink; }
    void sink(Sink* s) { m_sink = s; }

    void write(const char*);
    void write(uint8_t);
    void write(uint16_t);
    void write(uint32_t);
    void write(uint64_t);
    void write(int8_t);
    void write(int16_t);
    void write(int32_t);
    void write(int64_t);
private:
    Sink* m_sink;
};

template <typename T>
inline void klog(T& value) {
    Logger::global().write(value);
}

template <typename T0, typename T1, typename ... Ts>
inline void klog(T0 value, T1 next, Ts... rest) {
    Logger::global().write(value);
    klog(next, rest...);
}
