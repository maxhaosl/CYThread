#include "CYThreadPCH.hpp"
#include "CYThreadPoolProperties.hpp"

CYTHRAD_NAMESPACE_BEGIN

CYThreadPoolProperties::CYThreadPoolProperties()
    : m_nMaxThreads(10)
    , m_nMaxTasks(25)
{
}

/**
 * Function member to set the maximum allowable threads
 * @param nMaxThreads: maximum number of threads that the pool can allocate
 * @return void
 */
void CYThreadPoolProperties::SetMaxThreads(int nMaxThreads)
{
    m_nMaxThreads = nMaxThreads;
}

/**
 * Function member to set the maximum allowable number of tasks
 * @param nMaxTasks: maximum number of tasks that the pool can allocate
 * @return void
 */
void CYThreadPoolProperties::SetMaxTasks(int nMaxTasks)
{
    m_nMaxTasks = nMaxTasks;
}

/**
 * Function member to set the objTask pool lock
 * @param nStatus: the lock status, 0 means unlock, 1 means lock
 * @return void
 */
void CYThreadPoolProperties::SetTaskPoolLock(int32_t status)
{
    m_nBlockTask = status;
}

/**
 * Get the objTask pool lock
 * @return the lock status, 0 means unlock, 1 means lock
 * @note This function is not thread-safe, please use it with caution.
 */
int32_t CYThreadPoolProperties::GetTaskPoolLock(void)
{
    return m_nBlockTask;
}

CYTHRAD_NAMESPACE_END