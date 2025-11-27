#include "CYThreadPCH.hpp"
#include "CYThreadWindows.hpp"
#include <thread>
#include <chrono>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <pthread.h>
#include <sched.h>
#endif

CYTHRAD_NAMESPACE_BEGIN

CYThreadWindows::CYThreadWindows() = default;

CYThreadWindows::~CYThreadWindows()
{
    TerminateThread();
}

/**
 * Thread creation method.
 * @param objCreationProps The properties of the thread to be created.
 * @return True if the thread is created successfully, false otherwise.
 */
bool CYThreadWindows::CreateThread(const CYThreadProperties& objCreationProps) noexcept
{
    try
    {
        m_ptrThread = std::make_unique<CYJThread>(ThreadFunction, this);
        return m_ptrThread != nullptr;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

/**
 * Forwarding mechanism for threads "execution" point.
 * @param pThis The CYThreadWindows object pointer.
 * @note This method is called by the thread function.
 */
void CYThreadWindows::ThreadFunction(CYThreadWindows* pThis)
{
    pThis->ExecuteThread();
}

/**
 * Controls how a thread gets execution.
 * @return The thread's execution status.
 */
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

/**
 * Alter a threads properties, such as its stack size, id etc..
 * @param objAttributes The properties of the thread to be changed.
 * @note This method can only be called before the thread is started.
 */
void CYThreadWindows::ChangeThreadProperties(const CYThreadTask& objAttributes) noexcept
{
    // No implementation needed as we use CYJThread abstraction
}

/**
 * Alter a threads properties, such as its stack size, id etc.. and then execute it.
 * @param objAttributes The properties of the thread to be changed.
 * @note This method can only be called before the thread is started.
 */
void CYThreadWindows::ChangeThreadPropertiesandResume(const CYThreadTask& objAttributes) noexcept
{
    m_objThreadsTask.funTaskToExecute = objAttributes.funTaskToExecute;
    SetThreadAvail(CYThreadStatus::STATUS_THREAD_EXECUTING);
    ResumeThread();
}

/**
 * Alter the threads properties, such as its stack size, id etc.. and then execute it
 * @param pAttributes The properties of the thread to be changed.
 * @note This method can only be called before the thread is started.
 */
void CYThreadWindows::ChangeThreadPropertiesandResume(ICYIThreadableObject* objAttributes) noexcept
{
    if (!objAttributes) return;

    m_nChangedThreadsObject.fetch_add(1);
    m_pNextThreadsObject = objAttributes;
    SetThreadAvail(CYThreadStatus::STATUS_THREAD_EXECUTING);
    ResumeThread();
}

/**
 * Allows the thread to execute.
 * @note This method can only be called after the thread is started.
 */
void CYThreadWindows::ResumeThread() noexcept
{
    std::lock_guard<std::mutex> lock(m_objMutex);
    m_bSuspended.store(false);
    m_objCondVar.notify_one();
}

/**
 * Destroy a thread.
 * @note This method can only be called before the thread is started.
 */
void CYThreadWindows::TerminateThread() noexcept
{
    m_objStopSource.request_stop();
    if (m_ptrThread && m_ptrThread->joinable())
    {
        m_ptrThread->join();
    }
}

/**
 * Suspend a thread from executing.
 * @note This method can only be called after the thread is started.
 */
void CYThreadWindows::SuspendThread() noexcept
{
    std::lock_guard<std::mutex> lock(m_objMutex);
    m_bSuspended.store(true);
}

/**
 * Alter the threads execution properties.
 * @param pExecutionProps The execution properties of the thread to be changed.
 * @note This method can only be called before the thread is started.
 */
void CYThreadWindows::ChangeThreadsExecutionProperties(CYThreadExecutionProps* pAttributes) noexcept
{
    if (!m_ptrThread || !pAttributes) return;

    std::thread::native_handle_type handle = m_ptrThread->native_handle();

    // ----------------------------------------------------
    // 1. Set thread priority (Cross-platform implementation)
    // ----------------------------------------------------
#if defined(_WIN32)
    int nPriority = THREAD_PRIORITY_NORMAL;
    switch (pAttributes->GetTasksDefinedPriority())
    {
    case CYThreadPriority::PRIORITY_THREAD_LOW:            nPriority = THREAD_PRIORITY_BELOW_NORMAL; break;
    case CYThreadPriority::PRIORITY_THREAD_NORMAL:         nPriority = THREAD_PRIORITY_NORMAL; break;
    case CYThreadPriority::PRIORITY_THREAD_HIGH:           nPriority = THREAD_PRIORITY_ABOVE_NORMAL; break;
    case CYThreadPriority::PRIORITY_THREAD_CRITICAL:       nPriority = THREAD_PRIORITY_HIGHEST; break;
    case CYThreadPriority::PRIORITY_THREAD_TIME_CRITICAL:  nPriority = THREAD_PRIORITY_TIME_CRITICAL; break;
    default: break;
    }
    SetThreadPriority(handle, nPriority);

#elif defined(__linux__)
    // Linux uses SCHED_OTHER-based priority (0~99)
    struct sched_param param {};
    int scheduler = SCHED_OTHER;

    switch (pAttributes->GetTasksDefinedPriority())
    {
    case CYThreadPriority::PRIORITY_THREAD_LOW:            param.sched_priority = 0; break;
    case CYThreadPriority::PRIORITY_THREAD_NORMAL:         param.sched_priority = 1; break;
    case CYThreadPriority::PRIORITY_THREAD_HIGH:           param.sched_priority = 5; break;
    case CYThreadPriority::PRIORITY_THREAD_CRITICAL:       param.sched_priority = 10; break;
    case CYThreadPriority::PRIORITY_THREAD_TIME_CRITICAL:  param.sched_priority = 20; break;
    default: break;
    }

    pthread_setschedparam(handle, scheduler, &param);

#elif defined(__APPLE__)
    // macOS does not allow pthread real-time priority scheduling.
    // Use QoS classes instead (Apple's recommended thread priority system).
    // More details: https://developer.apple.com
    qos_class_t qos = QOS_CLASS_DEFAULT;

    switch (pAttributes->GetTasksDefinedPriority())
    {
    case CYThreadPriority::PRIORITY_THREAD_LOW:            qos = QOS_CLASS_UTILITY; break;
    case CYThreadPriority::PRIORITY_THREAD_NORMAL:         qos = QOS_CLASS_DEFAULT; break;
    case CYThreadPriority::PRIORITY_THREAD_HIGH:           qos = QOS_CLASS_USER_INITIATED; break;
    case CYThreadPriority::PRIORITY_THREAD_CRITICAL:       qos = QOS_CLASS_USER_INTERACTIVE; break;
    case CYThreadPriority::PRIORITY_THREAD_TIME_CRITICAL:  qos = QOS_CLASS_USER_INTERACTIVE; break;
    default: break;
    }

    pthread_set_qos_class_self_np(qos, 0);

#else
    // Generic POSIX: only "nice" values are supported
    int niceVal = 0;
    switch (pAttributes->GetTasksDefinedPriority())
    {
    case CYThreadPriority::PRIORITY_THREAD_LOW:            niceVal = 10; break;
    case CYThreadPriority::PRIORITY_THREAD_NORMAL:         niceVal = 0; break;
    case CYThreadPriority::PRIORITY_THREAD_HIGH:           niceVal = -5; break;
    case CYThreadPriority::PRIORITY_THREAD_CRITICAL:       niceVal = -10; break;
    case CYThreadPriority::PRIORITY_THREAD_TIME_CRITICAL:  niceVal = -15; break;
    default: break;
    }
    setpriority(PRIO_PROCESS, 0, niceVal);
#endif

    // ----------------------------------------------------
    // 2. Set CPU affinity (Pin thread to CPU cores)
    // ----------------------------------------------------
#if defined(_WIN32)
    if (pAttributes->GetTasksDefinedProcessAffinity() == CYProcessorAffinity::AFFINITY_PROCESSOR_HARD)
    {
        // Hard affinity: force the thread to run only on specific CPUs
        SetThreadAffinityMask(handle, pAttributes->GetTasksProcessAffinityMask());
    }
    else
    {
        // Soft affinity: Set preferred (ideal) core
        SetThreadIdealProcessor(handle, pAttributes->GetTasksDefinedCore());
    }

#elif defined(__linux__)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    if (pAttributes->GetTasksDefinedProcessAffinity() == CYProcessorAffinity::AFFINITY_PROCESSOR_HARD)
    {
        // Hard affinity: use affinity mask
        uint64_t mask = pAttributes->GetTasksProcessAffinityMask();
        for (int i = 0; i < CPU_SETSIZE; ++i)
        {
            if (mask & (1ull << i)) CPU_SET(i, &cpuset);
        }
    }
    else
    {
        // Soft affinity: bind to a single preferred core
        CPU_SET(pAttributes->GetTasksDefinedCore(), &cpuset);
    }

    #if !defined(__ANDROID__)
        pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpuset);
    #endif

#elif defined(__APPLE__)
    // macOS does not support CPU affinity.
    // Thread core binding must be ignored.
    (void)handle;
    (void)pAttributes;

#else
    // Generic POSIX systems do not support affinity either.
    (void)handle;
    (void)pAttributes;

#endif
}

/**
 * Wait for the thread to complete.
 * @param timeout The maximum time to wait for the thread to complete.
 * @return The thread's execution status.
 */
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