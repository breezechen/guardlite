#include <ntddk.h>
#include "NetProc.h"
#include "tdifw/tdi_fw_lib.h"

NTSTATUS DriverEntry(IN PDRIVER_OBJECT theDriverObject,
						   IN PUNICODE_STRING theRegistryPath)
{
	tdifw_DriverEntry(theDriverObject, theRegistryPath);
	return STATUS_SUCCESS;
}

VOID OnUnload(IN PDRIVER_OBJECT DriverObject)
{
	tdifw_OnUnload(DriverObject);
}

NTSTATUS DeviceDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp)
{
	BOOLEAN		bDispath		= FALSE;
	NTSTATUS	status;

	// ����tdifw���¼�������
	status = tdifw_DeviceDispatch(DeviceObject, irp, &bDispath);
	if(FALSE != bDispath)
		return status;
	// �Լ��Ĵ�����


	return STATUS_SUCCESS;
}
/*
 *	�¼�����ص����� 
 */
int		tdifw_filter(struct flt_request *request)
{
	return FILTER_ALLOW;
}