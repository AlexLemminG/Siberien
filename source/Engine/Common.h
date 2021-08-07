#pragma once

#include <cassert>
#include "Config.h"
#include "SDL_assert.h"

#define ASSERT(cond) SDL_assert(cond)

#define INIT_SYSTEM(InitFunc) \
if (!InitFunc()) {	\
	ASSERT(false);				\
}

#define LOG_ERROR(msg) \
std::cout << msg << std::endl;