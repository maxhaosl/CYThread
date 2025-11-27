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

#ifndef __CY_THREAD_FOUNDATION_HPP__
#define __CY_THREAD_FOUNDATION_HPP__

#include "CYThread.hpp"
#include "CYThreadPool.hpp"
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "CYThread/CYThreadDefine.hpp"

CYTHRAD_NAMESPACE_BEGIN

class CYThreadFoundation
{
public:
    CYThreadFoundation();
    virtual ~CYThreadFoundation();

    CYThreadFoundation(const CYThreadFoundation&) = delete;
    CYThreadFoundation& operator=(const CYThreadFoundation&) = delete;
    CYThreadFoundation(CYThreadFoundation&&) noexcept = default;
    CYThreadFoundation& operator=(CYThreadFoundation&&) noexcept = default;

public:
    /**
     * Check if thread pool is empty.
     * @return true if thread pool is empty, otherwise false.
     */
    [[nodiscard]] bool IsEmpty() const noexcept;

    /**
     * Check if any threads are executing tasks.
     * @return true if any threads are executing tasks, otherwise false.
     */
    [[nodiscard]] bool AreAnyThreadsWorking() const noexcept;

    /**
     * Terminate all working threads.
     * @note This function will block until all threads are terminated.
     */
    void TerminateAllWorkingThreads() noexcept;

    /**
     * Suspend all executing threads.
     * @note This function will block until all threads are suspended.
     */
    void SuspendAllWorkingThreads() noexcept;

    /**
     * Pause all working threads.
     * @note This function will block until all threads are paused.
     */
    void PauseAllWorkingThreads() noexcept;

    /**
     * Pause specific working thread.
     * @param pInvokingObject The thread to be paused.
     * @note This function will block until the specified thread is paused.
     */
    void PauseWorkingThread(ICYIThreadableObject* pInvokingObject) noexcept;

    /**
     * Resume all working threads.
     * @note This function will block until all threads are resumed.
     */
    void ResumeAllWorkingThreads() noexcept;

    /**
     * Resume specific working thread.
     * @param pInvokingObject The thread to be resumed.
     * @note This function will block until the specified thread is resumed.
     */
    void ResumeWorkingThread(ICYIThreadableObject* pInvokingObject) noexcept;

    /**
     * Terminate specific working thread.
     * @param pInvokingObject The thread to be terminated.
     * @note This function will block until the specified thread is terminated.
     */
    void TerminateWorkingThread(ICYIThreadableObject* pInvokingObject) noexcept;

    /**
     * Get status of specific working thread.
     * @param pInvokingObject The thread to be queried.
     * @return The status of the specified thread.
     */
    [[nodiscard]] CYThreadStatus GetWorkingThreadStatus(ICYIThreadableObject* pInvokingObject) const noexcept;

    /**
     * Get execution properties.
     * @param objTep The execution properties to be queried.
     * @note This function will block until the specified thread is resumed.
     */
    void GetTaskExecutionProps(CYThreadExecutionProps& objTep) const noexcept;

    /**
     * Submit a objTask to the pool.
     * @param pInvokingObject The task to be submitted.
     * @return true if the task is submitted successfully, otherwise false.
     */
    bool SubmitTask(ICYIThreadableObject* pInvokingObject) noexcept;

    /**
     * Process submitted tasks.
     * @note This function will be called by CYThread periodically.
     */
    void Distribute() noexcept;

    /**
     * Create thread pool.
     * @param iMaxThread The maximum number of threads in the pool.
     */
    void CreateThreadPool(int iMaxThread = 10) noexcept;

    /**
     * Get nAvailable thread.
     * @param bRemove Remove thread from list if true.
     */
    [[nodiscard]] CYThread* GetAvailThread(bool bRemove = false) const noexcept;

#ifdef _DEBUG
    /**
     * Get thread pool instance (debug only).
     * @return The thread pool instance.
     */
    [[nodiscard]] CYThreadPool& GetTPInstance() noexcept;
#endif

    /**
     * Wait for thread completion.
     * @param pInvokingObject The thread to be waited.
     */
    [[nodiscard]] uint32_t WaitForSingleObject(ICYIThreadableObject* pInvokingObject, uint32_t dwMiliseconds) const noexcept;

private:
    /**
     * Clean up all allocations.
     */
    void Shutdown() noexcept;

private:
    /**
     * Thread pool singleton.
     */
    static std::unique_ptr<CYThreadPool> m_ptrThreadPool;

    /**
     * Platform specifier.
     */
    CYPlatformSpecifier m_objPlatformSpecifier;
};

CYTHRAD_NAMESPACE_END

#endif //__CY_THREAD_FOUNDATION_HPP__