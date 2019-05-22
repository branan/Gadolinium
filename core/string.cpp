#include "string.hpp"

extern "C" char* strcpy(char* dest, const char* src) {
    auto ret = dest;
    while(*src) {
        *dest = *src;
        dest++;
        src++;
    }
    return ret;
}

extern "C" size_t strlen(const char* str) {
    size_t len = 0;
    while (*str) {
        len++;
        str++;
    }
    return len;
}

String::String(const char* src) {
    _cap = _len = strlen(src);
    _data = new char[_len+1];
    strcpy(_data, src);
}
