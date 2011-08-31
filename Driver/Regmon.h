#pragma once

// ע�����ģ�麯��
NTSTATUS	RegmonEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);
void		RegmonUnload();
NTSTATUS	RegmonDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp);

typedef struct _SRVTABLE {
	PVOID           *ServiceTable;
	ULONG           LowCall;        
	ULONG           HiCall;
	PVOID		*ArgTable;
} SRVTABLE, *PSRVTABLE;	

typedef struct _rootkey {
	WCHAR                RootName[256];
	WCHAR                RootShort[32];
	ULONG				RootNameLen;
} ROOTKEY, *PROOTKEY;

// DDK ������C++
extern PSRVTABLE	KeServiceDescriptorTable;	// ϵͳSSDT��
extern PSRVTABLE    ServiceTable;

// ��ʵ��д��ַ
typedef NTSTATUS (*fnRealRegSetValueKey)( IN HANDLE KeyHandle, IN PUNICODE_STRING ValueName,
										 IN ULONG TitleIndex, IN ULONG Type, 
										 IN PVOID Data, IN ULONG DataSize );

NTSTATUS RegSetValueKey( IN HANDLE KeyHandle, IN PUNICODE_STRING ValueName,
						IN ULONG TitleIndex, IN ULONG Type, 
						IN PVOID Data, IN ULONG DataSize );

// ����ϵͳ����
#if defined(_ALPHA_)
#define SYSCALL(_function)  ServiceTable->ServiceTable[ (*(PULONG)_function)  & 0x0000FFFF ]
#else
#define SYSCALL(_function)  ServiceTable->ServiceTable[ *(PULONG)((PUCHAR)_function+1)]
#endif

void		EnableServiceTable();
void		DisableServiceTable();

#define SETSYSCALL(_fun, _setfun)		{ \
	EnableServiceTable(); \
	SYSCALL(_fun) = (PVOID)_setfun;\
	DisableServiceTable(); \
} \

