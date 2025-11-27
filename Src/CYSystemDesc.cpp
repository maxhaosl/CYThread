#include "CYThreadPCH.hpp"
#include "CYSystemDesc.hpp"
#include <thread>
#include <map>

CYTHRAD_NAMESPACE_BEGIN

CYSystemDescription::CYSystemDescription()
{
    Startup();
}

void CYSystemDescription::Startup()
{
#if defined(_WIN32)
    m_objMemInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&m_objMemInfo);
    GetSystemInfo(&m_objSystemInfo);
    m_nBytesPhysicalRam = static_cast<uint32_t>(m_objMemInfo.ullTotalPhys);
    m_nMemoryLoad = static_cast<uint32_t>(m_objMemInfo.dwMemoryLoad);
#elif defined(__linux__)
    if (sysinfo(&m_objSysInfo) == 0)
    {
        m_nBytesPhysicalRam = static_cast<uint32_t>(m_objSysInfo.totalram * m_objSysInfo.mem_unit);
        m_nMemoryLoad = static_cast<uint32_t>((m_objSysInfo.totalram - m_objSysInfo.freeram) * 100 / m_objSysInfo.totalram);
    }
#endif
}

int CYSystemDescription::GetNumberProcessors() const noexcept
{
#if defined(_WIN32)
    return static_cast<int>(m_objSystemInfo.dwNumberOfProcessors);
#elif defined(__linux__)
    return static_cast<int>(std::thread::hardware_concurrency());
#endif
}

int CYSystemDescription::GetHyperThreadAvailability() const noexcept
{
#if defined(_WIN32)
    // Use GetLogicalProcessorInformation to detect HT
    DWORD returnLength = 0;
    GetLogicalProcessorInformation(nullptr, &returnLength);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        return 0;
    }

    std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(returnLength / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
    if (!GetLogicalProcessorInformation(buffer.data(), &returnLength))
    {
        return 0;
    }

    // Count the number of logical processors per physical core
    std::map<DWORD_PTR, int> logicalProcessorsPerCore;
    for (const auto& info : buffer)
    {
        if (info.Relationship == RelationProcessorCore)
        {
            logicalProcessorsPerCore[info.ProcessorMask]++;
        }
    }

    // If any core has more than one logical processor, HT is supported
    for (const auto& [mask, count] : logicalProcessorsPerCore)
    {
        if (count > 1)
        {
            return 1;
        }
    }
    return 0;
#elif defined(__linux__) && CYTHREAD_HAS_LINUX_CPUID
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
    __get_cpuid(1, &eax, &ebx, &ecx, &edx);
    return (edx & (1 << 28)) ? 1 : 0;
#else
    return 0;
#endif
}

uint32_t CYSystemDescription::GetMemoryLoad() const noexcept
{
    return m_nMemoryLoad;
}

uint32_t CYSystemDescription::GetBytesPhysicalMemory() const noexcept
{
    return m_nBytesPhysicalRam;
}

int32_t CYSystemDescription::DoMemoryExced(uint32_t MemValue) const noexcept
{
    return (m_nBytesPhysicalRam / m_nCYSystemMB > MemValue) ? 1 : 0;
}

CYTHRAD_NAMESPACE_END