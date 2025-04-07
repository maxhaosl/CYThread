#include "CYThreadPCH.hpp"
#include "CYThreadPool.hpp"
#include "CYThread.hpp"
#include "CYThreadProperties.hpp"
#include <thread>
#include <chrono>
#include <stdexcept>

CYTHRAD_NAMESPACE_BEGIN

CYThreadPool::CYThreadPool()
{
}

CYThreadPool::~CYThreadPool()
{
    Shutdown();
}

bool CYThreadPool::Shutdown() noexcept
{
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

        return !m_lstThread.empty();
    }
    catch (const std::exception&)
    {
        return false;
    }
}

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

void CYThreadPool::processObjectTaskList() noexcept
{
    std::lock_guard<std::mutex> lock(m_objMutex);

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

    PromoteCleanupToAvailability();
}

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

bool CYThreadPool::AreAnyThreadsWorking() const noexcept
{
    std::lock_guard<std::mutex> lock(m_objMutex);
    return std::any_of(m_lstThread.begin(), m_lstThread.end(),
        [](const auto& thread) {
            return thread->GetThreadAvail() != CYThreadStatus::STATUS_THREAD_NOT_EXECUTING;
        });
}

//todo: should not terminate a thread if it is within a synchronization
// mechanism (CYSync)
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

int CYThreadPool::GetMaxThreadCount() const noexcept
{
    return m_objTPProps.GetMaxThreadCount();
}

void CYThreadPool::PromoteCleanupToAvailability() noexcept
{
    std::lock_guard<std::mutex> lock(m_objMutex);
    for (auto& thread : m_lstThread)
    {
        if (thread->GetThreadAvail() == CYThreadStatus::STATUS_THREAD_PURGING)
        {
            thread->SetThreadAvail(CYThreadStatus::STATUS_THREAD_NOT_EXECUTING);
        }
    }
}

int CYThreadPool::GetSpecificThreadStatusCount(CYThreadStatus eThreadType) const noexcept
{
    std::lock_guard<std::mutex> lock(m_objMutex);
    return std::count_if(m_lstThread.begin(), m_lstThread.end(),
        [eThreadType](const auto& thread) {
            return thread->GetThreadAvail() == eThreadType;
        });
}

bool CYThreadPool::IsPoolEmpty() const noexcept
{
    std::lock_guard<std::mutex> lock(m_objMutex);
    return m_lstTask.empty() && m_lstTTask.empty() && m_lstTTaskMiss.empty();
}

// SuspendAllWorkingThreads() ϳ ÷׸ CYThreadStatus::STATUS_THREAD_PAUSING
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

//
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

CYTHRAD_NAMESPACE_END