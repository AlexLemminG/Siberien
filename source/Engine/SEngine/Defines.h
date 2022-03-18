#pragma once

#define SE_SYMBOL_EXPORT __declspec(dllexport)
#define SE_SYMBOL_IMPORT __declspec(dllimport)

#if SE_BUILD
#   define SE_SHARED_LIB_API SE_SYMBOL_EXPORT
#else
#   define SE_SHARED_LIB_API SE_SYMBOL_IMPORT
#endif // SE_SHARED_LIB_*

#if defined(__cplusplus)
#   define SE_CPP_API SE_SHARED_LIB_API
#else
#   define SE_CPP_API SE_SHARED_LIB_API
#endif // defined(__cplusplus)

#if defined(_MSC_VER) && defined(SE_DEBUG)
//force optimization of this code section in special debug configuration
#define SE_DEBUG_OPTIMIZE_ON __pragma(optimize("gt", on))    //enable optimizations
#define SE_DEBUG_OPTIMIZE_OFF __pragma(optimize("", on))     //reset optimization settings
#else
#define SE_DEBUG_OPTIMIZE_ON
#define SE_DEBUG_OPTIMIZE_OFF
#endif

#ifndef SE_RETAIL
#define SE_USE_OPTICK
#define SE_HAS_DEBUG
#endif

#ifndef SE_BUILD
#define RYML_SHARED
#endif

#ifndef SE_RETAIL
#define SDL_ASSERT_LEVEL 2
#endif//TODO else