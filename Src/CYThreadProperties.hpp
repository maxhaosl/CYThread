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

#ifndef __CY_THREAD_PROPERTIES_HPP__
#define __CY_THREAD_PROPERTIES_HPP__

#include <vector>
#include <map>
#include <deque>
#include <stdint.h>
#include "CYPlatformSpecifier.hpp"
#include "CYThread/CYThreadDefine.hpp"

CYTHRAD_NAMESPACE_BEGIN

class CYThreadProperties
{
public:
    CYThreadProperties();

public:
    /**
     * Function member to set the stack size of a particular thread.
     * @param stackSize: the stack size of a particular thread.
     * @return void
     */
    void SetStackSize(int stackSize);

    /**
     * Function member to populate the threads properties.
     * @param platformId: the platform id of the current system.
     * @return void
     */
    void CreateProperties(const CYPlatformId& platformId);
public:
    /**
     * Stack size for a particular thread.
     */
    uint32_t m_nStackSize;

    /**
     * A particular threads handle.
     */
    void* m_pThreadHandle = nullptr;

    /**
     * A particular threads id.
     */
    uint32_t m_nThreadId;
};

CYTHRAD_NAMESPACE_END

#endif //__CY_THREAD_PROPERTIES_HPP__