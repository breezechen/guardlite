/*
 *	获取内核态的EXPLORER结构的
 */
#ifndef _PROCESS_HANDLE_
#define _PROCESS_HANDLE_

// 获取PEB
BOOLEAN EPROCESS_PPEB(PEPROCESS pEproc, PPEB* pPeb);
BOOLEAN PEB_ImageBaseAddress(PPEB pPeb, PVOID* pBase);
BOOLEAN PEB_Ldr(PPEB pPeb, PVOID* pLdr);
BOOLEAN PEB_ProcessParameters(PPEB pPeb, PVOID* pParam);

BOOLEAN RTL_USER_PROCESS_PARAMETERS_DllPath(PVOID pParam, PUNICODE_STRING* pDllPath);
BOOLEAN RTL_USER_PROCESS_PARAMETERS_ImagePathName(PVOID pParam, PUNICODE_STRING* pPathName);
BOOLEAN RTL_USER_PROCESS_PARAMETERS_CommandLine(PVOID pParam, PUNICODE_STRING* pCommandLine);


BOOLEAN PEB_LDR_DATA_InLoadOrderModuleList(PVOID pLdr, PLIST_ENTRY* pEntry);
BOOLEAN LDR_DATA_TABLEFromInLoadOrderModuleList(PVOID pInload, PVOID* pLdr);

BOOLEAN LDR_DATA_TABLE_DllBase(PVOID pLdr, PVOID* pBase);
BOOLEAN LDR_DATA_TABLE_EntryPoint(PVOID pLdr, PVOID* pPoint);
BOOLEAN LDR_DATA_TABLE_SizeOfImage(PVOID pLdr, UINT32* pSize);
BOOLEAN LDR_DATA_TABLE_FullDllName(PVOID pLdr, PUNICODE_STRING* pFullName);
BOOLEAN LDR_DATA_TABLE_BaseDllName(PVOID pLdr, PUNICODE_STRING* pDllName);
#endif //#ifndef _PROCESS_HANDLE_