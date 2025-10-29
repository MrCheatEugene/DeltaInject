#pragma once
#include <cstdint>
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <map>
#include <string>

const wchar_t* GetWC(const char* c);
int findMyProc(const char* procname);
uintptr_t GetModuleBaseAddress(DWORD dwProcID, const char* szModuleName);

template <typename T>
void ReadMem(HANDLE process, uintptr_t ADDRESS, T& out, int length);

template <typename T>
void WriteMem(HANDLE process, uintptr_t ADDRESS, T out);

unsigned int ReadUInt(HANDLE process, uintptr_t ADDRESS);
unsigned long ReadUInt64(HANDLE process, uintptr_t ADDRESS);
int ReadInt(HANDLE process, uintptr_t ADDRESS);
uintptr_t ReadPtr(HANDLE process, uintptr_t ADDRESS);
void ReadCString(HANDLE process, uintptr_t ADDRESS, char* buffer, size_t maxLen);
bool ReadBool(HANDLE process, uintptr_t ADDRESS);
float* ReadMatrix(HANDLE process, uintptr_t ADDRESS);
short ReadShort(HANDLE process, uintptr_t ADDRESS);

void WriteInt(HANDLE process, uintptr_t ADDRESS, int i);