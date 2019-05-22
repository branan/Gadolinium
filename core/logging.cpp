#include "logging.hpp"
#include "string.hpp"

static const char* nybbles = "0123456789ABCDEF";

Logger& Logger::global() {
    static Logger logger;
    if (logger.sink() == nullptr) {
        logger.sink(Sink::global());
    }
    return logger;
}

void Logger::write(const char* str) {
    while (*str != 0) {
        m_sink->writec(*str);
        str++;
    }
}

void Logger::write(const String& str) {
    write(str.str());
}

#define WRITE_U(s) void Logger::write(uint##s##_t u) { \
    int shift = s - 4; \
    uint##s##_t mask = 0xf << shift; \
    m_sink->writec('0'); \
    m_sink->writec('x'); \
    while(mask) { \
        auto nybble = (u & mask) >> shift; \
        mask >>= 4; \
        shift -= 4; \
        m_sink->writec(nybbles[nybble]); \
    } \
}

WRITE_U(8)
WRITE_U(16)
WRITE_U(32)
WRITE_U(64)

#define WRITE_I(s) void Logger::write(int##s##_t i) { \
    int##s##_t rev = 0; \
    uint64_t digits = 0; \
    if (i < 0) { \
        m_sink->writec('-'); \
        i = 0 - i; \
    } \
    while(i) { \
        auto v = i % 10; \
        rev *= 10; \
        rev += v; \
        i /= 10; \
        digits++; \
    } \
    while(digits) { \
        auto v = rev % 10; \
        m_sink->writec('0'+v); \
        rev /= 10; \
        digits--; \
    } \
}

WRITE_I(8)
WRITE_I(16)
WRITE_I(32)
WRITE_I(64)
