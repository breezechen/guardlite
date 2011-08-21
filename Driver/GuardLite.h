#pragma once

#include <ntddk.h>
#include "../common/GuardLiteCtrl.h"

#define arrayof(p)		( sizeof(p) / sizeof((p)[0]) )

#define PAGEDCODE			code_seg("PAGE")
#define LOCKEDCODE			code_seg()
#define INITCODE			code_seg("INIT")

#define PAGEDDATA			data_seg("PAGE")
#define LOCKEDDATA			data_seg()
#define INITDATA			data_seg("INIT")

#define GUARDLITE_DEVICE		L"\\Device\\DDKGuardLite"
#define GUARDLITE_LINK			L"\\??\\GuardLite"

#define PAGE_DEBUG			'etiL'


// �豸����
typedef enum _GuardLiteDeviceType
{
	GLDT_Main = 0x0
	, GLDT_File
}GuardLiteDeviceType;

// IRP����
typedef struct _IRP_LIST{
	PIRP				pIrp;
	LIST_ENTRY			list;
}IRP_LIST, *PIRP_LIST;

// ��⵽һ��������������
typedef struct _GuardLiteQuery{
	GUARDLITEPACK			Pack;
	KEVENT					Event;
	ULONG					Timeout;
	ULONG					ulID;
}GUARDLITEQUERY, *PGUARDLITEQUERY;

// PACK����
typedef struct _PACK_LIST{
	GUARDLITEQUERY		query;
	LIST_ENTRY			list;
}PACK_LIST, *PPACK_LIST;

// Pack��Lookaside�ṹ
typedef struct _PACK_QUEUE{
	LIST_ENTRY					list;
	PNPAGED_LOOKASIDE_LIST		lookaside;
} PACK_QUEUE;
// IRP����
typedef struct _IRP_QUEUE{
	IRP_LIST					list;
	PNPAGED_LOOKASIDE_LIST		lookaside;	
} IRP_QUEUE;

// �豸��չ����
typedef struct _DEVICE_EXTENSION
{
	// �豸����
	GuardLiteDeviceType			DeviceType;
	// ������
	UNICODE_STRING				LinkName;
	// ������ص�Mask
	ULONG						StartMask;

}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//////////////////////////////////////////////////////////////////////////
// ȫ�ֱ���
extern PACK_QUEUE			gPackQueue;
extern LIST_ENTRY			gPackWaitList;
extern IRP_QUEUE			gIrpQueue;
extern KMUTEX				gIrpPackMutex;
extern ULONG				gPackWaitID;
//////////////////////////////////////////////////////////////////////////
// ������ģ�麯��
NTSTATUS	DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);
void		DriverOnload();
NTSTATUS	DriverDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS	DriverDeviceControlRuntine(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS	DriverCloseRuntine(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS	DriverCreateRuntine(PDEVICE_OBJECT pDevObj, PIRP pIrp);

//////////////////////////////////////////////////////////////////////////
// �ļ����ģ�麯��
NTSTATUS	FilemonEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);
void		FilemonUnload();
NTSTATUS	FilemonDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp);


//////////////////////////////////////////////////////////////////////////
// ע�����ģ�麯��
NTSTATUS	RegmonEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);
void		RegmonUnload();
NTSTATUS	RegmonDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp);


//////////////////////////////////////////////////////////////////////////
// ������ģ��
NTSTATUS	ServicesEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);
void		ServicesUnload();
NTSTATUS	ServicesDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp);


//////////////////////////////////////////////////////////////////////////
// ���̼��
NTSTATUS	ProcmonEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);
void		ProcmonUnload();
NTSTATUS	ProcmonDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp);


//////////////////////////////////////////////////////////////////////////
// ��������
NTSTATUS		AddIrpToQueue(PIRP pIrp);
NTSTATUS		AddPackToQueue(PGUARDLITEQUERY pQuery);
NTSTATUS		DealIrpAndPackQueue();
