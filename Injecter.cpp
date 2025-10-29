// ConsoleApplication2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "MemStuff.h"
#include "psapi.h";
#include <thread>

// seems like the page's size isn't stable, so instead of fucking around with ram we'll set bounds
// TODO: REDO this because RAM region sizes are extremely unstable and the 16MB page breaks into random pieces however it wants it
const int minRegionSize = 4096*1024; // minimal region size
const int minPageSize = 14*1024*1024; // minimal page size
const int maxRegionSize = 17000 * 1024; // max region size
std::map<LPVOID, int> regionSize{}; // ACTUAL page size
std::vector<LPVOID> knownAddresses{}; // cache
const int strLen = 128; // string length
const int minPageOffset = 0x14a30000; // Set to an actual value for faster processing. For DELTARUNE v17 on Windows on my machine, it's 0x14a30000
const int delay = 10; // Delay between memory writes

struct MemRegion {
    LPVOID addr;
    byte mem[maxRegionSize];
};

struct MemString {
    std::string s;
    int offset;
};

std::vector<MemRegion*> mrs;
int lastUpdateRs = 0;
HANDLE process;

bool IsValidRegion(byte* byteArr) {
    return (
        byteArr[0] + byteArr[1] + byteArr[2] + byteArr[3] + byteArr[4] + byteArr[5] + byteArr[6] + byteArr[7] == 0x00 &&
        (
            (byteArr[14] == 0x01 && byteArr[15] == 0x01 && byteArr[16] == 0xEE && byteArr[17] == 0xFF && byteArr[18] == 0xEE && byteArr[19] == 0xFF)
            ||
            (byteArr[0x1C] == 0xDE && byteArr[0x1D] == 0xC0 && byteArr[0x1E] == 0xAD && byteArr[0x1F] == 0xDE)
        )
    );
}

std::vector<MemRegion*> FindMemRegions() {
    unsigned char* p = NULL;
    MEMORY_BASIC_INFORMATION info;
    std::vector<MemRegion*> reg;
    bool inPage = false;
    int totalSize = 0;
    bool cacheExists = knownAddresses.size() != 0;
    p += minPageOffset-1;

    for (p = NULL;
        VirtualQueryEx(process, p, &info, sizeof(info)) == sizeof(info);
        p += info.RegionSize)
    {
        if (cacheExists && std::count(knownAddresses.begin(), knownAddresses.end(), info.BaseAddress) == 0) continue;
        /* basically the page structure is something like :
        * [Page of 16MB]
        * - 8100KB
        * - 4KB
        * - 8120KB
        * - 4KB
        * 
        * But sometimes it's
        * 
        * - 1234 KB
        * - 4KB
        * - 4321 KB
        * - 4KB
        * - 8912KB
        * - 4KB
        * 
        * Basically, either reasanoble, or random. I don't know what to do with it, so below is my idea on how to detect the full page regions from it.
        * Just like that.
        */
       
        
        if (info.Type == MEM_PRIVATE && info.Protect == PAGE_READWRITE && info.RegionSize < maxRegionSize && info.RegionSize > minRegionSize)
        {
            bool iVR = false;
            if (!inPage) {
                byte tempArr[32];
                ReadProcessMemory(process, info.BaseAddress, &tempArr, sizeof(tempArr), NULL);
                iVR = IsValidRegion(tempArr);
            }
            if (inPage || iVR) {
                if (iVR) {
                    inPage = true;
                }
                MemRegion* mr = new MemRegion;
                mr->addr = info.BaseAddress;
                ReadProcessMemory(process, info.BaseAddress, &mr->mem, sizeof(mr->mem), NULL);
                reg.push_back(mr);
                totalSize += info.RegionSize;
                regionSize[info.BaseAddress] = (int)info.RegionSize;
            }
        }
        if (inPage && info.RegionSize == 4096 && info.State == MEM_RESERVE) { // end of page
            if (totalSize < minPageSize) {
                //std::cout << "Bad page - " << totalSize << "bytes\n";
                inPage = false;
                totalSize = 0; // bad page
            }
            else {
                return reg;
            }
        }
    }
    return reg;
}

std::vector<MemString> FindStrings(byte* bytes, LPVOID pageAddr) {
    std::vector<MemString> strings;
    const int ix = regionSize[pageAddr];
    for (int i = 0; i + 9 < ix; i++) {
        if (
            (
                bytes[i] + bytes[i + 1] + bytes[i + 2]) == 0x00 &&
                bytes[i + 3] == 0x5c &&
                bytes[i + 4] == 0x45
            ) {
            static char c[strLen];
            std::copy(&bytes[i + 3], &bytes[i + 3 + strLen], c);
            MemString ms;
            ms.offset = i+3;
            ms.s = std::string(c);
            strings.push_back(ms);
            i += 5 + std::strlen(c);
        }
    }
    return strings;
}

void ReplaceMemString(LPVOID pageAddr, MemString* ms, const char toReplace[]) {
    LPVOID addr = (LPVOID)((int)pageAddr + ms->offset);
    std::cout << "Writing " << toReplace << " to " << addr << " | " << strlen(toReplace) << " bytes \n";
    WriteProcessMemory(process, addr, toReplace, strlen(toReplace), NULL);
}

bool ReplaceMemStringFind(LPVOID pageAddr, MemString* ms, const char toFind[], const char toReplace[]) { // True, if we found a string
    if (ms->s.find(toFind) != std::string::npos) {
        ReplaceMemString(pageAddr, ms, toReplace);
        return true;
    }
    return false;
}

void updateStuff() {
    if (lastUpdateRs < 0) {
        knownAddresses.clear();
        mrs = FindMemRegions();
        for (int i = 0; i < mrs.size(); i++) {
            knownAddresses.push_back(mrs.at(i)->addr);
        }
        lastUpdateRs = delay * 1000;
    }
    else {
        lastUpdateRs--;
    }
}

void updateStuffLoop() {
    while (true) {
        updateStuff(); 
        Sleep(10);
    }
}

int main()
{
    int PID = findMyProc("DELTARUNE.exe");
    process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);

    updateStuff();
    std::thread t(updateStuffLoop);
    t.detach();
    while (true) {
        mrs = FindMemRegions();
        for (int ii = 0; ii < mrs.size(); ii++) {
            MemRegion* mr = mrs.at(ii);
            LPVOID addr = mr->addr;
            //std::cout << "Found the region at " << addr << " !\n";
            std::vector<MemString> strings = FindStrings(mr->mem, mr->addr);
            for (int i = 0; i < strings.size(); i++) {
                MemString s = strings.at(i);
                if (s.s.length() < 5) continue;
                // strings should be shorter, or equal to the original length!
                ReplaceMemStringFind(addr, &s, "I think I'm parked", "\\EK* I think we've injected.    /"); // Note that when we find, we only take a part that's going into std::find, but on the replacement - we use the full string.
                ReplaceMemStringFind(addr, &s, "Parallel", "\\E2* We've injected ? /"); // Entities have different keys. 
                ReplaceMemStringFind(addr, &s, "within the lines", "\\EX* As far as I know^1, we indeed have injected.   /%"); // "%" ends the dialogue
            }
            //std::cout << "End at " << addr << " !\n";
        }
        Sleep(delay);
    }
}