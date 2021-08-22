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
