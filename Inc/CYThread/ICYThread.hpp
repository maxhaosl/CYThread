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

#ifndef __I_CY_THREAD_HPP__
#define __I_CY_THREAD_HPP__

#include "CYThreadDefine.hpp"

#include <thread>
#include <atomic>
#include <functional>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <sched.h>
#include <unistd.h>
#endif

CYTHRAD_NAMESPACE_BEGIN

/**
 * Thread Execution Properties.
 */
class CYTHREAD_API CYThreadExecutionProps
{
public:
    CYThreadExecutionProps()
        : m_eTasksProcessorAffinity(CYProcessorAffinity::AFFINITY_PROCESSOR_SOFT)
        , m_eTasksPriority(CYThreadPriority::PRIORITY_THREAD_NORMAL)
        , m_nTasksIdealCore(0)
        , m_nTasksProcessAffinityMask(0)
    {
    }
    ~CYThreadExecutionProps() = default;

    CYThreadExecutionProps(const CYThreadExecutionProps&) = default;
    CYThreadExecutionProps& operator=(const CYThreadExecutionProps&) = default;
    CYThreadExecutionProps(CYThreadExecutionProps&&) noexcept = default;
    CYThreadExecutionProps& operator=(CYThreadExecutionProps&&) noexcept = default;

public:
    /**
     * Method to get the process affinity.
     */
    [[nodiscard]] CYProcessorAffinity	GetTasksDefinedProcessAffinity(void) const noexcept
    {
        return m_eTasksProcessorAffinity;
    }

    /**
     * Method to get the tasks execution priority.
     */
    [[nodiscard]] CYThreadPriority	GetTasksDefinedPriority(void) const noexcept
    {
        return m_eTasksPriority;
    }

    /**
     * Method to get the tasks process element.
     */
    [[nodiscard]] int GetTasksDefinedCore(void) const noexcept
    {
        return m_nTasksIdealCore;
    }

    /**
     * Method to get the tasks process affinity mask.
     */
    [[nodiscard]] uint32_t GetTasksProcessAffinityMask(void) const noexcept
    {
        return m_nTasksProcessAffinityMask;
    }

    /**
     * Method to create an affinity mask.
     */
    void CreateTasksProcessorAffinity(void)
    {
        m_nTasksProcessAffinityMask = GetProcessorAffinity(m_nTasksIdealCore);
    }

    /**
     * Set tasks processor affinity.
     */
    void SetTasksProcessAffinity(const CYProcessorAffinity& eProcAffinity) noexcept
    {
        m_eTasksProcessorAffinity = eProcAffinity;
    }

    /**
     * Set tasks priority.
     */
    void SetTasksPriority(const CYThreadPriority& eProcPriority) noexcept
    {
        m_eTasksPriority = eProcPriority;
    }

    /**
     * Set a tasks ideal core.
     */
    void SetTasksCore(const int& nProcCore) noexcept
    {
        m_nTasksIdealCore = nProcCore;
    }

    /**
     * Fill execution properties.
     */
    void CreateExecutionProp(const CYProcessorAffinity& eProcAffinity, const CYThreadPriority& eProcPriority, const int& nProcCore) noexcept
    {
        m_eTasksProcessorAffinity = eProcAffinity;
        m_eTasksPriority = eProcPriority;
        m_nTasksIdealCore = nProcCore;
    }
private:
    /**
     * Local method to determine processor affinity mask based on desired core.
     */
    [[nodiscard]] uint32_t GetProcessorAffinity(int nDesiredCore) const noexcept
    {
        if (nDesiredCore < 0)
        {
            return 0; ///< Invalid core number
        }

#ifdef _WIN32
        SYSTEM_INFO m_objSysInfo;
        GetSystemInfo(&m_objSysInfo);
        const int nMaxCores = static_cast<int>(m_objSysInfo.dwNumberOfProcessors);
#else
        const int nMaxCores = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
#endif

        if (nDesiredCore >= nMaxCores)
        {
            return 0; ///< Core number exceeds nAvailable cores
        }

        return 1u << nDesiredCore;
    }

protected:
    /**
     * Defines what core the objTask/job will execute on.
     */
    uint32_t				m_nTasksProcessAffinityMask{ 0 };

    /**
     * The tasks processor affinity.
     */
    CYProcessorAffinity		m_eTasksProcessorAffinity{ CYProcessorAffinity::AFFINITY_PROCESSOR_SOFT };

    /**
     * The tasks priority.
     */
    CYThreadPriority		m_eTasksPriority{ CYThreadPriority::PRIORITY_THREAD_NORMAL };

    /**
     * The tasks ideal core.
     */
    int						m_nTasksIdealCore{ 0 };
};

/**
 * Thread Object.
 */
class CYTHREAD_API ICYIThreadableObject
{
public:
    ICYIThreadableObject() = default;
    virtual ~ICYIThreadableObject() = default;

    ICYIThreadableObject(const ICYIThreadableObject&) = delete;
    ICYIThreadableObject& operator=(const ICYIThreadableObject&) = delete;
    ICYIThreadableObject(ICYIThreadableObject&&) noexcept = default;
    ICYIThreadableObject& operator=(ICYIThreadableObject&&) noexcept = default;

public:
    /**
     * Method that is overrideable and maps an outside function which is normally a callback.
     */
    virtual void TaskToExecute() = 0;

    /**
     * Function member to retrieve the id for an object for access through object registry.
     */
    [[nodiscard]] uint32_t GetObjectId() const noexcept
    {
        return m_nObjectId;
    }

    /**
     * Method to retrieve the execution properties.
     */
    [[nodiscard]] virtual CYThreadExecutionProps* GetExecutionProps()
    {
        return &m_objTaskExecutionProps;
    }

protected:
    /**
     * Tasks execution properties.
     */
    CYThreadExecutionProps m_objTaskExecutionProps;

    /**
     * Used to register an object with the object registry.
     */
    uint32_t m_nObjectId;
};

/**
 * Thread Pool Interface.
 */
class CYTHREAD_API ICYThreadPool
{
public:
    ICYThreadPool() = default;
    virtual ~ICYThreadPool() = default;

    ICYThreadPool(const ICYThreadPool&) = delete;
    ICYThreadPool& operator=(const ICYThreadPool&) = delete;
    ICYThreadPool(ICYThreadPool&&) noexcept = default;
    ICYThreadPool& operator=(ICYThreadPool&&) noexcept = default;

public:
    /**
     * Create thread pool with specified platform and max threads.
     */
    virtual bool CreateThreadPool(const CYPlatformId& eThreadType, int iMaxThread = 10) = 0;

    /**
     * Submit a objTask to the pool.
     */
    virtual bool SubmitTask(const CYThreadTask& objTask) = 0;

    /**
     * Submit an object objTask to the pool.
     */
    virtual bool SubmitTask(ICYIThreadableObject* pInvokingObject) = 0;

    /**
     * Check if any threads are working.
     */
    virtual bool AreAnyThreadsWorking() const noexcept = 0;

    /**
     * Terminate all working threads.
     */
    virtual void TerminateAllWorkingThreads() noexcept = 0;

    /**
     * Suspend all working threads.
     */
    virtual void SuspendAllWorkingThreads() noexcept = 0;

    /**
     * Get objTask count.
     */
    virtual int GetTaskCount() const noexcept = 0;

    /**
     * Get missed objTask count.
     */
    virtual int GetTasksMissedCount() const noexcept = 0;

    /**
     * Get nAvailable thread count.
     */
    virtual int GetThreadAvailableCount() const noexcept = 0;

    /**
     * Get max thread count.
     */
    virtual int GetMaxThreadCount() const noexcept = 0;

    /**
     * Get count of threads with specific status.
     */
    virtual int GetSpecificThreadStatusCount(CYThreadStatus eThreadType) const noexcept = 0;

    /**
     * Check if pool is empty.
     */
    virtual bool IsPoolEmpty() const noexcept = 0;

    /**
     * Pause all working threads.
     */
    virtual void PauseAllWorkingThreads() noexcept = 0;

    /**
     * Pause specific working thread.
     */
    virtual void PauseWorkingThread(ICYIThreadableObject* pInvokingObject) noexcept = 0;

    /**
     * Resume all working threads.
     */
    virtual void ResumeAllWorkingThreads() noexcept = 0;

    /**
     * Resume specific working thread.
     */
    virtual void ResumeWorkingThread(ICYIThreadableObject* pInvokingObject) noexcept = 0;

    /**
     * Terminate specific working thread.
     */
    virtual void TerminateWorkingThread(ICYIThreadableObject* pInvokingObject) noexcept = 0;

    /**
     * Get status of specific working thread.
     */
    virtual CYThreadStatus GetWorkingThreadStatus(ICYIThreadableObject* pInvokingObject) const noexcept = 0;

    /**
     * Wait for thread completion.
     */
    virtual uint32_t WaitForSingleObject(ICYIThreadableObject* pInvokingObject, uint32_t timeout) const noexcept = 0;
};

CYTHRAD_NAMESPACE_END

#endif // __I_CY_THREAD_HPP__