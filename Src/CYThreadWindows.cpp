#include "CYThreadPCH.hpp"
#include "CYThreadWindows.hpp"
#include <thread>
#include <chrono>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

CYTHRAD_NAMESPACE_BEGIN

CYThreadWindows::CYThreadWindows() = default;

CYThreadWindows::~CYThreadWindows()
{
    TerminateThread();
}

bool CYThreadWindows::CreateThread(const CYThreadProperties& objCreationProps) noexcept
{
    try
    {
        m_ptrThread = std::make_unique<std::jthread>(ThreadFunction, this);
        return m_ptrThread != nullptr;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

void CYThreadWindows::ThreadFunction(CYThreadWindows* pThis)
{
    pThis->ExecuteThread();
}

uint32_t CYThreadWindows::ExecuteThread() noexcept
{
    while (!m_objStopSource.get_token().stop_requested())
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

void CYThreadWindows::ChangeThreadProperties(const CYThreadTask& objAttributes) noexcept
{
    // No implementation needed as we use std::jthread
}

void CYThreadWindows::ChangeThreadPropertiesandResume(const CYThreadTask& objAttributes) noexcept
{
    m_objThreadsTask.funTaskToExecute = objAttributes.funTaskToExecute;
    SetThreadAvail(CYThreadStatus::STATUS_THREAD_EXECUTING);
    ResumeThread();
}

void CYThreadWindows::ChangeThreadPropertiesandResume(ICYIThreadableObject* objAttributes) noexcept
{
    if (!objAttributes) return;

    m_nChangedThreadsObject.fetch_add(1);
    m_pNextThreadsObject = objAttributes;
    SetThreadAvail(CYThreadStatus::STATUS_THREAD_EXECUTING);
    ResumeThread();
}

void CYThreadWindows::ResumeThread() noexcept
{
    std::lock_guard<std::mutex> lock(m_objMutex);
    m_bSuspended.store(false);
    m_objCondVar.notify_one();
}

void CYThreadWindows::TerminateThread() noexcept
{
    m_objStopSource.request_stop();
    if (m_ptrThread && m_ptrThread->joinable())
    {
        m_ptrThread->join();
    }
}

void CYThreadWindows::SuspendThread() noexcept
{
    std::lock_guard<std::mutex> lock(m_objMutex);
    m_bSuspended.store(true);
}

void CYThreadWindows::ChangeThreadsExecutionProperties(CYThreadExecutionProps* pAttributes) noexcept
{
    if (!m_ptrThread) return;

    // Set thread priority
    int nPriority = 0;
    switch (pAttributes->GetTasksDefinedPriority())
    {
    case CYThreadPriority::PRIORITY_THREAD_LOW:
        nPriority = 1;
        break;
    case CYThreadPriority::PRIORITY_THREAD_NORMAL:
        nPriority = 0;
        break;
    case CYThreadPriority::PRIORITY_THREAD_HIGH:
        nPriority = -1;
        break;
    case CYThreadPriority::PRIORITY_THREAD_CRITICAL:
        nPriority = -2;
        break;
    case CYThreadPriority::PRIORITY_THREAD_TIME_CRITICAL:
        nPriority = -3;
        break;
    default:
        nPriority = 0;
        break;
    }

    // Set thread affinity
    if (pAttributes->GetTasksDefinedProcessAffinity() == CYProcessorAffinity::AFFINITY_PROCESSOR_HARD)
    {
        // Set hard affinity
        std::thread::native_handle_type handle = m_ptrThread->native_handle();
#ifdef _WIN32
        SetThreadAffinityMask(handle, pAttributes->GetTasksProcessAffinityMask());
#else
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(pAttributes->GetTasksDefinedCore(), &cpuset);
        pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpuset);
#endif
    }
    else
    {
        // Set soft affinity
        std::thread::native_handle_type handle = m_ptrThread->native_handle();
#ifdef _WIN32
        SetThreadIdealProcessor(handle, pAttributes->GetTasksDefinedCore());
#else
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(pAttributes->GetTasksDefinedCore(), &cpuset);
        pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpuset);
#endif
    }
}

#undef max
uint32_t CYThreadWindows::WaitForSingleObject(std::chrono::milliseconds timeout) noexcept
{
    if (!m_ptrThread) return 0;

    auto start = std::chrono::steady_clock::now();
    while (true)
    {
        if (m_ptrThread->joinable())
        {
            if (timeout == std::chrono::milliseconds::max())
            {
                m_ptrThread->join();
                return 0;
            }
            else
            {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
                if (elapsed >= timeout)
                {
                    return 1; // Timeout
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        else
        {
            return 0; // Thread completed
        }
    }
}

CYTHRAD_NAMESPACE_END