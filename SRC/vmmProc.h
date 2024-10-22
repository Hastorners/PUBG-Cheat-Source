#pragma once
#include <string>
#include <vector>
#include "leechcore.h"
#include "vmmdll.h"
#pragma comment(lib, "leechcore.lib")
#pragma comment(lib, "vmm.lib")  
using namespace std;



BOOL VMMDLL_Initialize();
void SetProcessPid(DWORD dwPid);
DWORD GetProcessPid(std::string strPorName);
std::string GetPidName(DWORD dwPid);
std::string GetPidName2(DWORD dwPid);
std::vector<DWORD> GetProcessPidList();
uint64_t GetModuleFromName(std::string strName);
BOOL ReadMemory(uint64_t uBaseAddr, LPVOID lpBuffer, DWORD nSize);
BOOL WriteMemory(uint64_t uBaseAddr, LPVOID lpBuffer, DWORD nSize);
BOOL MemWriteMemory(uint64_t uBaseAddr, _In_reads_(cb) PBYTE pb, _In_ DWORD cb);
LPSTR ProcessGetInformationString(DWORD dwPID);

vector<BYTE> ReadBYTE(uint64_t ptr, SIZE_T size);


template<typename T>
T Read(uint64_t ptr)
{
	T buff;
	ReadMemory(ptr, &buff, sizeof(T));
	return buff;
}

template<typename T>
BOOL Write(uint64_t ptr, T value)
{
	WriteMemory(ptr, &value, sizeof(T));
	return true;
}

bool Scatter_Read(VMMDLL_SCATTER_HANDLE HS, uint64_t addr, PVOID pBuf, DWORD size);

template<typename T>

T SRead(VMMDLL_SCATTER_HANDLE HS, uint64_t ptr)
{
	T buff;
	Scatter_Read(HS, ptr, &buff, sizeof(T));
	return buff;
}

bool SPrepare(VMMDLL_SCATTER_HANDLE HS, uint64_t va, DWORD cb);
bool SClear(VMMDLL_SCATTER_HANDLE HS, DWORD flags);
VMMDLL_SCATTER_HANDLE Scatter_Initialize(DWORD flags);
bool ExecuteRead(VMMDLL_SCATTER_HANDLE HS);



