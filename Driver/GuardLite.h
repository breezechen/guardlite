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
typedef struct _GuardLiteInnerPack{
	GUARDLITEREQUEST			Pack;		// �ⲿ�����ݽṹ
	KEVENT					Event;		// ���¼�
	BOOLEAN					Timeout;	// �Ƿ�ʱ
	BOOLEAN					Read;		// �Ƿ񱻶�ȡ
	BOOLEAN					Access;		// �Ƿ�ͨ��
}GUARDLITEINNERPACK, *PGUARDLITEINNERPACK;

// PACK����
typedef struct _InnerPackList{
	GUARDLITEINNERPACK		innerPack;
	LIST_ENTRY				list;
}INNERPACK_LIST, *PINNERPACK_LIST;

// Pack��Lookaside�ṹ
typedef struct _PACK_QUEUE{
	LIST_ENTRY					list;
	ULONG						ulWaitID;
	KMUTEX						mutex;
	NPAGED_LOOKASIDE_LIST		lookaside;
} PACK_QUEUE;

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

// IRP��ȡջ
typedef struct _IRP_Read_Stack{
	PIRP			irp[8];
	LONG			lPos;			// ��ǰָ��
	KSPIN_LOCK		spinkLock;		// ������
}IRP_READ_STACK;

//////////////////////////////////////////////////////////////////////////
// ȫ�ֱ���
extern PACK_QUEUE			gPackQueue;
extern IRP_READ_STACK		gIrpReadStack;
//////////////////////////////////////////////////////////////////////////
// ������ģ�麯��
NTSTATUS	DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);
void		DriverOnload();
NTSTATUS	DriverDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS	DriverDeviceControlRuntine(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS	DriverCloseRuntine(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS	DriverCreateRuntine(PDEVICE_OBJECT pDevObj, PIRP pIrp);
BOOLEAN		IsGuardStart();
void		DriverUnload(PDRIVER_OBJECT pDriverObject);

//////////////////////////////////////////////////////////////////////////
// �ļ����ģ�麯��
NTSTATUS	FilemonEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);
void		FilemonUnload();
NTSTATUS	FilemonDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp);

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
