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

#ifndef __CY_THREAD_HPP__
#define __CY_THREAD_HPP__

#include <vector>
#include <map>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <memory>
#include <chrono>
#include <future>
#include "CYThreadProperties.hpp"
#include "CYThread/ICYThread.hpp"
#include "CYJThread.hpp"

CYTHRAD_NAMESPACE_BEGIN

template <class T>
class CYThreadedTask : public ICYIThreadableObject
{
public:
    /**
     * Ctor - no setup occuring.
     */
    CYThreadedTask()
        : m_pInvokingObject(nullptr)
        , m_pFunctionToExecute(nullptr)
    {
    }

    /**
     * Ctor - registering the invoking object and the function that will be executed, requiring objTask execution properties.
     * @param pInvokingObjectName - The object that will be used to execute the function.
     * @param pFunctionToExecute - The function that will be executed by the object.
     */
    CYThreadedTask(T* pInvokingObjectName, void (T::* pFunctionToExecute)())
        : m_pInvokingObject(pInvokingObjectName)
        , m_pFunctionToExecute(pFunctionToExecute)
    {
    }

public:
    /**
     * Function member to fill in the invoking object and the function that it executes.
     * @param pInvokingObjectName - The object that will be used to execute the function.
     * @param pFunctionToExecute - The function that will be executed by the object.
     */
    void CreateThreadedTask(T* pInvokingObjectName, void (T::* pFunctionToExecute)())
    {
        m_pInvokingObject = pInvokingObjectName;
        m_pFunctionToExecute = pFunctionToExecute;
    }

    /**
     * Method calling the registered objects (m_pInvokingObject) function.
     * This method is called by the thread pool when the thread is nAvailable for work.
     * It is the actual function that will be executed by the thread.
     * @note This method is called by the thread pool, not by the user.
     */
    void TaskToExecute() override
    {
        if (m_pInvokingObject && m_pFunctionToExecute)
        {
            (m_pInvokingObject->*m_pFunctionToExecute)();
        }
    }

private:
    /**
     * Name of the object that object thread objTask will use.
     */
    T* m_pInvokingObject{ nullptr };

private:
    /**
     * Method inside the invoking object that will be executed.
     */
    void (T::* m_pFunctionToExecute)()
    {
        nullptr
    };
};

class CYThread
{
public:
    CYThread();
    virtual ~CYThread();

    CYThread(const CYThread&) = delete;
    CYThread& operator=(const CYThread&) = delete;
    CYThread(CYThread&&) noexcept = delete;
    CYThread& operator=(CYThread&&) noexcept = delete;

public:
    /**
     * Thread creation method.
     * @param objProps - The properties of the thread to be created.
     * @return True if the thread was created successfully, false otherwise.
     */
    virtual bool CreateThread(const CYThreadProperties& objProps);

    /**
     * Alter the threads properties, such as its stack size, id etc..
     * @param objAttributes - The properties of the thread to be changed.
     */
    virtual void ChangeThreadProperties(const CYThreadTask& objAttributes);

    /**
     * Alter the threads properties, such as its stack size, id etc.. and then execute it.
     * @param objAttributes - The properties of the thread to be changed.
     */
    virtual void ChangeThreadPropertiesandResume(const CYThreadTask& objAttributes);

    /**
     * Alter the threads properties, such as its stack size, id etc.. and then execute it.
     * @param pAttributes - The properties of the thread to be changed.
     */
    virtual void ChangeThreadPropertiesandResume(ICYIThreadableObject* pAttributes);

    /**
     * Alter the threads execution properties.
     * @param objExecutionProps - The execution properties of the thread to be changed.
     */
    virtual void ChangeThreadsExecutionProperties(CYThreadExecutionProps* objExecutionProps);

    /**
     * Controls how a thread gets executed.
     * @return The return value of the thread function.
     */
    virtual uint32_t ExecuteThread() noexcept;

    /**
     * Allows the thread to execute (run).
     * @note This method is called by the thread pool, not by the user.
     */
    virtual void ResumeThread();

    /**
     * Destroy a thread, permanently.
     * @note This method is called by the thread pool, not by the user.
     */
    virtual void TerminateThread();

    /**
     * Suspend a thread from executing.
     * @note This method is called by the thread pool, not by the user.
     */
    virtual void SuspendThread();

    /**
     * Accessor to determine if a thread is nAvailable for work?
     * @return The status of the thread.
     */
    [[nodiscard]] CYThreadStatus GetThreadAvail() const noexcept
    {
        return m_eThreadAvail.load();
    }

    /**
     * Accessor to determine if a thread is nAvailable for work?
     * @param status - The new status of the thread.
     */
    void SetThreadAvail(CYThreadStatus status);

    /**
     * Wait for a thread to complete its execution.
     * @return The return value of the thread function.
     */
    virtual uint32_t WaitForSingleObject(std::chrono::milliseconds timeout);

    /**
     * Accessor to get the thread object.
     * @return The thread object.
     */
    virtual ICYIThreadableObject* GetThreadObject() const noexcept
    {
        return m_pThreadsObject;
    }

    /**
     * Accessor to get the thread handle.
     * @return The thread handle.
     */
    virtual void* GetThreadHandle()
    {
        return m_objThreadProp.m_pThreadHandle;
    }

protected:
    /**
     * Threads static properties - from objTask to objTask.
     */
    CYThreadProperties		m_objThreadProp;
    /**
     * This contains execution data for the thread. Its callback and potential arguments.
     */
    CYThreadTask			m_objThreadsTask;
    CYThreadTask			m_objNextThreadsTask;

    /**
     * This contains execution data for the thread. Its callback and potential arguments.
     */
    ICYIThreadableObject* m_pThreadsObject{ nullptr };

    /**
     * Is the thread nAvailable?
     */
    std::atomic<CYThreadStatus> m_eThreadAvail{ CYThreadStatus::STATUS_THREAD_NOT_EXECUTING };

    std::atomic<int32_t>    m_nChangedThreadsTask{ 0 };
    std::atomic<int32_t>    m_nChangedThreadsObject{ 0 };
    ICYIThreadableObject* m_pNextThreadsObject{ nullptr };

    std::unique_ptr<CYJThread> m_ptrThread;
    std::mutex                    m_objMutex;
    std::condition_variable       m_objCondVar;
    std::atomic<bool>             m_bSuspended{ false };
};

CYTHRAD_NAMESPACE_END

#endif // __CY_THREAD_HPP__