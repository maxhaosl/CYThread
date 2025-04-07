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

#ifndef __CY_THREAD_FACTORY_HPP__
#define __CY_THREAD_FACTORY_HPP__

#include "CYThread/ICYThread.hpp"

CYTHRAD_NAMESPACE_BEGIN

class CYTHREAD_API CYThreadFactory
{
public:
    CYThreadFactory();
    virtual ~CYThreadFactory();

public:
    ICYThreadPool* CreateThreadPool();
    void ReleaseThreadPool(ICYThreadPool* pThreadPool);
};

CYTHRAD_NAMESPACE_END

#endif // __CY_THREAD_FACTORY_HPP__

#ifndef __CY_THREAD_WINDOWS_HPP__
#define __CY_THREAD_WINDOWS_HPP__

#include "CYThread.hpp"
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <stop_token>
#include <future>
#include "CYThread/CYThreadDefine.hpp"

CYTHRAD_NAMESPACE_BEGIN

class CYThreadWindows : public CYThread
{
public:
    CYThreadWindows();
    virtual ~CYThreadWindows();

    CYThreadWindows(const CYThreadWindows&) = delete;
    CYThreadWindows& operator=(const CYThreadWindows&) = delete;
    CYThreadWindows(CYThreadWindows&&) noexcept = default;
    CYThreadWindows& operator=(CYThreadWindows&&) noexcept = default;

public:
    //Thread creation method
    [[nodiscard]] virtual bool CreateThread(const CYThreadProperties& objCreationProps) noexcept override;

    //Alter a threads properties, such as its stack size, id etc..
    virtual void ChangeThreadProperties(const CYThreadTask& objAttributes) noexcept override;

    //Alter a threads properties, such as its stack size, id etc.. and then execute it
    virtual void ChangeThreadPropertiesandResume(const CYThreadTask& objAttributes) noexcept override;

    //Alter the threads properties, such as its stack size, id etc.. and then execute it
    virtual void ChangeThreadPropertiesandResume(ICYIThreadableObject* pAttributes) noexcept override;

    //Alter the threads execution properties
    virtual void ChangeThreadsExecutionProperties(CYThreadExecutionProps* pExecutionProps) noexcept override;

    //Controls how a thread gets execution
    [[nodiscard]] virtual uint32_t ExecuteThread() noexcept override;

    //Allows the thread to execute
    virtual void ResumeThread() noexcept override;

    //Destroy a thread
    virtual void TerminateThread() noexcept override;

    //Suspend a thread from executing
    virtual void SuspendThread() noexcept override;

    [[nodiscard]] virtual uint32_t WaitForSingleObject(std::chrono::milliseconds timeout) noexcept override;

private:
    //Forwarding mechanism for threads "execution" point
    static void ThreadFunction(CYThreadWindows* pThis);

private:
    std::atomic<int32_t> m_nChangedThreadsObject{ 0 };
    ICYIThreadableObject* m_pNextThreadsObject{ nullptr };
    std::unique_ptr<std::jthread> m_ptrThread;
    std::mutex m_objMutex;
    std::condition_variable m_objCondVar;
    std::atomic<bool> m_bSuspended{ false };
    std::stop_source m_objStopSource;
};

CYTHRAD_NAMESPACE_END

#endif //__CY_THREAD_WINDOWS_HPP__