#pragma once

#include <stddef.h>

class String {
public:
    String() = default;
    String(const char* src);

    const char* str() const {
        return _data;
    }
private:
    char* _data;
    size_t _len;
    size_t _cap;
};
