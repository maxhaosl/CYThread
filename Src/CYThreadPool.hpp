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

#ifndef __CY_THREAD_POOL_HPP__
#define __CY_THREAD_POOL_HPP__

#include <vector>
#include <map>
#include <deque>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <thread>
#include "CYThread.hpp"
#include "CYThreadPoolProperties.hpp"
#include "CYThread/ICYThread.hpp"

CYTHRAD_NAMESPACE_BEGIN

class CYThreadPool : public ICYThreadPool
{
public:
    CYThreadPool();
    ~CYThreadPool();

    CYThreadPool(const CYThreadPool&) = delete;
    CYThreadPool& operator=(const CYThreadPool&) = delete;
    CYThreadPool(CYThreadPool&&) noexcept = default;
    CYThreadPool& operator=(CYThreadPool&&) noexcept = default;

public:
    // Create thread pool with specified platform and max threads
    [[nodiscard]] bool CreateThreadPool(const CYPlatformId& eThreadType, int iMaxThread = 10) noexcept override;

    // Submit a objTask to the pool
    [[nodiscard]] bool SubmitTask(const CYThreadTask& objTask) noexcept override;

    // Submit an object objTask to the pool
    [[nodiscard]] bool SubmitTask(ICYIThreadableObject* pInvokingObject) noexcept override;

    // Check if any threads are working
    [[nodiscard]] bool AreAnyThreadsWorking() const noexcept override;

    // Terminate all working threads
    void TerminateAllWorkingThreads() noexcept override;

    // Suspend all working threads
    void SuspendAllWorkingThreads() noexcept override;

    // Get objTask count
    [[nodiscard]] int GetTaskCount() const noexcept override
    {
        return static_cast<int>(m_lstTTask.size());
    }

    // Get missed objTask count
    [[nodiscard]] int GetTasksMissedCount() const noexcept override
    {
        return static_cast<int>(m_lstTTaskMiss.size());
    }

    // Get nAvailable thread count
    [[nodiscard]] int GetThreadAvailableCount() const noexcept override;

    // Get max thread count
    [[nodiscard]] int GetMaxThreadCount() const noexcept override;

    // Get count of threads with specific status
    [[nodiscard]] int GetSpecificThreadStatusCount(CYThreadStatus eThreadType) const noexcept override;

    // Check if pool is empty
    [[nodiscard]] bool IsPoolEmpty() const noexcept override;

    // Pause all working threads
    void PauseAllWorkingThreads() noexcept override;

    // Pause specific working thread
    void PauseWorkingThread(ICYIThreadableObject* pInvokingObject) noexcept override;

    // Resume all working threads
    void ResumeAllWorkingThreads() noexcept override;

    // Resume specific working thread
    void ResumeWorkingThread(ICYIThreadableObject* pInvokingObject) noexcept override;

    // Terminate specific working thread
    void TerminateWorkingThread(ICYIThreadableObject* pInvokingObject) noexcept override;

    // Get status of specific working thread
    [[nodiscard]] CYThreadStatus GetWorkingThreadStatus(ICYIThreadableObject* pInvokingObject) const noexcept override;

    // Wait for thread completion
    [[nodiscard]] uint32_t WaitForSingleObject(ICYIThreadableObject* pInvokingObject, uint32_t timeout) const noexcept override;

    // Get nAvailable thread
    [[nodiscard]] CYThread* GetAvailThread(bool bRemove = false) noexcept;

public:
    // Process object objTask list
    void processObjectTaskList() noexcept;

    // Shutdown thread pool
    [[nodiscard]] bool Shutdown() noexcept;

private:
    // Thread list types
    using ThreadList = std::vector<std::unique_ptr<CYThread>>;
    using ThreadListIT = ThreadList::iterator;

    // Task list types
    using TaskList = std::deque<CYThreadTask>;
    using TaskListIT = TaskList::iterator;

    // Object objTask list types
    using TTaskList = std::deque<ICYIThreadableObject*>;
    using TTaskListIT = TTaskList::iterator;

    // Thread pool data
    ThreadList m_lstThread;
    TaskList m_lstTask, m_lstTaskMiss;
    TTaskList m_lstTTask, m_lstTTaskMiss;
    CYThreadPoolProperties m_objTPProps;

    // Synchronization
    mutable std::mutex m_objMutex;
    mutable std::condition_variable m_objCondVar;
    std::atomic<bool> m_bShutdown{ false };

private:
    // Remove objTask from objTask list
    [[nodiscard]] bool RemoveTask(TaskListIT tlIT) noexcept;

    // Clean up objTask list
    [[nodiscard]] bool CleanTaskList() noexcept;

    // Set objTask pool lock status
    void SetTaskPoolLock(bool status) noexcept;

    // Get objTask pool lock status
    [[nodiscard]] bool GetTaskPoolLock() const noexcept;

    // Promote cleanup threads to nAvailable
    void PromoteCleanupToAvailability() noexcept;
};

CYTHRAD_NAMESPACE_END

#endif // __CY_THREAD_POOL_HPP__