#include "CYThreadPCH.hpp"
#include "CYThreadProperties.hpp"

CYTHRAD_NAMESPACE_BEGIN

CYThreadProperties::CYThreadProperties()
    : m_pThreadHandle(0)
    , m_nThreadId(0)
    , m_nStackSize(0)
{
}

/**
 * Function member to populate the threads properties.
 * @param platformId: the platform id of the current system.
 * @return void
 */
void CYThreadProperties::CreateProperties(const CYPlatformId& ePlatformId)
{
    //Generate the threads properties
    switch (ePlatformId)
    {
    case CY_PLATFORM_WINDOWS:
        SetStackSize(65536);
        break;
    default:
        SetStackSize(65536);
        break;
    }
}

/**
 * Function member to set the stack size of a particular thread.
 * @param stackSize: the stack size of a particular thread.
 * @return void
 */
void CYThreadProperties::SetStackSize(int nStackSize)
{
    m_nStackSize = nStackSize;
}

CYTHRAD_NAMESPACE_END