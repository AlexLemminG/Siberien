#pragma once

#include <cassert>
#include <iostream>
#include "Config.h"
#include "SDL_assert.h"
#include "optick.h"

#define ASSERT(cond) SDL_assert(cond)

#define INIT_SYSTEM(InitFunc) \
if (!InitFunc()) {	\
	ASSERT(false);				\
}

#define LOG_ERROR(msg) \
std::cout << msg << std::endl;

#define SAFE_DELETE(var) \
if(var) \
{delete var; var = nullptr;}

template<typename ... Args>
std::string FormatString(const std::string & format, Args ... args)
{
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
    if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
    auto size = static_cast<size_t>(size_s);
    auto buf = std::make_unique<char[]>(size);
    std::snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

template<typename ... Args>
void LogError(const std::string& format, Args ... args) {
    std::cout << FormatString(format, args...) << std::endl;
}


class BinaryBuffer {
public:
    BinaryBuffer() {}
    BinaryBuffer(std::vector<uint8_t>&& readBuffer) : buffer(readBuffer){}

    std::vector<uint8_t>&& ReleaseData() {
        return std::move(buffer);
    }

    template<typename T>
    void Read(T& val) {
        memcpy(&val, &buffer[currentReadOffset], sizeof(T));
        currentReadOffset += sizeof(T);
    }
    template<>
    void Read<std::string>(std::string& val) {
        int length;
        Read(length);
        if (length) {
            val = std::string((char*)&buffer[currentReadOffset], length);
        }
        else {
            val = "";
        }
        currentReadOffset += length;
    }
    template<typename T>
    void Write(const T& val) {
        auto offset = buffer.size();
        buffer.resize(buffer.size() + sizeof(T));
        memcpy(&buffer[offset], &val, sizeof(T));
    }

    template<>
    void Write<std::string>(const std::string& val) {
        Write((int)val.size());
        if (val.size()) {
            Write((uint8_t*)&val[0], val.size());
        }
    }

    void Write(const uint8_t* buf, int size) {
        auto offset = buffer.size();
        buffer.resize(buffer.size() + size);
        memcpy(&buffer[offset], buf, size);
    }
    void Read(uint8_t* buf, int size) {
        memcpy(buf, &buffer[currentReadOffset], size);
        currentReadOffset += size;
    }
private:
    int currentReadOffset = 0;
    std::vector<uint8_t> buffer;
};