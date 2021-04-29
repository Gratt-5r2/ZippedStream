#pragma once
#include <Windows.h>
#include <stdio.h>

#ifdef _ZLIB
#define ZLIB_WINAPI
#define ZLIB_DLL
#define ZLIB_INTERNAL
#include "zlib.h"
#pragma comment(lib, "zlibstat.lib")
#endif

#ifdef _UNION_DEFINITIONS
typedef unsigned int uint, uint_t, bool_t;
typedef int int_t;
typedef unsigned long ulong;
#define Null nullptr
#define True (1)
#define False (0)
#define shi_malloc malloc
#define shi_realloc realloc
#define shi_free free
#define Invalid (-1)
#endif


#ifdef _ZIPPEDSTREAM_DLL
#ifdef _ZIPPEDSTREAM_INTERNAL
#define ZSTREAMAPI __declspec(dllexport)
#else
#define ZSTREAMAPI __declspec(dllimport)
#endif
#else
#ifndef _ZIPPEDSTREAM_INTERNAL
#ifndef _EXE
#pragma comment(lib, "ZippedStream.lib")
#endif
#endif
#define ZSTREAMAPI
#endif

#ifndef __UNION_ARRAY_H__
#include "Array\Array.h"
#endif
#include "Thread\Thread.h"
#include "ZippedStream.h"