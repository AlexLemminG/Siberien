#pragma once

#include <cassert>
#include <iostream>
#include "Config.h"
#include "SDL_assert.h"

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
