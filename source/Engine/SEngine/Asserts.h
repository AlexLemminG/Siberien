#pragma once

#include "SDL_assert.h"

#define ASSERT(cond) SDL_assert(cond)
#define ASSERT_FAILED(message) SDL_assert(false && message)