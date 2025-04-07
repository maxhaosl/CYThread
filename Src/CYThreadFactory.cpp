#include "CYThread/CYThreadFactory.hpp"
#include "CYThreadPool.hpp"

CYTHRAD_NAMESPACE_BEGIN

CYThreadFactory::CYThreadFactory()
{
}

CYThreadFactory::~CYThreadFactory()
{
}

ICYThreadPool* CYThreadFactory::CreateThreadPool()
{
    return new CYThreadPool();
}

void CYThreadFactory::ReleaseThreadPool(ICYThreadPool* pThreadPool)
{
    delete pThreadPool;
}

CYTHRAD_NAMESPACE_END