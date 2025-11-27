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

#ifndef __CY_SYSTEM_DESC_HPP__
#define __CY_SYSTEM_DESC_HPP__

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <chrono>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <sys/sysinfo.h>
#if defined(__i386__) || defined(__x86_64__)
#include <cpuid.h>
#define CYTHREAD_HAS_LINUX_CPUID 1
#else
#define CYTHREAD_HAS_LINUX_CPUID 0
#endif
#endif

#ifndef CYTHREAD_HAS_LINUX_CPUID
#define CYTHREAD_HAS_LINUX_CPUID 0
#endif

#include "CYThread/CYThreadDefine.hpp"

CYTHRAD_NAMESPACE_BEGIN

class CYSystemDescription
{
public:
    CYSystemDescription();
    ~CYSystemDescription() = default;

public:
    CYSystemDescription(const CYSystemDescription&) = delete;
    CYSystemDescription& operator=(const CYSystemDescription&) = delete;
    CYSystemDescription(CYSystemDescription&&) noexcept = default;
    CYSystemDescription& operator=(CYSystemDescription&&) noexcept = default;

    [[nodiscard]] int GetNumberProcessors() const noexcept;
    [[nodiscard]] int GetHyperThreadAvailability() const noexcept;
    [[nodiscard]] uint32_t GetMemoryLoad() const noexcept;
    [[nodiscard]] uint32_t GetBytesPhysicalMemory() const noexcept;
    [[nodiscard]] int32_t DoMemoryExced(uint32_t dwMemValue) const noexcept;

private:
    virtual void Startup();

protected:
    static constexpr uint32_t m_nCYSystemMB = 1024 * 1024;  // 1MB in bytes

private:
    uint32_t m_nBytesPhysicalRam{ 0 };
    uint32_t m_nMemoryLoad{ 0 };

#if defined(_WIN32)
    MEMORYSTATUSEX m_objMemInfo{};
    SYSTEM_INFO m_objSystemInfo{};
#elif defined(__linux__)
    struct sysinfo m_objSysInfo
    {
    };
#endif
};

CYTHRAD_NAMESPACE_END

#endif //__CY_SYSTEM_DESC_HPP__