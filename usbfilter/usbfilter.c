#include "usbfilter.h"
#include <srb.h>
#include <scsi.h>
#include <usbdi.h>
#include <usbdlib.h>

/*
 *	驱动入口函数
 */
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObj, PUNICODE_STRING pRegistryString)
{
	int				i;

	KdPrint(("usbfilter.sys!!! DriverEnter\n"));

	pDriverObj->DriverUnload = ufd_unload;
	pDriverObj->DriverExtension->AddDevice = ufd_add_device;
	
	for(i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		pDriverObj->MajorFunction[i] = ufd_dispatch_default;
	}

	pDriverObj->MajorFunction[IRP_MJ_POWER] = ufd_dispatch_power;
	pDriverObj->MajorFunction[IRP_MJ_PNP] = ufd_dispatch_pnp;
	pDriverObj->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = ufd_dispatch_internal_device_control;
//	pDriverObj->MajorFunction[IRP_MJ_SCSI] = ufd_dispatch_scsi;

	return STATUS_SUCCESS;
}

/*
 *	御载函数
 */
void ufd_unload(IN PDRIVER_OBJECT DriverObject)
{
	KdPrint(("usbfilter.sys!!! uf_driver_unload\n"));
}

/*
 *	添加新设备
 */
NTSTATUS ufd_add_device(IN PDRIVER_OBJECT DriverObject, 
						IN PDEVICE_OBJECT pdo)
{
	NTSTATUS							status;
	PDEVICE_OBJECT						fido;
	device_extension_ptr				pdx;
	PDEVICE_OBJECT						fdo;

	// 创建设备
	KdPrint(("usbfilter.sys!!! ufd_add_device enter[%wZ]\n", &DriverObject->DriverName));
	status = IoCreateDevice(DriverObject, sizeof(device_extension), NULL,
		ufd_get_device_type(pdo), 0, FALSE, &fido);
	if( !NT_SUCCESS(status) )
	{
		KdPrint(("usbfilter.sys!!! create device failed: %d\n", status));
		return status;
	}

	pdx = (device_extension_ptr)fido->DeviceExtension;
	do 
	{
		IoInitializeRemoveLock(&pdx->remove_lock, 0, 0, 0);
		pdx->device_object = fido;
		pdx->pdo = pdo;
		
		fdo = IoAttachDeviceToDeviceStack(fido, pdo);
		if(!fdo)
		{
			KdPrint(("usbfilter.sys!!! attach to device failed\n"));
			status = STATUS_DEVICE_REMOVED;
			break;
		}

		pdx->lower_device_object = fdo;

		fido->Flags |= fdo->Flags & (DO_DIRECT_IO | DO_BUFFERED_IO | DO_POWER_PAGABLE);
		fido->Flags &= ~DO_DEVICE_INITIALIZING;
	} while (FALSE);

	// 附加失败删除操作
	if( !NT_SUCCESS(status) )
	{
		if( pdx->lower_device_object )
		{
			IoDetachDevice(pdx->lower_device_object);
		}

		IoDeleteDevice(fido);
	}

	return status;
}
/*
 *	默认的分发函数
 */
NTSTATUS ufd_dispatch_default(IN PDEVICE_OBJECT device_object, IN PIRP irp)
{
	device_extension_ptr			pdx;
	PIO_STACK_LOCATION				stack;
	NTSTATUS						status;

	pdx = (device_extension_ptr)device_object->DeviceExtension;
	stack = IoGetCurrentIrpStackLocation(irp);
	KdPrint(("usbfilter.sys!!! [0x%x]ufd_dispatch_default: MJ(0x%x), MN(0x%x)\n",
		device_object, stack->MajorFunction, stack->MinorFunction));

	status = IoAcquireRemoveLock(&pdx->remove_lock, irp);
	if( !NT_SUCCESS(status) )
	{
		irp->IoStatus.Information = 0;
		irp->IoStatus.Status = status;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return status;
	}

	IoSkipCurrentIrpStackLocation(irp);
	status = IoCallDriver(pdx->lower_device_object, irp);
	IoReleaseRemoveLock(&pdx->remove_lock, irp);

	return status;
}
/*
 *	电源管理分发函数
 */
NTSTATUS ufd_dispatch_power(IN PDEVICE_OBJECT device_object, IN PIRP irp)
{
	device_extension_ptr		pdx;
	NTSTATUS					status;

	KdPrint(("usbfilter.sys!!! [0x%x]ufd_dispatch_power enter\n", device_object));
	pdx = (device_extension_ptr)device_object->DeviceExtension;
	PoStartNextPowerIrp(irp);
	status = IoAcquireRemoveLock(&pdx->remove_lock, irp);
	if( !NT_SUCCESS(status) )
	{
		irp->IoStatus.Status = status;
		irp->IoStatus.Information = 0;

		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return status;
	}

	IoSkipCurrentIrpStackLocation(irp);
	status = PoCallDriver(pdx->lower_device_object, irp);

	IoReleaseRemoveLock(&pdx->remove_lock, irp);
	return status;
}
/*
 *	对即插即用IRP进行处理
 */
NTSTATUS ufd_dispatch_pnp(IN PDEVICE_OBJECT device_object, IN PIRP irp)
{
	device_extension_ptr			pdx;
	PIO_STACK_LOCATION				stack;
	NTSTATUS						status;

	pdx = (device_extension_ptr)device_object->DeviceExtension;
	stack = IoGetCurrentIrpStackLocation(irp);
	KdPrint(("usbfilter.sys!!! [0xx%x]ufd_dispatch_pnp: MN(0x%x)\n", 
		device_object, stack->MinorFunction));
	
	status = IoAcquireRemoveLock(&pdx->remove_lock, irp);
	if( !NT_SUCCESS(status) )
	{
		irp->IoStatus.Information = 0;
		irp->IoStatus.Status = status;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		
		return status;
	}

	if(IRP_MN_START_DEVICE == stack->MinorFunction)
	{
		//status = ufd_dispatch_pnp_start_device(device_object, irp);
		if( !NT_SUCCESS(status) )
		{
			IoReleaseRemoveLock(&pdx->remove_lock, irp);
			return status;
		}
	}

	if(IRP_MN_DEVICE_USAGE_NOTIFICATION == stack->MinorFunction)
	{
		if(!device_object->AttachedDevice || 
			(device_object->AttachedDevice->Flags & DO_POWER_PAGABLE))
		{
			device_object->Flags |= DO_POWER_PAGABLE;
		}

		IoCopyCurrentIrpStackLocationToNext(irp);
		IoSetCompletionRoutine(irp, ufd_completion_usage_notification, 
			(PVOID)pdx, TRUE, TRUE, TRUE);

		return IoCallDriver(pdx->lower_device_object, irp);
	}

// 	if(IRP_MN_START_DEVICE == stack->MinorFunction)
// 	{
// 		IoCopyCurrentIrpStackLocationToNext(irp);
// 		IoSetCompletionRoutine(irp, ufd_completion_start_device,
// 			(PVOID)pdx, TRUE, TRUE, TRUE);
// 		
// 		return IoCallDriver(pdx->lower_device_object, irp);
// 	}

	if(IRP_MN_REMOVE_DEVICE == stack->MinorFunction)
	{
		IoSkipCurrentIrpStackLocation(irp);
		status = IoCallDriver(pdx->lower_device_object, irp);
		IoReleaseRemoveLockAndWait(&pdx->remove_lock, irp);

		ufd_driver_removedevice(device_object);
		return status;
	}

	IoSkipCurrentIrpStackLocation(irp);
	status = IoCallDriver(pdx->lower_device_object, irp);
	IoReleaseRemoveLock(&pdx->remove_lock, irp);

	return status;
}
/*
 *	开启设备
 */
NTSTATUS ufd_dispatch_pnp_start_device(IN PDEVICE_OBJECT device_object, 
									   IN PIRP irp)
{
	PDRIVER_OBJECT						pDriver;
	UNICODE_STRING						usName;
	device_extension_ptr				pdx;
	NTSTATUS							status				= STATUS_SUCCESS;
	USHORT								vid, pid;
	PWCHAR								pbuff				= NULL;
	PWCHAR								pManufacturer		= NULL;
	PWCHAR								pProduct			= NULL;
	PWCHAR								pSerialNumber		= NULL;
	UCHAR								uclass				= 0;

	pdx = (device_extension_ptr)device_object->DeviceExtension;
	// 获取设备Class
	status = ufd_get_usb_class(pdx->lower_device_object, &uclass);
	if( !NT_SUCCESS(status) )
	{
		goto ufd_dispatch_pnp_start_device_from_name;
	}
	// 验证设备是否放行
	status = ufd_check_usb_class(uclass);
	if( NT_SUCCESS(status) )
	{
		return STATUS_SUCCESS;
	}
	// 设备是阻止的，　看是否在放行列表
	// 获取设备信息
	pbuff = ExAllocatePoolWithTag(NonPagedPool, 64 * 3 * sizeof(WCHAR), 'ehom');
	if(NULL == pbuff)
	{
		goto ufd_dispatch_pnp_start_device_from_name;
	}
	
	pManufacturer = pbuff;
	pProduct = pManufacturer + 64;
	pSerialNumber = pProduct + 64;
	status = ufd_get_usb_info(pdx->lower_device_object, &vid, &pid, pManufacturer, 
		pProduct, pSerialNumber);
	if( !NT_SUCCESS(status) )
	{
		status = STATUS_UNSUCCESSFUL;
		goto  ufd_dispatch_pnp_start_device_end;
	}
	// 验证是否跳过设备
	status = ufd_check_usb_skip(vid, pid, pManufacturer, pProduct, pSerialNumber);
	if( NT_SUCCESS(status) )
	{
		status = STATUS_SUCCESS;
		goto ufd_dispatch_pnp_start_device_end;
	}
	else
	{
		status = STATUS_UNSUCCESSFUL;
		goto  ufd_dispatch_pnp_start_device_end;
	}

ufd_dispatch_pnp_start_device_from_name:
	// 最后根据驱动文件名称判断
	if(NULL == device_object->AttachedDevice)
	{
		status = STATUS_SUCCESS;
		goto ufd_dispatch_pnp_start_device_end;
	}

	pDriver = device_object->AttachedDevice->DriverObject;
	RtlInitUnicodeString(&usName, L"\\driver\\usbstor");
	KdPrint(("usbfilter.sys!!! IRP_MN_START_DEVICE: %wZ <=> %wZ\n", 
		&pDriver->DriverName, &usName));
	if( 0 == RtlCompareUnicodeString(&usName, &pDriver->DriverName, TRUE) )
	{
		// 获取设备名
		irp->IoStatus.Information = 0;
		irp->IoStatus.Status = STATUS_UNSUCCESSFUL;

		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_UNSUCCESSFUL;
	}

ufd_dispatch_pnp_start_device_end:
	if(NULL != pbuff)
	{
		ExFreePoolWithTag(pbuff, 'ehom');
	}

	return status;
}
/*
 *	SCSI例程
 */
NTSTATUS ufd_dispatch_scsi(IN PDEVICE_OBJECT device_object, IN PIRP irp)
{
	device_extension_ptr		pdx;
	PIO_STACK_LOCATION			stack;
	NTSTATUS					status;

	KdPrint(("usbfilter.sys!!! [0x%x]ufd_dispatch_scsi enter\n", device_object));
	pdx = (device_extension_ptr)device_object->DeviceExtension;
	stack = IoGetCurrentIrpStackLocation(irp);
	status = IoAcquireRemoveLock(&pdx->remove_lock, irp);
	if( !NT_SUCCESS(status) )
	{
		irp->IoStatus.Status = status;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return status;
	}

	IoCopyCurrentIrpStackLocationToNext(irp);
	IoSetCompletionRoutine(irp, ufd_completion_scsi,
		NULL, TRUE, TRUE, TRUE);
	status = IoCallDriver(pdx->lower_device_object, irp);
	IoReleaseRemoveLock(&pdx->remove_lock, irp);

	return status;
}
/*
 *	
 */
NTSTATUS ufd_dispatch_internal_device_control(IN PDEVICE_OBJECT device_object, 
											  IN PIRP irp)
{
	device_extension_ptr			pdx;
	PIO_STACK_LOCATION				stack;
	NTSTATUS						status;
	PURB							purb;

	pdx = (device_extension_ptr)device_object->DeviceExtension;
	stack = IoGetCurrentIrpStackLocation(irp);

	status = IoAcquireRemoveLock(&pdx->remove_lock, irp);
	if( !NT_SUCCESS(status) )
	{
		irp->IoStatus.Information = 0;
		irp->IoStatus.Status = status;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return status;
	}

	if( IOCTL_INTERNAL_USB_SUBMIT_URB == stack->Parameters.DeviceIoControl.IoControlCode
		&& NULL != (purb = (PURB)stack->Parameters.Others.Argument1))
	{
// 		KdPrint(("usbfilter.sys!!! device_control (code)%d: \n", 
// 			purb->UrbHeader.Function));
		if(URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER == purb->UrbHeader.Function)
		{
			if(0x1F == purb->UrbBulkOrInterruptTransfer.TransferBufferLength)
			{
				char*		pBuf		= NULL;
				
				if(NULL != purb->UrbBulkOrInterruptTransfer.TransferBuffer)
				{
					pBuf = purb->UrbBulkOrInterruptTransfer.TransferBuffer;
				}
				else
				{
					pBuf = MmGetMdlVirtualAddress(purb->UrbBulkOrInterruptTransfer.TransferBufferMDL);
				}

				// CBW结构体
				if(NULL != pBuf)
				{
// 					KdPrint(("usbfilter.sys!!! device_control (flag)%x \n", 
// 						purb->UrbBulkOrInterruptTransfer.TransferFlags));
					switch(pBuf[0xf])
					{
					case 0:
						break;
					default:
						KdPrint(("usbfilter.sys!!! sig(%c%c%c%c) tag(%X) opt(0x%x)\n",
							pBuf[0], pBuf[1], pBuf[2], pBuf[3],
							*((ULONG *)&pBuf[4]),
							(LONG)pBuf[0xf]
						));
						break;
					};
					

					if(SCSIOP_WRITE == pBuf[0xf])
					{
// 						pBuf[0xf] = SCSIOP_READ;
// 						pBuf[0xc] = 0x80;
						status = STATUS_MEDIA_WRITE_PROTECTED;
						irp->IoStatus.Status = status;
						irp->IoStatus.Information = 0;
						IoCompleteRequest(irp, IO_NO_INCREMENT);
						IoReleaseRemoveLock(&pdx->remove_lock, irp);
						return status;
					}

					if(/*SCSIOP_MODE_SENSE10 == pBuf[0xf]*/
					/*||*/ SCSIOP_MODE_SENSE == pBuf[0xf])
					{
						int			i;
						for(i = 0; i < 31; i++)
							KdPrint(("%x-", pBuf[i]));
						KdPrint(("\n"));
						IoCopyCurrentIrpStackLocationToNext(irp);
						IoSetCompletionRoutine(irp, ufd_completion_internal_device_control,
							(PVOID)pdx, TRUE, TRUE, TRUE);
						
						return IoCallDriver(pdx->lower_device_object, irp);
					}
				}
			}
			
// 			status = STATUS_UNSUCCESSFUL;
// 			irp->IoStatus.Status = status;
// 			irp->IoStatus.Information = 0;
// 			IoCompleteRequest(irp, IO_NO_INCREMENT);
// 			IoReleaseRemoveLock(&pdx->remove_lock, irp);
// 			return status;
		}
	}

	IoSkipCurrentIrpStackLocation(irp);
	status = IoCallDriver(pdx->lower_device_object, irp);
	IoReleaseRemoveLock(&pdx->remove_lock, irp);

	return status;
}
/*
 *	IRP_MN_DEVICE_USAGE_NOTIFICATION
 */
NTSTATUS ufd_completion_usage_notification(IN PDEVICE_OBJECT device_object,
										   IN PIRP irp, IN PVOID Context)
{
	device_extension_ptr		pdx;

	pdx = (device_extension_ptr)Context;

	if(irp->PendingReturned)
	{
		IoMarkIrpPending(irp);
	}

	if(!(pdx->lower_device_object->Flags & DO_POWER_PAGABLE))
	{
		device_object->Flags &= ~DO_POWER_PAGABLE;
	}

	IoReleaseRemoveLock(&pdx->remove_lock, irp);
	return STATUS_SUCCESS;
}
/*
 *	IRP_MN_START_DEVICE
 */
NTSTATUS ufd_completion_start_device(IN PDEVICE_OBJECT device_object, 
									 IN PIRP irp, IN PVOID Context)
{
	device_extension_ptr		pdx;

	pdx = (device_extension_ptr)Context;
	if(irp->PendingReturned)
	{
		IoMarkIrpPending(irp);
	}

	if(pdx->lower_device_object->Characteristics & FILE_REMOVABLE_MEDIA)
	{
		device_object->Characteristics |= FILE_REMOVABLE_MEDIA;
	}

	IoReleaseRemoveLock(&pdx->remove_lock, irp);

	return STATUS_SUCCESS;
}
/*
 *	IRP_MJ_SCSI
 */
NTSTATUS ufd_completion_scsi(IN PDEVICE_OBJECT device_object, 
							 IN PIRP irp, IN PVOID Context)
{
	device_extension_ptr		pdx;
	PIO_STACK_LOCATION			stack;
	PSCSI_REQUEST_BLOCK			srb;
	PCDB						cdb;
	CHAR						code;

	pdx = (device_extension_ptr)device_object->DeviceExtension;
	IoAcquireRemoveLock(&pdx->remove_lock, irp);
	stack = IoGetCurrentIrpStackLocation(irp);
	
	srb = stack->Parameters.Scsi.Srb;
	cdb = (PCDB)srb->Cdb;
	code = cdb->CDB6GENERIC.OperationCode;
	
	KdPrint(("usbfilter.sys!!! [0x%x]ufd_completion_scsi code=0x%x\n", 
		device_object, code));
	if(SCSIOP_MODE_SENSE == code && srb->DataBuffer
		&& srb->DataTransferLength >= sizeof(MODE_PARAMETER_HEADER))
	{
		PMODE_PARAMETER_HEADER		mode;

		KdPrint(("usbfilter.sys!!! read only!\n"));
		// U盘只读
		mode = (PMODE_PARAMETER_HEADER)srb->DataBuffer;
		mode->DeviceSpecificParameter |= MODE_DSP_WRITE_PROTECT;
	}

	if(irp->PendingReturned)
	{
		IoMarkIrpPending(irp);
	}

	IoReleaseRemoveLock(&pdx->remove_lock, irp);

	return irp->IoStatus.Status;
}

NTSTATUS ufd_completion_internal_device_control(IN PDEVICE_OBJECT device_object, 
											IN PIRP irp, IN PVOID Context)
{
	device_extension_ptr		pdx;
	PIO_STACK_LOCATION			stack;
	PURB						purb;
	char*						pBuf;
	PMODE_PARAMETER_HEADER		mode;

	pdx = (device_extension_ptr)device_object->DeviceExtension;
	stack = IoGetCurrentIrpStackLocation(irp);

	purb = (PURB)stack->Parameters.Others.Argument1;
	
	if(purb->UrbBulkOrInterruptTransfer.TransferBuffer)
	{
		pBuf = purb->UrbBulkOrInterruptTransfer.TransferBuffer;
	}
	else
	{
		pBuf = MmGetMdlVirtualAddress(purb->UrbBulkOrInterruptTransfer.TransferBufferMDL);
	}

	KdPrint(("[%x]usbfilter.sys!!! ufd_completion_internal_device_control sig(%c%c%c%c) %x-%x-%x-%x--%x-%x \n", 
		irp->IoStatus.Status,
		pBuf[0], pBuf[1], pBuf[2], pBuf[3],
		pBuf[12], pBuf[13], pBuf[14], pBuf[15], pBuf[12+7], pBuf[12+8]
		));
	{
		int			i;
		for(i = 0; i < 31; i++)
			KdPrint(("%x-", pBuf[i]));
		KdPrint(("\n"));
	}
	mode = (PMODE_PARAMETER_HEADER)&pBuf[12];
	
// 	if('USBW' 
// 		&& srb->DataTransferLength >= sizeof(MODE_PARAMETER_HEADER))
// 	{
// 		PMODE_PARAMETER_HEADER		mode;
// 
// 		KdPrint(("usbfilter.sys!!! read only!\n"));
// 		// U盘只读
// 		mode = (PMODE_PARAMETER_HEADER)srb->DataBuffer;
// 		mode->DeviceSpecificParameter |= MODE_DSP_WRITE_PROTECT;
// 	}

	if(irp->PendingReturned)
	{
		IoMarkIrpPending(irp);
	}

	IoReleaseRemoveLock(&pdx->remove_lock, irp);

	return irp->IoStatus.Status;
}
/*
 *	移除设备
 */
void ufd_driver_removedevice(IN PDEVICE_OBJECT device_object)
{
	device_extension_ptr		pdx;
	
	KdPrint(("usbfilter.sys!!! ufd_driver_removedevice\n"));
	pdx = (device_extension_ptr)device_object->DeviceExtension;
	if( pdx->lower_device_object )
	{
		IoDetachDevice(pdx->lower_device_object);
	}

	IoDeleteDevice(device_object);
}
/*
 *	获取设备的类型　
 */
ULONG ufd_get_device_type(PDEVICE_OBJECT pdo)
{
	PDEVICE_OBJECT			ldo;
	ULONG					type;

	ldo = IoGetAttachedDeviceReference(pdo);
	if(!ldo)
	{
		return FILE_DEVICE_UNKNOWN;
	}

	type = ldo->DeviceType;
	ObDereferenceObject(ldo);

	return type;
}
/*
 *	获取Ｕ盘信息
 */
NTSTATUS ufd_get_usb_info(PDEVICE_OBJECT fdo, USHORT* pvid, USHORT* ppid, 
						  WCHAR* pmanuf, WCHAR* pproduct, WCHAR* psn)
{
	NTSTATUS							status;
	URB									urb;
	PUSB_STRING_DESCRIPTOR				pusd			= NULL;
	PUSB_DEVICE_DESCRIPTOR				pudd			= NULL;
	LONG								size;

	//获取设备信息
	size = sizeof(USB_DEVICE_DESCRIPTOR);
	pudd = ExAllocatePoolWithTag(NonPagedPool, size, 'ehom');
	UsbBuildGetDescriptorRequest(&urb, 
		(USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
		USB_DEVICE_DESCRIPTOR_TYPE, 0, 0, pudd, NULL, 
		size, NULL);
	status = ufd_CallUSBD(fdo, &urb);
	if( !NT_SUCCESS(status) )
	{
		KdPrint(("ufd_get_usb_info get USB_DEVICE_DESCRIPTOR_TYPE failed: 0x%x",
			status));
		goto ufd_get_usb_info_end;
	}

	*pvid = pudd->idVendor;
	*ppid = pudd->idProduct;
	// 获取生产厂家字符串
	size = 128;
	pusd = ExAllocatePoolWithTag(NonPagedPool, size, 'ehom');
	RtlZeroMemory(pusd, size);
	UsbBuildGetDescriptorRequest(&urb, 
		(USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
		USB_STRING_DESCRIPTOR_TYPE, pudd->iManufacturer, 936, pusd, NULL, 
		size, NULL);
	status = ufd_CallUSBD(fdo, &urb);
	if( NT_SUCCESS(status) )
	{
		wcsncpy(pmanuf, pusd->bString, min(wcslen(pusd->bString), 63));
	}
	// 获取产品字符串
	RtlZeroMemory(pusd, size);
	UsbBuildGetDescriptorRequest(&urb, 
		(USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
		USB_STRING_DESCRIPTOR_TYPE, pudd->iProduct, 936, pusd, NULL, 
		size, NULL);
	status = ufd_CallUSBD(fdo, &urb);
	if( NT_SUCCESS(status) )
	{
		wcsncpy(pproduct, pusd->bString, min(wcslen(pusd->bString), 63));
	}
	// 获取序列号符串
	RtlZeroMemory(pusd, size);
	UsbBuildGetDescriptorRequest(&urb, 
		(USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
		USB_STRING_DESCRIPTOR_TYPE, pudd->iSerialNumber, 936, pusd, NULL, 
		size, NULL);
	status = ufd_CallUSBD(fdo, &urb);
	if( NT_SUCCESS(status) )
	{
		wcsncpy(psn, pusd->bString, min(wcslen(pusd->bString), 63));
	}
	
ufd_get_usb_info_end:
	if(NULL != pudd)
	{
		ExFreePoolWithTag(pudd, 'ehom');
	}
	if(NULL != pusd)
	{
		ExFreePoolWithTag(pusd, 'ehom');
	}

	return status;
}
/*
 *	获取USB设备的类名
 */
NTSTATUS ufd_get_usb_class(PDEVICE_OBJECT fdo, UCHAR* pclass)
{
	NTSTATUS							status			= STATUS_SUCCESS;
	URB									urb;
	PUSB_INTERFACE_DESCRIPTOR			puid			= NULL;
	PUSB_CONFIGURATION_DESCRIPTOR		pucd			= NULL;
	LONG								size;

	size = sizeof(USB_CONFIGURATION_DESCRIPTOR);
	pucd = ExAllocatePoolWithTag(NonPagedPool, size, 'ehom');
	UsbBuildGetDescriptorRequest(&urb, 
		(USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
		USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, pucd, NULL, 
		size, NULL);
	status = ufd_CallUSBD(fdo, &urb);
	if( !NT_SUCCESS(status) )
	{
		goto ufd_get_usb_class;
	}

	size = pucd->wTotalLength;
	ExFreePoolWithTag(pucd, 'ehom');
	pucd = ExAllocatePoolWithTag(NonPagedPool, size, 'ehom');
	UsbBuildGetDescriptorRequest(&urb, 
		(USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
		USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, pucd, NULL, 
		size, NULL);
	status = ufd_CallUSBD(fdo, &urb);
	if( !NT_SUCCESS(status) )
	{
		goto ufd_get_usb_class;
	}

#define INTERFACENUMBER 0  
#define ALTERNATESETTING 1  
	puid = USBD_ParseConfigurationDescriptorEx(pucd, pucd, 
		INTERFACENUMBER,
		0/*ALTERNATESETTING*/,
		-1, -1, -1);
	if(NULL == puid)
	{
		status = STATUS_UNSUCCESSFUL;
		goto ufd_get_usb_class;
	}

	*pclass = puid->bInterfaceClass;
ufd_get_usb_class:
	if(NULL != pucd)
	{
		ExFreePoolWithTag(pucd, 'ehom');
	}

	return status;
}
/*
 *	检查设备类型
 */
NTSTATUS ufd_check_usb_class(UCHAR uclass)
{
	if(USB_DEVICE_CLASS_STORAGE == uclass 
		|| USB_DEVICE_CLASS_PRINTER == uclass)
	{
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}
/*
 *	查看设备是否跳过的设备
 */
NTSTATUS ufd_check_usb_skip(USHORT vid, USHORT pid, WCHAR* pmanuf, 
							WCHAR* pproduct, WCHAR* psn)
{
	return STATUS_UNSUCCESSFUL;
}
/*
 *	调用usb驱动
 */
NTSTATUS ufd_CallUSBD(IN PDEVICE_OBJECT fdo, IN PURB Urb)
{  
   NTSTATUS					ntStatus, status = STATUS_SUCCESS;  
   PIRP						irp;  
   KEVENT					event;  
   IO_STATUS_BLOCK			ioStatus;  
   PIO_STACK_LOCATION		nextStack;  
  
     // issue a synchronous request (see notes above)  
   KeInitializeEvent(&event, NotificationEvent, FALSE);  
  
   irp = IoBuildDeviceIoControlRequest(  
             IOCTL_INTERNAL_USB_SUBMIT_URB,  
             fdo,  
             NULL,  
             0,  
             NULL,  
             0,  
             TRUE, /* INTERNAL */  
             &event,  
             &ioStatus);  
  
   // Prepare for calling the USB driver stack  
   nextStack = IoGetNextIrpStackLocation(irp);  
   ASSERT(nextStack != NULL);  
  
   // Set up the URB ptr to pass to the USB driver stack  
   nextStack->Parameters.Others.Argument1 = Urb;  
  
    ntStatus = IoCallDriver(fdo, irp);  
    
   if (ntStatus == STATUS_PENDING)  
   {  
      status = KeWaitForSingleObject(  
                    &event,  
                    Suspended,  
                    KernelMode,  
                    FALSE,  
                    NULL);  
   }  
   else  
   {  
      ioStatus.Status = ntStatus;  
   }  
  
   ntStatus = ioStatus.Status;  
  
   if (NT_SUCCESS(ntStatus))  
   {  
      if (!(USBD_SUCCESS(Urb->UrbHeader.Status)))  
         ntStatus = STATUS_UNSUCCESSFUL;  
   } 
  
   return ntStatus;  
}