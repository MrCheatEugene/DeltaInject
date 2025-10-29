#include "MemStuff.h"
#include <vector>

/*
Yeah, I've pasted some stuff and made it to work here.
What're you gonna do?
*/

using namespace std;

const wchar_t* GetWC(const char* c) // https://stackoverflow.com/questions/8032080/how-to-convert-char-to-wchar-t
{
    const size_t cSize = strlen(c) + 1;
    wchar_t* wc = new wchar_t[cSize];
    mbstowcs(wc, c, cSize);

    return wc;
}

int findMyProc(const char* procname) { // https://cocomelonc.github.io/pentest/2021/09/29/findmyprocess.html

    HANDLE hSnapshot;
    PROCESSENTRY32 pe;
    int pid = 0;
    BOOL hResult;

    // snapshot of all processes in the system
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hSnapshot) return 0;

    // initializing size: needed for using Process32First
    pe.dwSize = sizeof(PROCESSENTRY32);

    // info about first process encountered in a system snapshot
    hResult = Process32First(hSnapshot, &pe);

    // retrieve information about the processes
    // and exit if unsuccessful
    while (hResult) {
        // if we find the process: return process ID
        if (wcscmp(GetWC(procname), pe.szExeFile) == 0) {
            pid = pe.th32ProcessID;
            break;
        }
        hResult = Process32Next(hSnapshot, &pe);
    }

    // closes an open handle (CreateToolhelp32Snapshot)
    CloseHandle(hSnapshot);
    return pid;
}

uintptr_t GetModuleBaseAddress(DWORD dwProcID, const char* szModuleName) // https://stackoverflow.com/questions/48208782/read-memory-of-process-c
{
    uintptr_t ModuleBaseAddress = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, dwProcID);
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 ModuleEntry32;
        ModuleEntry32.dwSize = sizeof(MODULEENTRY32);
        if (Module32First(hSnapshot, &ModuleEntry32))
        {
            do
            {

                if (wcscmp(ModuleEntry32.szModule, GetWC(szModuleName)) == 0)
                {
                    ModuleBaseAddress = (uintptr_t)ModuleEntry32.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnapshot, &ModuleEntry32));
        }
        CloseHandle(hSnapshot);
    }
    return ModuleBaseAddress;
}

template <typename T>
void ReadMem(HANDLE process, uintptr_t ADDRESS, T& out, int length = 0) {
    ReadProcessMemory(process, (LPVOID)ADDRESS, &out, length == 0 ? sizeof(out) : (size_t)length, NULL);
}

template <typename T>
void WriteMem(HANDLE process, uintptr_t ADDRESS, T out) {
    WriteProcessMemory(process, (LPVOID)ADDRESS, &out, sizeof(out), NULL);
}

// Helper functions

uintptr_t ReadPtr(HANDLE process, uintptr_t ADDRESS) {
    uintptr_t r;
    ReadMem(process, ADDRESS, r);
    return r;
}

unsigned int ReadUInt(HANDLE process, uintptr_t ADDRESS) {
    unsigned int r;
    ReadMem(process, ADDRESS, r);
    return r;
}

unsigned long ReadUInt64(HANDLE process, uintptr_t ADDRESS) {
    unsigned int r;
    ReadMem(process, ADDRESS, r);
    return r;
}

int ReadInt(HANDLE process, uintptr_t ADDRESS) {
    int r;
    ReadMem(process, ADDRESS, r);
    return r;
}

short ReadShort(HANDLE process, uintptr_t ADDRESS) {
    short r;
    ReadMem(process, ADDRESS, r);
    return r;
}


void ReadCString(HANDLE process, uintptr_t ADDRESS, char* buffer, size_t maxLen = 128) {
    ReadProcessMemory(process, (LPCVOID)ADDRESS, buffer, maxLen, NULL);
}

float* ReadMatrix(HANDLE process, uintptr_t ADDRESS) {
    static float buffer[16];
    ReadProcessMemory(process, (LPCVOID)ADDRESS, buffer, sizeof(buffer), NULL);
    return buffer;
}
void WriteInt(HANDLE process, uintptr_t ADDRESS, int i) {
    WriteMem(process, ADDRESS, i);
}

bool ReadBool(HANDLE process, uintptr_t ADDRESS) {
    bool r;
    ReadMem(process, ADDRESS, r);
    return r;
}
