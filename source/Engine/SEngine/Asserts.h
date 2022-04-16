#pragma once

#include "SDL_assert.h"
#include "StringUtils.h"

#define SE_ASSERT_HANDLER(condition, message, ...) \
    do { \
        while ( !(condition) ) { \
            auto msg = FormatString(message, __VA_ARGS__); \
            if (msg.empty()) { msg = #condition; } else { msg = FormatString("%s': '%s", #condition, msg.c_str()); } \
            static struct SDL_AssertData sdl_assert_data = { \
                0, 0, msg.c_str(), 0, 0, 0, 0 \
            }; \
            const SDL_AssertState sdl_assert_state = SDL_ReportAssertion(&sdl_assert_data, SDL_FUNCTION, SDL_FILE, SDL_LINE); \
            if (sdl_assert_state == SDL_ASSERTION_RETRY) { \
                continue; /* go again. */ \
            } else if (sdl_assert_state == SDL_ASSERTION_BREAK) { \
                SDL_TriggerBreakpoint(); \
            } \
            break; /* not retrying. */ \
        } \
    } while (SDL_NULL_WHILE_LOOP_CONDITION)

//check sdl_assert.h for explanation
#define SE_DISABLED_ASSERT(condition, ...) \
    do { (void) sizeof ((condition)); } while (SDL_NULL_WHILE_LOOP_CONDITION)

/* Enable various levels of assertions. */
#if SDL_ASSERT_LEVEL == 0 || SDL_ASSERT_LEVEL == 1   /* retail settings */
#define ASSERT(condition, ...) SE_DISABLED_ASSERT(condition, __VA_ARGS__)
#define ASSERT_FAILED(...) SE_DISABLED_ASSERT(false, __VA_ARGS__)
#else /* release settings. */
#define ASSERT(condition, ...) SE_ASSERT_HANDLER(condition, __VA_ARGS__)
#define ASSERT_FAILED(...) SE_ASSERT_HANDLER(false, __VA_ARGS__)
#endif