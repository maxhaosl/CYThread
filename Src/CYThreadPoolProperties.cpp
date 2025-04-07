#include "CYThreadPCH.hpp"
#include "CYThreadPoolProperties.hpp"

CYTHRAD_NAMESPACE_BEGIN

CYThreadPoolProperties::CYThreadPoolProperties()
    : m_nMaxThreads(10)
    , m_nMaxTasks(25)
{
}

void CYThreadPoolProperties::SetMaxThreads(int nMaxThreads)
{
    m_nMaxThreads = nMaxThreads;
}

void CYThreadPoolProperties::SetMaxTasks(int nMaxTasks)
{
    m_nMaxTasks = nMaxTasks;
}

void CYThreadPoolProperties::SetTaskPoolLock(int32_t status)
{
    m_nBlockTask = status;
}

int32_t CYThreadPoolProperties::GetTaskPoolLock(void)
{
    return m_nBlockTask;
}

CYTHRAD_NAMESPACE_END