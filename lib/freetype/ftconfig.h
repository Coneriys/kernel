#ifndef FTCONFIG_H
#define FTCONFIG_H

// Kernel-specific configuration for FreeType
#define FT_CONFIG_OPTION_NO_ASSEMBLER
#define FT_CONFIG_OPTION_NO_POSTSCRIPT_NAMES
#define FT_CONFIG_OPTION_DISABLE_STREAM_SUPPORT

// Basic type definitions for kernel environment
#include <stdint.h>
#include <stddef.h>

typedef int8_t   FT_Int8;
typedef uint8_t  FT_UInt8;
typedef int16_t  FT_Int16;
typedef uint16_t FT_UInt16;
typedef int32_t  FT_Int32;
typedef uint32_t FT_UInt32;
typedef int64_t  FT_Int64;
typedef uint64_t FT_UInt64;

typedef int      FT_Int;
typedef unsigned FT_UInt;
typedef long     FT_Long;
typedef unsigned long FT_ULong;
typedef long     FT_F26Dot6;
typedef int      FT_Error;
typedef void*    FT_Pointer;

#define FT_CHAR_BIT  8
#define FT_INT_MAX   0x7FFFFFFF
#define FT_UINT_MAX  0xFFFFFFFFU
#define FT_LONG_MAX  0x7FFFFFFFL
#define FT_ULONG_MAX 0xFFFFFFFFUL

// Memory management hooks for kernel
#define FT_ALLOC(ptr, size)       ptr = kmalloc(size)
#define FT_FREE(ptr)              kfree(ptr)
#define FT_REALLOC(ptr, old, new) ptr = krealloc(ptr, new)

// Disable file I/O - we'll use memory buffers
#define FT_CONFIG_OPTION_DISABLE_STREAM_SUPPORT

#endif // FTCONFIG_H