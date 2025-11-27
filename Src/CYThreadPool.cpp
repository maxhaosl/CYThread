#include "CYThreadPCH.hpp"
#include "CYThreadPool.hpp"
#include "CYThread.hpp"
#include "CYThreadProperties.hpp"
#include <thread>
#include <chrono>
#include <stdexcept>

CYTHRAD_NAMESPACE_BEGIN

/**
 * Constructor.
 */
CYThreadPool::CYThreadPool()
{
}

CYThreadPool::~CYThreadPool()
{
    Shutdown();
}

/**
 * Shutdown thread pool.
 * @return True if success, false otherwise.
 * @note This function will terminate all working threads and release resources.
 */
bool CYThreadPool::Shutdown() noexcept
{
    // Stop the distribution thread first (before acquiring the main mutex)
    StopDistributionThread();

    std::lock_guard<std::mutex> lock(m_objMutex);
    m_bShutdown.store(true, std::memory_order_release);
    m_objCondVar.notify_all();

    for (auto& objThread : m_lstThread)
    {
        if (objThread)
        {
            objThread->TerminateThread();
        }
    }

    m_lstThread.clear();
    m_lstTask.clear();
    m_lstTaskMiss.clear();
    m_lstTTask.clear();
    m_lstTTaskMiss.clear();

    return true;
}

/**
 * Create thread pool with specified platform and max threads.
 * @param eThreadType Platform type.
 * @param iMaxThread Max thread count.
 * @return True if success, false otherwise.
 */
bool CYThreadPool::CreateThreadPool(const CYPlatformId& eThreadType, int iMaxThread) noexcept
{
    try
    {
        std::lock_guard<std::mutex> lock(m_objMutex);

        // Set thread pool properties
        m_objTPProps.SetMaxTasks(25);
        m_objTPProps.SetMaxThreads(iMaxThread);
        m_objTPProps.SetTaskPoolLock(false);

        // Create threads
        for (int i = 0; i < m_objTPProps.GetMaxThreadCount(); i++)
        {
            auto thread = std::make_unique<CYThread>();
            CYThreadProperties objThreadProps;
            objThreadProps.CreateProperties(eThreadType);

            if (thread->CreateThread(objThreadProps))
            {
                m_lstThread.push_back(std::move(thread));
            }
        }

        // Start the task distribution thread
        StartDistributionThread();

        return !m_lstThread.empty();
    }
    catch (const std::exception&)
    {
        return false;
    }
}

/**
 * Submit a objTask to the pool.
 * @param objTask Task object.
 * @return True if success, false otherwise.
 */
bool CYThreadPool::SubmitTask(const CYThreadTask& objTask) noexcept
{
    std::lock_guard<std::mutex> lock(m_objMutex);
    if (!m_objTPProps.GetTaskPoolLock() && m_lstTask.size() <= m_objTPProps.GetMaxTasks())
    {
        m_lstTask.push_front(objTask);
        m_objCondVar.notify_one();
        return true;
    }
    return false;
}

/**
 * Submit a objTask to the pool.
 * @param objTask Task object.
 * @return True if success, false otherwise.
 */
bool CYThreadPool::SubmitTask(ICYIThreadableObject* pInvokingObject) noexcept
{
    if (!pInvokingObject) return false;

    std::lock_guard<std::mutex> lock(m_objMutex);
    if (!m_objTPProps.GetTaskPoolLock() && m_lstTTask.size() <= m_objTPProps.GetMaxTasks())
    {
        m_lstTTask.push_front(pInvokingObject);
        m_objCondVar.notify_one();
        return true;
    }
    return false;
}

/**
 * Process object objTask list.
 * @note This function will be called by CYThread periodically.
 * @note This function will be called in a thread-safe context.
 */
void CYThreadPool::processObjectTaskList() noexcept
{
    // Process missed tasks
    if (!m_lstTTaskMiss.empty())
    {
        for (auto it = m_lstTTaskMiss.begin(); it != m_lstTTaskMiss.end(); )
        {
            if (auto pWorker = GetAvailThread())
            {
                pWorker->ChangeThreadPropertiesandResume(*it);
                it = m_lstTTaskMiss.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    // Process new tasks
    for (auto it = m_lstTTask.begin(); it != m_lstTTask.end(); )
    {
        if (auto pWorker = GetAvailThread())
        {
            pWorker->ChangeThreadPropertiesandResume(*it);
            it = m_lstTTask.erase(it);
        }
        else
        {
            m_lstTTaskMiss.push_front(*it);
            it = m_lstTTask.erase(it);
        }
    }

    // Process function tasks (missed first)
    if (!m_lstTaskMiss.empty())
    {
        for (auto it = m_lstTaskMiss.begin(); it != m_lstTaskMiss.end(); )
        {
            if (auto pWorker = GetAvailThread())
            {
                pWorker->ChangeThreadPropertiesandResume(*it);
                it = m_lstTaskMiss.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    for (auto it = m_lstTask.begin(); it != m_lstTask.end(); )
    {
        if (auto pWorker = GetAvailThread())
        {
            pWorker->ChangeThreadPropertiesandResume(*it);
            it = m_lstTask.erase(it);
        }
        else
        {
            m_lstTaskMiss.push_front(*it);
            it = m_lstTask.erase(it);
        }
    }

    PromoteCleanupToAvailability();
}

/**
 * Get nAvailable thread.
 * @param bRemove Remove thread from list if true.
 * @return nAvailable thread.
 */
CYThread* CYThreadPool::GetAvailThread(bool bRemove) noexcept
{
    std::lock_guard<std::mutex> lock(m_objMutex);

    for (auto it = m_lstThread.begin(); it != m_lstThread.end(); ++it)
    {
        if ((*it)->GetThreadAvail() == CYThreadStatus::STATUS_THREAD_NOT_EXECUTING)
        {
            auto pThread = it->get();
            if (bRemove)
                m_lstThread.erase(it);

            return pThread;
        }
    }
    return nullptr;
}

/**
 * Check if any threads are working.
 * @return True if any threads are working, false otherwise.
 */
bool CYThreadPool::AreAnyThreadsWorking() const noexcept
{
    std::lock_guard<std::mutex> lock(m_objMutex);
    return std::any_of(m_lstThread.begin(), m_lstThread.end(),
        [](const auto& thread) {
            return thread->GetThreadAvail() != CYThreadStatus::STATUS_THREAD_NOT_EXECUTING;
        });
}

/**
* Terminate all working threads.
* @return True if success, false otherwise.
* todo: should not terminate a thread if it is within a synchronization.
*/
void CYThreadPool::TerminateAllWorkingThreads() noexcept
{
    std::lock_guard<std::mutex> lock(m_objMutex);
    m_objTPProps.SetTaskPoolLock(true);

    for (auto& thread : m_lstThread)
    {
        if (thread->GetThreadAvail() != CYThreadStatus::STATUS_THREAD_NOT_EXECUTING)
        {
            thread->TerminateThread();
        }
    }
}

/**
 * Suspend all working threads.
 * @return True if success, false otherwise.
 */
void CYThreadPool::SuspendAllWorkingThreads() noexcept
{
    std::lock_guard<std::mutex> lock(m_objMutex);
    m_objTPProps.SetTaskPoolLock(true);

    for (auto& thread : m_lstThread)
    {
        if (thread->GetThreadAvail() != CYThreadStatus::STATUS_THREAD_NOT_EXECUTING)
        {
            thread->SuspendThread();
        }
    }
}

/**
 * Get nAvailable thread count.
 * @return nAvailable thread count.
 */
int CYThreadPool::GetThreadAvailableCount() const noexcept
{
    std::lock_guard<std::mutex> lock(m_objMutex);
    return std::count_if(m_lstThread.begin(), m_lstThread.end(),
        [](const auto& thread) {
            auto status = thread->GetThreadAvail();
            return status == CYThreadStatus::STATUS_THREAD_NOT_EXECUTING ||
                status == CYThreadStatus::STATUS_THREAD_PURGING;
        });
}

/**
 * Get max thread count.
 * @return Max thread count.
 */
int CYThreadPool::GetMaxThreadCount() const noexcept
{
    return m_objTPProps.GetMaxThreadCount();
}

/**
 * Promote cleanup threads to nAvailable.
 * @note This function will promote cleanup threads to nAvailable.
 * @note This function will be called by CYThread periodically.
 */
void CYThreadPool::PromoteCleanupToAvailability() noexcept
{
    for (auto& thread : m_lstThread)
    {
        if (thread->GetThreadAvail() == CYThreadStatus::STATUS_THREAD_PURGING)
        {
            thread->SetThreadAvail(CYThreadStatus::STATUS_THREAD_NOT_EXECUTING);
        }
    }
}

/**
 * Get count of threads with specific status.
 * @param eThreadType Thread type.
 * @return Count of threads with specific status.
 */
int CYThreadPool::GetSpecificThreadStatusCount(CYThreadStatus eThreadType) const noexcept
{
    std::lock_guard<std::mutex> lock(m_objMutex);
    return std::count_if(m_lstThread.begin(), m_lstThread.end(),
        [eThreadType](const auto& thread) {
            return thread->GetThreadAvail() == eThreadType;
        });
}

/**
 * Check if pool is empty.
 * @return True if pool is empty, false otherwise.
 */
bool CYThreadPool::IsPoolEmpty() const noexcept
{
    std::lock_guard<std::mutex> lock(m_objMutex);
    return m_lstTask.empty() && m_lstTTask.empty() && m_lstTTaskMiss.empty();
}

/**
 * Pause all working threads.
 * @return True if success, false otherwise.
 * @note SuspendAllWorkingThreads() ϳ ÷׸ CYThreadStatus::STATUS_THREAD_PAUSING.
 */
void CYThreadPool::PauseAllWorkingThreads() noexcept
{
    std::lock_guard<std::mutex> lock(m_objMutex);
    for (auto& thread : m_lstThread)
    {
        if (thread->GetThreadAvail() != CYThreadStatus::STATUS_THREAD_NOT_EXECUTING)
        {
            thread->SuspendThread();
        }
    }
}

/**
 * Pause specific working thread.
 * @param pInvokingObject Object invoking the task.
 */
void CYThreadPool::PauseWorkingThread(ICYIThreadableObject* pInvokingObject) noexcept
{
    if (!pInvokingObject) return;

    std::lock_guard<std::mutex> lock(m_objMutex);
    for (auto& thread : m_lstThread)
    {
        if (thread->GetThreadObject() == pInvokingObject)
        {
            thread->SuspendThread();
            break;
        }
    }
}

/**
 * Resume all working threads.
 * @return True if success, false otherwise.
 * @note This function will resume all threads, including paused threads.
 */
void CYThreadPool::ResumeAllWorkingThreads() noexcept
{
    std::lock_guard<std::mutex> lock(m_objMutex);
    for (auto& thread : m_lstThread)
    {
        if (thread->GetThreadAvail() != CYThreadStatus::STATUS_THREAD_NOT_EXECUTING)
        {
            thread->ResumeThread();
        }
    }
}

/**
 * Resume specific working thread.
 * @param pInvokingObject Object invoking the task.
 * @note This function will resume only the specified thread, not all paused threads..
 */
void CYThreadPool::ResumeWorkingThread(ICYIThreadableObject* pInvokingObject) noexcept
{
    if (!pInvokingObject) return;

    std::lock_guard<std::mutex> lock(m_objMutex);
    for (auto& thread : m_lstThread)
    {
        if (thread->GetThreadObject() == pInvokingObject)
        {
            thread->ResumeThread();
            break;
        }
    }
}

/**
 * Get status of specific working thread.
 * @param pInvokingObject Object invoking the task.
 * @return Status of specific working thread.
 */
CYThreadStatus CYThreadPool::GetWorkingThreadStatus(ICYIThreadableObject* pInvokingObject) const noexcept
{
    if (!pInvokingObject) return CYThreadStatus::STATUS_THREAD_NONE;

    std::lock_guard<std::mutex> lock(m_objMutex);
    for (const auto& thread : m_lstThread)
    {
        if (thread->GetThreadObject() == pInvokingObject)
        {
            return thread->GetThreadAvail();
        }
    }

    return CYThreadStatus::STATUS_THREAD_NONE;
}

/**
 * Terminate specific working thread.
 * @param pInvokingObject Object invoking the task.
 * @note This function will terminate only the specified thread, not all working threads.
 */
void CYThreadPool::TerminateWorkingThread(ICYIThreadableObject* pInvokingObject) noexcept
{
    if (!pInvokingObject) return;

    std::lock_guard<std::mutex> lock(m_objMutex);
    for (auto& thread : m_lstThread)
    {
        if (thread->GetThreadObject() == pInvokingObject)
        {
            thread->TerminateThread();
            break;
        }
    }
}

/**
 * Wait for thread completion.
 * @param pInvokingObject Object invoking the task.
 * @param timeout Timeout in milliseconds.
 * @return True if success, false otherwise.
 */
uint32_t CYThreadPool::WaitForSingleObject(ICYIThreadableObject* pInvokingObject, uint32_t timeout) const noexcept
{
    if (!pInvokingObject) return 0;

    std::unique_lock<std::mutex> lock(m_objMutex);
    auto start = std::chrono::steady_clock::now();

    while (true)
    {
        bool found = false;
        for (const auto& thread : m_lstThread)
        {
            if (thread->GetThreadObject() == pInvokingObject)
            {
                found = true;
                if (thread->GetThreadAvail() == CYThreadStatus::STATUS_THREAD_NOT_EXECUTING)
                {
                    return 0; // Thread completed
                }
                break;
            }
        }

        if (!found) return 0; // Thread not found

#undef max
        if (std::chrono::milliseconds(timeout) != std::chrono::milliseconds::max())
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
            if (elapsed >= std::chrono::milliseconds(timeout))
            {
                return 1; // Timeout
            }
        }

        m_objCondVar.wait_for(lock, std::chrono::milliseconds(100));
    }
}

/**
 * Start the task distribution thread.
 * @note This thread will periodically call CYThreadFoundation::Distribute().
 */
void CYThreadPool::StartDistributionThread() noexcept
{
    if (!m_bDistributionRunning.load())
    {
        m_bDistributionRunning.store(true);
        m_ptrDistributionThread = std::make_unique<std::thread>([&]() {
            DistributionThreadFunction();
        });
    }
}

/**
 * Stop the task distribution thread.
 */
void CYThreadPool::StopDistributionThread() noexcept
{
    if (m_bDistributionRunning.load())
    {
        m_bDistributionRunning.store(false);
        if (m_ptrDistributionThread && m_ptrDistributionThread->joinable())
        {
            m_ptrDistributionThread->join();
        }
        m_ptrDistributionThread.reset();
    }
}

/**
 * Distribution thread function.
 * @note This function runs in a separate thread and periodically processes tasks.
 */
void CYThreadPool::DistributionThreadFunction() noexcept
{
    while (m_bDistributionRunning.load() && !m_bShutdown.load())
    {
        // Process pending tasks
        processObjectTaskList();
        
        // Sleep for a short time to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

CYTHRAD_NAMESPACE_END