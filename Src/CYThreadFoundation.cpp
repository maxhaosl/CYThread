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

void CYThreadFoundation::Shutdown() noexcept
{
    if (m_ptrThreadPool)
    {
        m_ptrThreadPool->TerminateAllWorkingThreads();
        m_ptrThreadPool.reset();
    }
}

#ifdef _DEBUG
//mpr: enabled only for debugging!
CYThreadPool& CYThreadFoundation::GetTPInstance() noexcept
{
    if (!m_ptrThreadPool)
    {
        CreateThreadPool();
    }
    return *m_ptrThreadPool;
}
#endif

bool CYThreadFoundation::IsEmpty() const noexcept
{
    return(m_ptrThreadPool ? m_ptrThreadPool->IsPoolEmpty() : true);
}

bool CYThreadFoundation::AreAnyThreadsWorking() const noexcept
{
    if (!m_ptrThreadPool) return false;

    const int nMaxThreadCount = m_ptrThreadPool->GetMaxThreadCount();
    const int nAvailable = m_ptrThreadPool->GetThreadAvailableCount();
    const int nPauseThreadCount = m_ptrThreadPool->GetSpecificThreadStatusCount(CYThreadStatus::STATUS_THREAD_PAUSING);

    return (nAvailable + nPauseThreadCount) != nMaxThreadCount;
}

void CYThreadFoundation::TerminateAllWorkingThreads() noexcept
{
    if (m_ptrThreadPool)
    {
        m_ptrThreadPool->TerminateAllWorkingThreads();
    }
}

void CYThreadFoundation::SuspendAllWorkingThreads() noexcept
{
    if (m_ptrThreadPool)
    {
        m_ptrThreadPool->SuspendAllWorkingThreads();
    }
}

void CYThreadFoundation::Distribute() noexcept
{
    if (m_ptrThreadPool)
    {
        m_ptrThreadPool->processObjectTaskList();
    }
}

bool CYThreadFoundation::SubmitTask(ICYIThreadableObject* pInvokingObject) noexcept
{
    if (!m_ptrThreadPool || !pInvokingObject) return false;
    return(m_ptrThreadPool->SubmitTask(pInvokingObject));
}

void CYThreadFoundation::CreateThreadPool(int iMaxThread) noexcept
{
    if (!m_ptrThreadPool)
    {
        m_ptrThreadPool = std::make_unique<CYThreadPool>();
        //Create the platform specific thread pool, using the platform specifier.
        m_ptrThreadPool->CreateThreadPool(m_objPlatformSpecifier.GetPlatformId(), iMaxThread);
    }
}

CYThread* CYThreadFoundation::GetAvailThread(bool bRemove) const noexcept
{
    return m_ptrThreadPool ? m_ptrThreadPool->GetAvailThread(bRemove) : nullptr;
}

// SuspendAllWorkingThreads() ϳ ÷׸ CYThreadStatus::STATUS_THREAD_PAUSING
void CYThreadFoundation::PauseAllWorkingThreads() noexcept
{
    if (m_ptrThreadPool)
    {
        m_ptrThreadPool->PauseAllWorkingThreads();
    }
}

void CYThreadFoundation::PauseWorkingThread(ICYIThreadableObject* pInvokingObject) noexcept
{
    if (m_ptrThreadPool && pInvokingObject)
    {
        m_ptrThreadPool->PauseWorkingThread(pInvokingObject);
    }
}

//
void CYThreadFoundation::ResumeAllWorkingThreads() noexcept
{
    if (m_ptrThreadPool)
    {
        m_ptrThreadPool->ResumeAllWorkingThreads();
    }
}

void CYThreadFoundation::ResumeWorkingThread(ICYIThreadableObject* pInvokingObject) noexcept
{
    if (m_ptrThreadPool && pInvokingObject)
    {
        m_ptrThreadPool->ResumeWorkingThread(pInvokingObject);
    }
}

CYThreadStatus CYThreadFoundation::GetWorkingThreadStatus(ICYIThreadableObject* pInvokingObject) const noexcept
{
    return m_ptrThreadPool && pInvokingObject ?
        m_ptrThreadPool->GetWorkingThreadStatus(pInvokingObject) :
        CYThreadStatus::STATUS_THREAD_NONE;
}

void CYThreadFoundation::TerminateWorkingThread(ICYIThreadableObject* pInvokingObject) noexcept
{
    if (m_ptrThreadPool && pInvokingObject)
    {
        m_ptrThreadPool->TerminateWorkingThread(pInvokingObject);
    }
}

uint32_t CYThreadFoundation::WaitForSingleObject(ICYIThreadableObject* pInvokingObject, uint32_t dwMiliseconds) const noexcept
{
    return m_ptrThreadPool ? m_ptrThreadPool->WaitForSingleObject(pInvokingObject, dwMiliseconds) : 0;
}

CYTHRAD_NAMESPACE_END