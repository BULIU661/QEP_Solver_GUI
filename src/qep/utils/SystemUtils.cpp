// utils/SystemUtils.cpp
#include "SystemUtils.h"
#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <unistd.h>
#endif

namespace QEP {

long getCurrentRSS() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return pmc.WorkingSetSize / 1024;
    return -1;
#else
    pid_t pid = getpid();
    std::string path = "/proc/" + std::to_string(pid) + "/statm";
    std::ifstream file(path);
    if (!file.is_open())
        return -1;
    long size, resident, share, text, lib, data, dt;
    file >> size >> resident >> share >> text >> lib >> data >> dt;
    file.close();
    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024;
    return resident * page_size_kb;
#endif
}

} // namespace QEP
