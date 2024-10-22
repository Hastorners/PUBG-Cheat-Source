#include "vmmProc.h"

DWORD GameID = NULL;
VMM_HANDLE HV = NULL;

BOOL VMMDLL_Initialize()
{
	const char* zhiLing[] = { "", "-device", "fpga" };
	HV = VMMDLL_Initialize(3, (LPSTR*)zhiLing);
	return HV != NULL;
}

void SetProcessPid(DWORD dwPid)
{
	GameID = dwPid;
}

DWORD GetProcessPid(std::string strPorName)
{
	DWORD dwPID = 0;
	if (!VMMDLL_PidGetFromName(HV,(LPSTR)strPorName.c_str(), &dwPID))
		return 0;
	return dwPID;
}

std::string GetPidName(DWORD dwPid)
{
	LPSTR szName = VMMDLL_ProcessGetInformationString(HV,dwPid, 1);
	std::string str = szName;
	VMMDLL_MemFree(szName);
	return str;
}

std::string GetPidName2(DWORD dwPid)
{
	std::string str = "";
	PSIZE_T size = 0;
	
	VMMDLL_PROCESS_INFORMATION res;
	if (VMMDLL_ProcessGetInformation(HV,dwPid, &res, size))
	{
		str = res.szNameLong;;
	}
	return str;
}

std::vector<DWORD> GetProcessPidList()
{
	std::vector<DWORD> data;
	ULONG64 dwSize = 0;
	BOOL bIs=VMMDLL_PidList(HV,NULL, &dwSize);
	if (!bIs || dwSize == 0)
		return data;
	DWORD* pPid = new DWORD[dwSize];
	VMMDLL_PidList(HV,pPid, &dwSize);
	for (int i = 0; i < dwSize; i++)
	{
		data.push_back(pPid[i]);
	}
	delete[]pPid;
	pPid = NULL;
	return data;
}

uint64_t GetModuleFromName(std::string strName)
{
	PVMMDLL_MAP_MODULEENTRY ModuleEntryExplorer;
	VMMDLL_Map_GetModuleFromNameU(HV,GameID, (LPSTR)strName.c_str(), &ModuleEntryExplorer, NULL);
	return ModuleEntryExplorer->vaBase;
}

BOOL ReadMemory(uint64_t uBaseAddr,  LPVOID lpBuffer, DWORD nSize)
{
	return VMMDLL_MemRead(HV,GameID, uBaseAddr, (PBYTE)lpBuffer, nSize);
}

BOOL WriteMemory(uint64_t uBaseAddr, LPVOID lpBuffer, DWORD nSize) {
	return VMMDLL_MemWrite(HV,GameID, uBaseAddr, (PBYTE)lpBuffer, nSize);
}

BOOL MemWriteMemory(uint64_t uBaseAddr, _In_reads_(cb) PBYTE pb, _In_ DWORD cb) {
	return VMMDLL_MemWrite(HV, GameID, uBaseAddr, (PBYTE)pb, cb);
}

LPSTR ProcessGetInformationString(DWORD dwPID) {
	LPSTR szName = VMMDLL_ProcessGetInformationString(HV, dwPID, 1);

	return szName;
}


vector<BYTE> ReadBYTE(uint64_t ptr, SIZE_T size)
{
	vector<BYTE> BYTES;
	for (auto i = 0; i < size; i++) {
		BYTES.push_back(Read<BYTE>(ptr++));
	}
	return BYTES;
}

bool Scatter_Read(VMMDLL_SCATTER_HANDLE HS, uint64_t addr, PVOID pBuf, DWORD size)
{
	DWORD dwSize;
	return VMMDLL_Scatter_Read(HS, addr, size, (PBYTE)pBuf, &dwSize);
}

bool SPrepare(VMMDLL_SCATTER_HANDLE HS, uint64_t va, DWORD cb)
{
	return 	VMMDLL_Scatter_Prepare(HS, va, cb);
}

bool SClear(VMMDLL_SCATTER_HANDLE HS, DWORD flags)
{
	return  VMMDLL_Scatter_Clear(HS, GameID, flags);
}

VMMDLL_SCATTER_HANDLE Scatter_Initialize(DWORD flags)
{
	return  VMMDLL_Scatter_Initialize(HV,GameID, flags);
}

bool ExecuteRead(VMMDLL_SCATTER_HANDLE HS)
{
	return VMMDLL_Scatter_ExecuteRead(HS);
}


