/*
 *	���Ŀ¼����ԭTDIFWת��ΪLIB�ļ������Ժ�ʹ��
 */
#pragma once
#include <ipc.h>

// ԭDriverEntry��Ϊtdifw_DriverEntry
NTSTATUS tdifw_DriverEntry(IN PDRIVER_OBJECT theDriverObject,
				  IN PUNICODE_STRING theRegistryPath);


// ԭOnUnload��Ϊtdifw_OnUnload
VOID tdifw_OnUnload(IN PDRIVER_OBJECT DriverObject);

// ԭDeviceDispatch��Ϊtdifw_DeviceDispatch
NTSTATUS tdifw_DeviceDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp);

// ������¼�
int		tdifw_filter(struct flt_request *request);