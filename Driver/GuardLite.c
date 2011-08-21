/*
 *	������ģ��
 */
#include "GuardLite.h"

#pragma LOCKEDDATA
PACK_QUEUE		gPackQueue;
#pragma LOCKEDDATA
LIST_ENTRY		gPackWaitList;
#pragma LOCKEDDATA
IRP_QUEUE		gIrpQueue;
#pragma LOCKEDDATA
KMUTEX			gIrpPackMutex;
#pragma LOCKEDDATA
ULONG			gPackWaitID;

// ����������
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS			status;
	int					i;
	UNICODE_STRING		usDeviceName		= RTL_CONSTANT_STRING(GUARDLITE_DEVICE);
	UNICODE_STRING		usLinkName			= RTL_CONSTANT_STRING(GUARDLITE_LINK);
	PDEVICE_OBJECT		pCtrlDev			= NULL;
	PDEVICE_EXTENSION	pDevExt				= NULL;

	for(i = 0; i < arrayof(pDriverObject->MajorFunction); i++)
	{
		pDriverObject->MajorFunction[i] = DriverDispatchRoutine;
	}
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDeviceControlRuntine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCloseRuntine;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreateRuntine;
	// ���������豸
	status = IoCreateDevice(pDriverObject
		, sizeof(DEVICE_EXTENSION)
		, &usDeviceName
		, FILE_DEVICE_UNKNOWN
		, 0
		, TRUE
		, &pCtrlDev);
	if(!NT_SUCCESS(status))
	{
		IoDeleteDevice(pCtrlDev);
		return status;
	}
	// ��������
	status = IoCreateSymbolicLink(&usLinkName, &usDeviceName);
	if(!NT_SUCCESS(status))
	{
		IoDeleteDevice(pCtrlDev);
		return status;
	}
	// �����豸����
	pDevExt = (PDEVICE_EXTENSION)pCtrlDev->DeviceExtension;
	pCtrlDev->Flags |= DO_BUFFERED_IO;
	pDevExt->DeviceType = GLDT_Main;
	RtlCopyUnicodeString(&pDevExt->LinkName, &usLinkName);
	pDevExt->StartMask = 0L;
	// ��ʼ��IRP����
	InitializeListHead(&gIrpQueue.list);
	ExInitializeNPagedLookasideList(&gIrpQueue.lookaside, NULL, NULL, 0, sizeof(IRP_LIST), PAGE_DEBUG, 0);
	// ��ʼ��PACK����
	InitializeListHead(&gPackQueue.list);
	ExInitializeNPagedLookasideList(&gPackQueue.lookaside, NULL, NULL, 0, sizeof(PACK_LIST), PAGE_DEBUG, 0);
	// �ȴ�����
	InitializeListHead(&gPackWaitList);
	gPackWaitID = 0L;
	// ��ʼ��������
	KeInitializeMutex(&gIrpPackMutex, 0);

	return status;
}

// Ĭ������
NTSTATUS DriverDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

// ������
NTSTATUS DriverCreateRuntine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

// �ر�����
NTSTATUS DriverCloseRuntine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

// ��������
NTSTATUS DriverDeviceControlRuntine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

