#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <windows.h>

#if __MINGW_DEBUGBREAK_IMPL
#include <intrin.h>
#define debug_break() __debugbreak();
#else
#define debug_break() __builtin_trap();
#endif

#define TRUE 1
#define FALSE 0
#define internal static
#define local_persist static
#define global_variable static
#define Assert(Expression) if(!(Expression)) {debug_break();}
#define _offsetof(TYPE, NAME) ((size_t)(&((TYPE* )0)->NAME))
#define FREE(handle) free(handle); handle = 0;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef size_t memory_index;

typedef float real32;
typedef double real64;

#ifdef DLL_EXPORT
// Exports
#ifdef _MSC_VER
#define MAPI __declspec(dllexport)
#elif   defined(__MINGW32__) || defined(__clang__)
#define MAPI __attribute__((__dllexport__))
#else
#error "Bad Luck Ma'am, only Windows Support" 
#endif
#else

// Imports
#ifdef _MSC_VER
#define MAPI __declspec(dllimport)
#else 
#define MAPI __attribute((__dllimport__))
#endif
#endif