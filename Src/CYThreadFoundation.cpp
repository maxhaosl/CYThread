#include "CYThreadPCH.hpp"
#include "CYThreadFoundation.hpp"
#include <thread>
#include <chrono>
#include <stdexcept>

CYTHRAD_NAMESPACE_BEGIN

// Initialize static member
std::unique_ptr<CYThreadPool> CYThreadFoundation::m_ptrThreadPool = nullptr;

CYThreadFoundation::CYThreadFoundation()
{
    //Set the platform
    CYPlatformId tempId;
    //Determine and store the platform specifier for later use.
    tempId = CY_PLATFORM_WINDOWS;
    m_objPlatformSpecifier.SetPlatformId(tempId);
}

CYThreadFoundation::~CYThreadFoundation()
{
    Shutdown();
}

/**
 * Clean up all allocations.
 */
void CYThreadFoundation::Shutdown() noexcept
{
    if (m_ptrThreadPool)
    {
        m_ptrThreadPool->TerminateAllWorkingThreads();
        m_ptrThreadPool.reset();
    }
}

#ifdef _DEBUG
/**
 * Get thread pool instance (debug only).
 * @return The thread pool instance.
 */
CYThreadPool& CYThreadFoundation::GetTPInstance() noexcept
{
    if (!m_ptrThreadPool)
    {
        CreateThreadPool();
    }
    return *m_ptrThreadPool;
}
#endif

/**
 * Check if thread pool is empty.
 * @return true if thread pool is empty, otherwise false.
 */
bool CYThreadFoundation::IsEmpty() const noexcept
{
    return(m_ptrThreadPool ? m_ptrThreadPool->IsPoolEmpty() : true);
}

/**
 * Check if any threads are executing tasks.
 * @return true if any threads are executing tasks, otherwise false.
 */
bool CYThreadFoundation::AreAnyThreadsWorking() const noexcept
{
    if (!m_ptrThreadPool) return false;

    const int nMaxThreadCount = m_ptrThreadPool->GetMaxThreadCount();
    const int nAvailable = m_ptrThreadPool->GetThreadAvailableCount();
    const int nPauseThreadCount = m_ptrThreadPool->GetSpecificThreadStatusCount(CYThreadStatus::STATUS_THREAD_PAUSING);

    return (nAvailable + nPauseThreadCount) != nMaxThreadCount;
}

/**
 * Terminate all working threads.
 * @note This function will block until all threads are terminated.
 */
void CYThreadFoundation::TerminateAllWorkingThreads() noexcept
{
    if (m_ptrThreadPool)
    {
        m_ptrThreadPool->TerminateAllWorkingThreads();
    }
}

/**
 * Suspend all executing threads.
 * @note This function will block until all threads are suspended.
 */
void CYThreadFoundation::SuspendAllWorkingThreads() noexcept
{
    if (m_ptrThreadPool)
    {
        m_ptrThreadPool->SuspendAllWorkingThreads();
    }
}

/**
 * Process submitted tasks.
 * @note This function will be called by CYThread periodically.
 */
void CYThreadFoundation::Distribute() noexcept
{
    if (m_ptrThreadPool)
    {
        m_ptrThreadPool->processObjectTaskList();
    }
}

/**
 * Submit a objTask to the pool.
 * @param pInvokingObject The task to be submitted.
 * @return true if the task is submitted successfully, otherwise false.
 */
bool CYThreadFoundation::SubmitTask(ICYIThreadableObject* pInvokingObject) noexcept
{
    if (!m_ptrThreadPool || !pInvokingObject) return false;
    return(m_ptrThreadPool->SubmitTask(pInvokingObject));
}

/**
 * Populate default execution properties that the caller can tweak before submitting
 * a task to the pool. The goal is to give each task a balanced preferred core while
 * keeping the defaults cross-platform safe.
 */
void CYThreadFoundation::GetTaskExecutionProps(CYThreadExecutionProps& objTep) const noexcept
{
    objTep.SetTasksProcessAffinity(CYProcessorAffinity::AFFINITY_PROCESSOR_SOFT);
    objTep.SetTasksPriority(CYThreadPriority::PRIORITY_THREAD_NORMAL);
#undef max
    const unsigned int hwConcurrency = std::max(1u, std::thread::hardware_concurrency());

    int preferredCore = 0;
    if (m_ptrThreadPool && hwConcurrency > 0)
    {
        const int activeThreads = m_ptrThreadPool->GetMaxThreadCount() - m_ptrThreadPool->GetThreadAvailableCount();
        preferredCore = activeThreads % static_cast<int>(hwConcurrency);
    }

    objTep.SetTasksCore(preferredCore);
    objTep.CreateTasksProcessorAffinity();
}

/**
 * Create thread pool.
 * @param iMaxThread The maximum number of threads in the pool.
 */
void CYThreadFoundation::CreateThreadPool(int iMaxThread) noexcept
{
    if (!m_ptrThreadPool)
    {
        m_ptrThreadPool = std::make_unique<CYThreadPool>();
        //Create the platform specific thread pool, using the platform specifier.
        m_ptrThreadPool->CreateThreadPool(m_objPlatformSpecifier.GetPlatformId(), iMaxThread);
    }
}

/**
 * Get nAvailable thread.
 * @param bRemove Remove thread from list if true.
 */
CYThread* CYThreadFoundation::GetAvailThread(bool bRemove) const noexcept
{
    return m_ptrThreadPool ? m_ptrThreadPool->GetAvailThread(bRemove) : nullptr;
}

/**
* Pause all working threads.
* @note This function will block until all threads are paused.
* @note SuspendAllWorkingThreads() ϳ ÷׸ CYThreadStatus::STATUS_THREAD_PAUSING.
*/
void CYThreadFoundation::PauseAllWorkingThreads() noexcept
{
    if (m_ptrThreadPool)
    {
        m_ptrThreadPool->PauseAllWorkingThreads();
    }
}

/**
 * Pause specific working thread.
 * @param pInvokingObject The thread to be paused.
 * @note This function will block until the specified thread is paused.
 */
void CYThreadFoundation::PauseWorkingThread(ICYIThreadableObject* pInvokingObject) noexcept
{
    if (m_ptrThreadPool && pInvokingObject)
    {
        m_ptrThreadPool->PauseWorkingThread(pInvokingObject);
    }
}

/**
 * Resume all working threads.
 * @note This function will block until all threads are resumed.
 */
void CYThreadFoundation::ResumeAllWorkingThreads() noexcept
{
    if (m_ptrThreadPool)
    {
        m_ptrThreadPool->ResumeAllWorkingThreads();
    }
}

/**
 * Resume specific working thread.
 * @param pInvokingObject The thread to be resumed.
 * @note This function will block until the specified thread is resumed.
 */
void CYThreadFoundation::ResumeWorkingThread(ICYIThreadableObject* pInvokingObject) noexcept
{
    if (m_ptrThreadPool && pInvokingObject)
    {
        m_ptrThreadPool->ResumeWorkingThread(pInvokingObject);
    }
}

/**
 * Get status of specific working thread.
 * @param pInvokingObject The thread to be queried.
 * @return The status of the specified thread.
 */
CYThreadStatus CYThreadFoundation::GetWorkingThreadStatus(ICYIThreadableObject* pInvokingObject) const noexcept
{
    return m_ptrThreadPool && pInvokingObject ? m_ptrThreadPool->GetWorkingThreadStatus(pInvokingObject) : CYThreadStatus::STATUS_THREAD_NONE;
}

/**
 * Terminate specific working thread.
 * @param pInvokingObject The thread to be terminated.
 * @note This function will block until the specified thread is terminated.
 */
void CYThreadFoundation::TerminateWorkingThread(ICYIThreadableObject* pInvokingObject) noexcept
{
    if (m_ptrThreadPool && pInvokingObject)
    {
        m_ptrThreadPool->TerminateWorkingThread(pInvokingObject);
    }
}

/**
 * Wait for thread completion.
 * @param pInvokingObject The thread to be waited.
 */
uint32_t CYThreadFoundation::WaitForSingleObject(ICYIThreadableObject* pInvokingObject, uint32_t dwMiliseconds) const noexcept
{
    return m_ptrThreadPool ? m_ptrThreadPool->WaitForSingleObject(pInvokingObject, dwMiliseconds) : 0;
}

CYTHRAD_NAMESPACE_END