/*
 * CYThread License
 * -----------
 *
 * CYThread is licensed under the terms of the MIT license reproduced below.
 * This means that CYThread is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 *
 *
 * ===============================================================================
 *
 * Copyright (C) 2023-2025 ShiLiang.Hao <newhaosl@163.com>, foobra<vipgs99@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * ===============================================================================
 */
 /*
  * AUTHORS:  ShiLiang.Hao <newhaosl@163.com>, foobra<vipgs99@gmail.com>
  * VERSION:  1.0.0
  * PURPOSE:  A cross-platform efficient and stable thread pool library.
  * CREATION: 2025.04.07
  * LCHANGE:  2025.04.07
  * LICENSE:  Expat/MIT License, See Copyright Notice at the begin of this file.
  */

#ifndef __CY_THREAD_DEFINE_HPP__
#define __CY_THREAD_DEFINE_HPP__

#include <stdint.h>
#include <functional>

#define CYTHRAD_NAMESPACE_BEGIN			namespace cry {
#define CYTHRAD_NAMESPACE				cry
#define CYTHRAD_NAMESPACE_END			}
#define CYTHRAD_NAMESPACE_USE			using namespace	cry

#if defined(__MINGW32__)
#    define CYTHREAD_MINGW_OS
#elif defined(_WIN32)
#    define CYTHREAD_WIN_OS
#elif defined(unix) || defined(__unix__) || defined(__unix)
#    define CYTHREAD_UNIX_OS
#elif defined(__APPLE__) || defined(__MACH__)
#    define CYTHREAD_MAC_OS
#elif defined(__FreeBSD__)
#    define CYTHREAD_FREE_BSD_OS
#elif defined(__ANDROID__)
#    define CYTHREAD_ANDROID_OS
#endif

#if defined(__clang__)
#    define CYTHREAD_CLANG_COMPILER
#elif defined(__GNUC__) || defined(__GNUG__)
#    define CYTHREAD_GCC_COMPILER
#elif defined(_MSC_VER)
#    define CYTHREAD_MSVC_COMPILER
#endif

#if !defined(NDEBUG) || defined(_DEBUG)
#    define CYTHREAD_DEBUG_MODE
#endif

#if defined(CYTHREAD_WIN_OS)
#    if defined(CYTHREAD_EXPORTS)
#        define CYTHREAD_API __declspec(dllexport)
#    else
#        define CYTHREAD_API __declspec(dllimport)
#    endif
#elif (defined(CRCPP_EXPORT_API) || defined(CRCPP_IMPORT_API)) && __has_cpp_attribute(gnu::visibility)
#    define CYTHREAD_API __attribute__((visibility("default")))
#endif

#if !defined(CYTHREAD_API)
#    define CYTHREAD_API
#endif

CYTHRAD_NAMESPACE_BEGIN

/**
 * Platform ID.
 */
    enum CYPlatformId
{
    CY_PLATFORM_WINDOWS = 0,
    CY_PLATFORM_NONE = 1,
};

/**
 * Thread status.
 */
enum class CYThreadStatus : uint8_t
{
    STATUS_THREAD_NOT_EXECUTING = 0,		// Not executing, ready to execute
    STATUS_THREAD_EXECUTING = 1,		// Currently executing
    STATUS_THREAD_PURGING = 2,		// STATUS_THREAD_PURGING state
    STATUS_THREAD_PAUSING = 3,		// STATUS_THREAD_PAUSING state
    STATUS_THREAD_NONE = 4,
};

/**
 * Thread priority level.
 */
enum class CYThreadPriority : uint8_t
{
    PRIORITY_THREAD_LOW = 0,		//PRIORITY_THREAD_LOW
    PRIORITY_THREAD_NORMAL = 1,		//PRIORITY_THREAD_NORMAL
    PRIORITY_THREAD_HIGH = 2,		//Above PRIORITY_THREAD_NORMAL
    PRIORITY_THREAD_CRITICAL = 3,		//PRIORITY_THREAD_HIGH
    PRIORITY_THREAD_TIME_CRITICAL = 4,		//Time PRIORITY_THREAD_CRITICAL
};

/**
 * Processor affinity type.
 */
enum class CYProcessorAffinity : uint8_t
{
    AFFINITY_PROCESSOR_SOFT = 0,
    AFFINITY_PROCESSOR_HARD,
    AFFINITY_PROCESSOR_UNDEFINED,
};

/**
 * Thread Task Struct.
 */
struct CYThreadTask
{
    //Threads execution callback
    std::function<void(void*, bool)> funTaskToExecute;
    //Threads execution arguments
    void* pArgList{ nullptr };
    //Reserved deletion flag
    bool bDelete{ false };
};

CYTHRAD_NAMESPACE_END

#endif // __CY_THREAD_DEFINE_HPP__