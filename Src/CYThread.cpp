#include "CYThreadPCH.hpp"
#include "CYThread.hpp"
#include <thread>
#include <chrono>
#include <future>

#ifdef _WIN32
#include <windows.h>
#endif

CYTHRAD_NAMESPACE_BEGIN

CYThread::CYThread() = default;

CYThread::~CYThread()
{
    if (m_ptrThread)
    {
        TerminateThread();
    }
}

void CYThread::SetThreadAvail(CYThreadStatus Available)
{
    m_eThreadAvail.store(Available, std::memory_order_release);
}

bool CYThread::CreateThread(const CYThreadProperties& objProps)
{
    try
    {
        m_ptrThread = std::make_unique<std::jthread>([this](std::stop_token stoken) {
            ThreadFunction();
            });
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

void CYThread::ThreadFunction()
{
    while (true)
    {
        if (m_nChangedThreadsObject.load(std::memory_order_acquire) != 0)
        {
            m_pThreadsObject = m_pNextThreadsObject;
            m_nChangedThreadsObject.fetch_sub(1, std::memory_order_release);
            m_pNextThreadsObject = nullptr;
        }

        if (m_nChangedThreadsTask.load(std::memory_order_acquire) != 0)
        {
            m_objThreadsTask = m_objNextThreadsTask;
            m_nChangedThreadsTask.fetch_sub(1, std::memory_order_release);
            m_objNextThreadsTask = { nullptr };
        }

        if (m_pThreadsObject)
        {
            ChangeThreadsExecutionProperties(m_pThreadsObject->GetExecutionProps());
            m_pThreadsObject->TaskToExecute();
            SetThreadAvail(CYThreadStatus::STATUS_THREAD_PURGING);
            m_pThreadsObject = nullptr;
        }

        if (m_objThreadsTask.funTaskToExecute)
        {
            ChangeThreadsExecutionProperties(m_pThreadsObject->GetExecutionProps());
            m_objThreadsTask.funTaskToExecute(m_objThreadsTask.pArgList, m_objThreadsTask.bDelete);
            SetThreadAvail(CYThreadStatus::STATUS_THREAD_PURGING);
            m_objThreadsTask = { nullptr };
        }

        {
            std::unique_lock<std::mutex> lock(m_objMutex);
            m_bSuspended.store(true, std::memory_order_release);
            m_objCondVar.wait(lock, [this] {
                return !m_bSuspended.load(std::memory_order_acquire);
                });
        }
    }
}

void CYThread::ChangeThreadProperties(const CYThreadTask& objAttributes)
{
    // Implementation depends on what properties need to be changed
    // Now handled through thread-local storage or execution context
}

void CYThread::ChangeThreadPropertiesandResume(const CYThreadTask& objAttributes)
{
    m_nChangedThreadsTask.fetch_add(1, std::memory_order_release);
    //m_objThreadsTask = objAttributes;
    m_objNextThreadsTask = objAttributes;
    SetThreadAvail(CYThreadStatus::STATUS_THREAD_EXECUTING);
    ResumeThread();
}

void CYThread::ChangeThreadPropertiesandResume(ICYIThreadableObject* pAttributes)
{
    m_nChangedThreadsObject.fetch_add(1, std::memory_order_release);
    m_pNextThreadsObject = pAttributes;
    SetThreadAvail(CYThreadStatus::STATUS_THREAD_EXECUTING);
    ResumeThread();
}

uint32_t CYThread::ExecuteThread() noexcept
{
    while (!m_ptrThread->get_stop_token().stop_requested())
    {
        if (m_nChangedThreadsObject.load() != 0)
        {
            m_pThreadsObject = m_pNextThreadsObject;
            m_nChangedThreadsObject.fetch_sub(1);
            m_pNextThreadsObject = nullptr;
        }

        if (m_pThreadsObject)
        {
            //Alter the threads execution properties
            ChangeThreadsExecutionProperties(m_pThreadsObject->GetExecutionProps());
            //Execute the associated objTask
            m_pThreadsObject->TaskToExecute();
            SetThreadAvail(CYThreadStatus::STATUS_THREAD_PURGING);
            m_pThreadsObject = nullptr;
        }

        {
            std::unique_lock<std::mutex> lock(m_objMutex);
            m_bSuspended.store(true);
            m_objCondVar.wait(lock, [this] { return !m_bSuspended.load(); });
        }
    }

    return 0;
}

void CYThread::ResumeThread()
{
    {
        std::lock_guard<std::mutex> lock(m_objMutex);
        m_bSuspended.store(false, std::memory_order_release);
    }
    m_objCondVar.notify_one();
}

void CYThread::TerminateThread()
{
    if (m_ptrThread && m_ptrThread->joinable())
    {
        m_ptrThread->request_stop();
        m_ptrThread->join();
    }
}

void CYThread::SuspendThread()
{
    m_bSuspended.store(true, std::memory_order_release);
}

void CYThread::ChangeThreadsExecutionProperties(CYThreadExecutionProps* pAttributes)
{
    if (!m_ptrThread) return;

    auto handle = m_ptrThread->native_handle();

    if (pAttributes->GetTasksDefinedProcessAffinity() == CYProcessorAffinity::AFFINITY_PROCESSOR_HARD)
    {
#if defined(_WIN32)
        SetThreadAffinityMask(handle, pAttributes->GetTasksProcessAffinityMask());
#elif defined(__linux__)
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        for (int i = 0; i < 32; i++)
        {
            if (pAttributes->GetTasksProcessAffinityMask() & (1 << i))
            {
                CPU_SET(i, &cpuset);
            }
        }
        pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpuset);
#endif
    }

#if defined(_WIN32)
    int threadPriority = THREAD_PRIORITY_NORMAL;
    switch (pAttributes->GetTasksDefinedPriority())
    {
    case CYThreadPriority::PRIORITY_THREAD_LOW:
        threadPriority = THREAD_PRIORITY_LOWEST;
        break;
    case CYThreadPriority::PRIORITY_THREAD_NORMAL:
        threadPriority = THREAD_PRIORITY_NORMAL;
        break;
    case CYThreadPriority::PRIORITY_THREAD_HIGH:
        threadPriority = THREAD_PRIORITY_ABOVE_NORMAL;
        break;
    case CYThreadPriority::PRIORITY_THREAD_CRITICAL:
        threadPriority = THREAD_PRIORITY_HIGHEST;
        break;
    case CYThreadPriority::PRIORITY_THREAD_TIME_CRITICAL:
        threadPriority = THREAD_PRIORITY_TIME_CRITICAL;
        break;
    }
    SetThreadPriority(handle, threadPriority);
#elif defined(__linux__)
    int policy;
    struct sched_param param;
    pthread_getschedparam(handle, &policy, &param);

    switch (pAttributes->GetTasksDefinedPriority())
    {
    case CYThreadPriority::PRIORITY_THREAD_LOW:
        param.sched_priority = sched_get_priority_min(policy);
        break;
    case CYThreadPriority::PRIORITY_THREAD_NORMAL:
        param.sched_priority = (sched_get_priority_max(policy) +
            sched_get_priority_min(policy)) / 2;
        break;
    case CYThreadPriority::PRIORITY_THREAD_HIGH:
    case CYThreadPriority::PRIORITY_THREAD_CRITICAL:
        param.sched_priority = sched_get_priority_max(policy) - 1;
        break;
    case CYThreadPriority::PRIORITY_THREAD_TIME_CRITICAL:
        param.sched_priority = sched_get_priority_max(policy);
        break;
    }
    pthread_setschedparam(handle, policy, &param);
#endif
}

#undef max

uint32_t CYThread::WaitForSingleObject(std::chrono::milliseconds timeout)
{
    if (!m_ptrThread) return 0;

    try
    {
        if (timeout == std::chrono::milliseconds::max())
        {
            m_ptrThread->join();
            return 0;
        }
        else
        {
            // Use a promise/future pair to implement the wait
            std::promise<void> objPromise;
            std::future<void> future = objPromise.get_future();

            // Create a temporary thread to wait for the main thread
            std::jthread waitThread([this, &objPromise, timeout]() {
                if (m_ptrThread->joinable())
                {
                    m_ptrThread->join();
                }
                objPromise.set_value();
                });

            // Wait for either the timeout or the thread completion
            auto status = future.wait_for(timeout);
            return (status == std::future_status::ready) ? 0 : 1;
        }
    }
    catch (const std::exception&)
    {
        return 2;
    }
}

CYTHRAD_NAMESPACE_END