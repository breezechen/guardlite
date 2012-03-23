/*  
// //////////////////////////////////////////////////////////////////////////////////////////
//             EHomeNet 
//             Edit by Chao Ding 08-20-2010
*/


#include "EhomeNet.h"
#include "EhomeDevCtl.h"
#include "Keyword.h"

// ����������
int							NamePos						= 0;
// ��������PEPROCESS�ṹ�е�����
PEPROCESS					gControlPID					= NULL;
PFILE_OBJECT				hcFileObj					= NULL;
URLINFO						storageurl					= {0};
char						DnsName[NAMELENGTH]			= {0};
URLINFO						HostInfo					= {0};
ULONG						IsHttpAllow					= 1;
ULONG						IsHttpFilterOn				= 0;
PKEVENT						EhomeUsrEvent				= NULL;
PDEVICE_OBJECT				EhomeCtlDev					= NULL;
PKEVENT						UrlAllowOrNotEvent			= NULL;
PKMUTEX						UrlNameMutex				= NULL;
EHOME_FILTER_RULE			gEHomeFilterRule			= {0};
EHOME_FILTER_KEYWORD		gEHomeKeyword				= {0};
// NPAGED_LOOKASIDE_LIST		gPagedLookaside				= {0};
// PNPAGED_LOOKASIDE_LIST		NpagedLookaside				= &gPagedLookaside;
// һ���µ�TCP������ʱ��洢��EHOME_LIST�ṹ��������
// PLIST_ENTRY					pEhomelistHeader			= NULL;
// ����������־
CTRLNETWORK*				gpCtrlNetWork				= NULL;
LONG						gRefCount					= 0L;
LONG						gEhomeClear					= 3L;

/*������ں���*/
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObj, PUNICODE_STRING pRegistryString)
{
	NTSTATUS				status					= STATUS_SUCCESS;
	UNICODE_STRING			TcpDevname,EHomelinkname,EhomeDevName,UdpDevName;
	PDEVICE_OBJECT			TcpDevobj = NULL, LowTcpDevObj = NULL, EhomefltDev = NULL;
	PDEVICE_OBJECT			UdpDevobj = NULL, LowUdpDevObj = NULL, EhomefltUdpDev = NULL;
	PFILE_OBJECT			TcpFileobj = NULL, UdpFileobj = NULL;
	PEPROCESS 				epro;
	int						i;
	PDEVICE_EXTENTION		DevExt;
	
	epro = PsGetCurrentProcess();
	UrlAllowOrNotEvent = (PKEVENT)ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), 'ehom');
	UrlNameMutex = (PKMUTEX)ExAllocatePoolWithTag(NonPagedPool, sizeof(KMUTEX), 'ehom');
	gpCtrlNetWork = (CTRLNETWORK*)ExAllocatePoolWithTag(NonPagedPool, 1024, 'ehom');
	// ��ΪEHOME_LIST��Ƶ��������ͻ��գ���������ڴ��©����ʹ��LOOKASIDE�ṹ
	if(!UrlAllowOrNotEvent && !UrlNameMutex && !gpCtrlNetWork)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto failed;
	}
	// �ҵ���������PEPROCESS�ṹ�е�����
	for(NamePos = 0; NamePos < 1024; NamePos++)
	{
		if(0 == _strnicmp((char*)epro + NamePos, "System", 6))
			break;
	}
	// ��ʼ���¼�����������������
	KeInitializeEvent(UrlAllowOrNotEvent, NotificationEvent, TRUE);
	KeInitializeMutex(UrlNameMutex, 0);
	////KdPrint(("Enter EhomeDrvEntry\n"));
	// ������ǲ����
	for(i=0;i<IRP_MJ_MAXIMUM_FUNCTION;i++)
	{
		pDriverObj->MajorFunction[i] = Ehomedisp;
	}
	pDriverObj->MajorFunction[IRP_MJ_CLOSE] = EhomeCloseCleanup;
	pDriverObj->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = EhomeInternalDevCtl;
	pDriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = EhomeDevCtl;
#if defined(DBG) && !defined(NO_UNLOAD)
	pDriverObj->DriverUnload = EhomeUnload;
#endif
	// ��ʼ������״̬
	RtlZeroMemory(gpCtrlNetWork, 1024);
	gpCtrlNetWork->code = 1;
	// ��ʼ���豸��ص�����
	RtlInitUnicodeString(&TcpDevname, TCPDEVNAME);
	RtlInitUnicodeString(&EhomeDevName, EHOMETCPDEVNAME);
	RtlInitUnicodeString(&EHomelinkname, EHOMELINKNAME);
	RtlInitUnicodeString(&UdpDevName, UDPDEVNAME);
	// ��ȡ"\\Device\\Tcp"�豸�Ķ���
	status=IoGetDeviceObjectPointer(&TcpDevname,FILE_ALL_ACCESS, &TcpFileobj, &TcpDevobj);
	if(!NT_SUCCESS(status))
		goto failed;
	status = IoGetDeviceObjectPointer(&UdpDevName, FILE_ALL_ACCESS, &UdpFileobj, &UdpDevobj);
	if(!NT_SUCCESS(status))
		goto failed;
	// ���������豸"\\Device\\EHomeNetFltDev"
	status=IoCreateDevice(pDriverObj, sizeof(DEVICE_EXTENTION), &EhomeDevName
		, FILE_DEVICE_UNKNOWN, 0, FALSE, &EhomeCtlDev);
	if(!NT_SUCCESS(status))
		goto failed;
	// �����豸�����豸
	status=IoCreateDevice(pDriverObj, sizeof(DEVICE_EXTENTION), NULL
		, FILE_DEVICE_UNKNOWN, 0, FALSE, &EhomefltDev);
	if(!NT_SUCCESS(status))
		goto failed;
	// ����UDP�����豸
	status = IoCreateDevice(pDriverObj, sizeof(DEVICE_EXTENTION), NULL
		, FILE_DEVICE_UNKNOWN, 0, FALSE, &EhomefltUdpDev);
	if(!NT_SUCCESS(status))
		goto failed;
	// �����豸�ķ�������"\\??\\EHomeNetDev"
	status=IoCreateSymbolicLink(&EHomelinkname, &EhomeDevName);
	if(!NT_SUCCESS(status))
	{
		//KdPrint(("Create Symbok link failed"));
		goto failed;
	}
	// ��ʼ��
	tdi_foc_Init();
	keyword_Init();
	// ���ӹ����豸��"\\Device\\Tcp"���豸ջ
	LowTcpDevObj = IoAttachDeviceToDeviceStack(EhomefltDev, TcpDevobj);
	LowUdpDevObj = IoAttachDeviceToDeviceStack(EhomefltUdpDev, UdpDevobj);
	if( !LowTcpDevObj || !LowUdpDevObj )
	{
		IoDeleteSymbolicLink(&EHomelinkname);
		status = STATUS_NOT_COMMITTED;
		goto failed;
	}

	// TCP�����豸�͸��ӳɹ������ñ���
	DevExt = (PDEVICE_EXTENTION)EhomefltDev->DeviceExtension;
	DevExt->DeviceType = DT_FILTER_TCP;
	DevExt->linkName = EHomelinkname;
	DevExt->LowTcpDev = LowTcpDevObj;
	EhomefltDev->Flags = LowTcpDevObj->Flags;
	EhomefltDev->Flags |= ~DO_DEVICE_INITIALIZING;
	EhomefltDev->Characteristics = LowTcpDevObj->Characteristics;
	// UDP�����豸�͸��ӳɹ�, ���ñ���
	DevExt = (PDEVICE_EXTENTION)EhomefltUdpDev->DeviceExtension;
	DevExt->DeviceType = DT_FILTER_UDP;
	DevExt->linkName = EHomelinkname;
	DevExt->LowTcpDev = LowUdpDevObj;
	EhomefltUdpDev->Flags = LowUdpDevObj->Flags;
	EhomefltUdpDev->Flags |= ~DO_DEVICE_INITIALIZING;
	EhomefltUdpDev->Characteristics = LowUdpDevObj->Characteristics;
	// ��ʼ�������豸����
	EhomeCtlDev->Flags |= DO_BUFFERED_IO;
	DevExt = (PDEVICE_EXTENTION)EhomeCtlDev->DeviceExtension;
	DevExt->linkName = EHomelinkname;
	DevExt->DeviceType = DT_EHOME;
	//KdPrint(("leave Ehome DriverEntry\n"));	
	gEhomeClear = 3L;
	// ��ʼ����Ϣ
	InitializeListHead(&gEHomeKeyword.headlist);
	KeInitializeSpinLock(&gEHomeKeyword.spinlock);
	ExInitializeNPagedLookasideList(&gEHomeKeyword.lookaside, NULL, NULL, 0, sizeof(FILTER_KEYWORD_BLOCK), 'ehom', 0);
	gEHomeKeyword.noticeevent = NULL;
	goto end;
failed:
	// ʧ�ܺ����������ڴ�
	gEhomeClear = 0L;
	EhomeClear();
	tdi_foc_Release();
	keyword_Release();
	// ʧ�ܺ�ɾ���������豸
	if(NULL != EhomeCtlDev)
		IoDeleteDevice(EhomeCtlDev);
	if(NULL != EhomefltDev)
		IoDeleteDevice(EhomefltDev);
	if(NULL != LowTcpDevObj)
		IoDetachDevice(LowTcpDevObj);
	if(NULL != LowUdpDevObj)
		IoDetachDevice(LowUdpDevObj);
end:
	// �˳�ʱ�ر�һЩ���
	if(NULL != TcpFileobj)
		ObDereferenceObject(TcpFileobj);
	if(NULL != UdpFileobj)
		ObDereferenceObject(UdpFileobj);

	return STATUS_SUCCESS;
}

// IRP_MJ_INTERNAL_DEVICE_CONTROL��ǲ����
NTSTATUS EhomeInternalDevCtl(PDEVICE_OBJECT pDevObj,PIRP irp)
{
	PEPROCESS					epro;
	TA_ADDRESS *				remote_addr;
	USHORT						port;
	ULONG						IPAdd;
	NTSTATUS					status;
	char*						httpPacket			= NULL;
	PVOID						dnspacket			= NULL;
	TDI_REQUEST_KERNEL_SEND*	reqSend;
	ULONG						i,j,k;
	PIO_STACK_LOCATION			stack;
	PDEVICE_EXTENTION			DevExt;
	int							lenth;
	TDI_REQUEST_KERNEL *		req;
	TDI_REQUEST_KERNEL_SENDDG*	requdp;
	tdi_foc_connection_ptr			pAddress		= NULL;

	InterlockedIncrement(&gRefCount);
	stack = IoGetCurrentIrpStackLocation(irp);	
	DevExt=(PDEVICE_EXTENTION)pDevObj->DeviceExtension;
	
	epro = PsGetCurrentProcess();
	if(epro == gControlPID)
		goto skipirp;	// �������ƽ���

	// ����ģʽ
	if(0 == gpCtrlNetWork->code)
	{
		status = CheckNetwork(stack, DevExt, (char *)epro + NamePos);
		if(NT_SUCCESS(status))
			goto skipirp;
		goto stopirp;	// ֹͣIRP
	}
	if(1 == irp->CurrentLocation)   
	{   
		KdPrint(("PacketDispatch encountered bogus current location\n"));   

		IoSkipCurrentIrpStackLocation(irp);
		return IoCallDriver(DevExt->LowTcpDev, irp);
	}
	// ��ͨ���ģʽ
	if(stack->MinorFunction == TDI_SEND)
	{
		tdi_foc_ptr		pSocketContext		= NULL;

		if(0 == IsHttpFilterOn)
			goto skipirp;
		// �������ݹ��˹���
		reqSend = (TDI_REQUEST_KERNEL_SEND *)&stack->Parameters;
		// ���˶�˿�����
		if(tdi_foc_CheckConnection(stack->FileObject, &pAddress, &pSocketContext))
		{
			BOOLEAN		bIsHttp			= pAddress->bChecked;
			
			if( FALSE != pSocketContext->bStopOption )
			{
				status = STATUS_INVALID_CONNECTION;
				goto stopirp;
			}
			
			status = STATUS_SUCCESS;
			httpPacket = (char*)MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
			if(FALSE == pAddress->bChecked)
			{
				if(KeGetCurrentIrql() >= DISPATCH_LEVEL)
				{
					KdPrint(("[EhomeInternalDevCtl] TDI_SEND IRQL: %d\n", KeGetCurrentIrql()
						)); 
					goto skipirp; // ����APC����Ͳ��������˳�
				}
				// ����ȴ�
				KeWaitForSingleObject(UrlNameMutex, Executive, KernelMode, FALSE, NULL); 
				// ֻ����GET����
				status = CheckUrl(httpPacket, reqSend->SendLength, pAddress, &bIsHttp);
				KeReleaseMutex(UrlNameMutex, FALSE);
				// �����ǣ�ʹ�䲻���ٴα���ȡ
				if(FALSE != bIsHttp)
				{
					tdi_foc_ptr		pSockAddress		= tdi_foc_GetAddress(pSocketContext->pConnectFileObj, FALSE);

					pAddress->bChecked = TRUE;
					pSockAddress->bIsHttp = TRUE;
				}
			}
			// �滻HTTP��ѹ����־
			if(FALSE != bIsHttp)
			{
				__try{
					// ��ֹ��ȡԽ������
					if(0 != gEHomeFilterRule.rule)
						HttpRequestEraseFlag(httpPacket, reqSend->SendLength);
				} 
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					goto skipirp;
				}
			}
			if(NT_SUCCESS(status))
				goto skipirp;
			goto stopirp;	// ֹͣ����
		}
	}
	else if(stack->MinorFunction == TDI_CONNECT)
	{
		// ������ж˿�
		IoCopyCurrentIrpStackLocationToNext(irp);
		IoSetCompletionRoutine(irp, EhomeConnectComRoutine, NULL, TRUE, TRUE, TRUE);
		status = IoCallDriver(DevExt->LowTcpDev, irp);
		return status;
	}
	else if(TDI_SET_EVENT_HANDLER == stack->MinorFunction)
	{
		status = EHomeTDISetEventHandler(irp, stack);
		if(!NT_SUCCESS(status))
			goto stopirp;
	}
	else if(TDI_RECEIVE == stack->MinorFunction)
	{
		tdi_client_irp_ctx*		new_ctx = (tdi_client_irp_ctx *)ExAllocatePoolWithTag(NonPagedPool, sizeof(tdi_client_irp_ctx), 'ehom');

		if(NULL != new_ctx)
		{
			RtlZeroMemory( new_ctx, sizeof(tdi_client_irp_ctx) );
			new_ctx->addrobj = stack->FileObject;
		}
		IoCopyCurrentIrpStackLocationToNext(irp);
		IoSetCompletionRoutine(irp, tdi_client_irp_complete, new_ctx, TRUE, TRUE, TRUE);
		status = IoCallDriver(DevExt->LowTcpDev, irp);
		return status;
	}
	else if(TDI_ASSOCIATE_ADDRESS == stack->MinorFunction)
	{
		// ������ַ����Ͷ˿ڶ���
		PFILE_OBJECT			pAddressFileObj		= NULL;
		tdi_foc_ptr		pSocketAddress		= NULL;
		tdi_foc_ptr		pSocketConnet		= NULL;

		status = ObReferenceObjectByHandle(((PTDI_REQUEST_KERNEL_ASSOCIATE)&stack->Parameters)->AddressHandle
			, FILE_ANY_ACCESS, 0, KernelMode, (PVOID *)&pAddressFileObj, NULL);
		KdPrint(("[EhomeInternalDevCtl] TDI_ASSOCIATE_ADDRESS PortObj: %x, AddrObj: %x \n", stack->FileObject, pAddressFileObj));
		pSocketAddress = tdi_foc_GetAddress(pAddressFileObj, TRUE);
		pSocketConnet = tdi_foc_GetConnection(stack->FileObject, TRUE);
		// ��ַ����
		if(NULL != pSocketAddress)
		{
			pSocketAddress->pConnectFileObj = stack->FileObject;
		}
		// ���Ӷ���
		if(NULL != pSocketConnet)
		{
			pSocketConnet->pConnectFileObj = pAddressFileObj;
		}
	}

	goto skipirp;
stopirp:
	// ֹͣIRP
	irp->IoStatus.Information = 0;
	irp->IoStatus.Status = status;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	InterlockedDecrement(&gRefCount);
	return status;
skipirp:
	// ��ͨģʽ
#ifdef NO_UNLOAD
	IoSkipCurrentIrpStackLocation(irp);
#else
	IoCopyCurrentIrpStackLocationToNext(irp);
	IoSetCompletionRoutine(irp, DispatchRoutineComplate, NULL, TRUE, TRUE, TRUE);
#endif
	return IoCallDriver(DevExt->LowTcpDev,irp);
}
// ��������, UDP�ǰѷ��ͳ�����0, TCP�ǰ����ӶϿ�
NTSTATUS CheckNetwork(PIO_STACK_LOCATION pStack, PDEVICE_EXTENTION pDevExt, char* pProcName)
{
	NTSTATUS			status		= STATUS_SUCCESS;

	if(DT_FILTER_UDP == pDevExt->DeviceType)
	{
		if(TDI_SEND_DATAGRAM == pStack->MinorFunction)
		{
			PTDI_REQUEST_KERNEL_SENDDG			requdp			= NULL;
			PTA_ADDRESS							remote_addr		= NULL;
			USHORT								port			= 0;

			requdp = (PTDI_REQUEST_KERNEL_SENDDG)&pStack->Parameters;
			remote_addr = (PTA_ADDRESS)((PTA_ADDRESS)requdp->SendDatagramInformation->RemoteAddress)->Address;
			port = my_ntohs(((TDI_ADDRESS_IP *)(remote_addr->Address))->sin_port);
			if(53 == port || 67 == port)
			{
				KdPrint(("%s:(UDP)opetion %d skip %d port.\r\n", pProcName, pStack->MinorFunction, port));
			}
			else if(FALSE == IsSkipDisnetwork(pProcName))
			{
				requdp->SendLength = 0L;	// ��53�˿ں�ָ�����򣬾Ͱѷ��ͳ�����0
			}
		}
	}
	else if(DT_FILTER_TCP == pDevExt->DeviceType)
	{
		switch(pStack->MinorFunction)
		{
		case TDI_CONNECT:
			status = STATUS_REMOTE_NOT_LISTENING;
			//status = STATUS_REMOTE_NOT_LISTENING; ���Ի��᲻�Ϸ���115.182.53.89:80���ַ
#ifdef DBG
			{
				PTDI_REQUEST_KERNEL			req;
				PTRANSPORT_ADDRESS			remote_addr;
				ULONG						IPAdd;
				USHORT						port;

				req = (TDI_REQUEST_KERNEL *)&pStack->Parameters;
				remote_addr = (PTRANSPORT_ADDRESS)(((TRANSPORT_ADDRESS *)req->RequestConnectionInformation->RemoteAddress))->Address;
				IPAdd = my_ntohl(((TDI_ADDRESS_IP *)(remote_addr->Address))->in_addr);
				port = my_ntohs(((TDI_ADDRESS_IP *)(remote_addr->Address))->sin_port);

				KdPrint(("%s: Connect %d.%d.%d.%d:%d \n"
					, pProcName
					, (IPAdd >> 0x18) &0xff
					, (IPAdd >> 0x10) & 0xff
					, (IPAdd >> 0x8) & 0xff
					, IPAdd & 0xff
					, port ));
			}
#endif
			break;
		case TDI_RECEIVE:
		case TDI_SEND:
			status = STATUS_INVALID_CONNECTION;
			break;
		}
		if(!NT_SUCCESS(status) && FALSE == IsSkipDisnetwork(pProcName))
		{
			// �ȴ�5����, ������һЩ����᲻�Ϸ������ӣ����������ϵͳ��Դ�˷�
			if(TDI_CONNECT == pStack->MinorFunction)
			{
				LARGE_INTEGER		timeout;

				timeout = RtlConvertLongToLargeInteger(-5 * 1000 * 10000);
				KeDelayExecutionThread(KernelMode, FALSE, &timeout);
			}
			goto irpstop;
		}
	}
	else
	{
		goto irpstop;
	}
	// ֹͣIRP
	KdPrint(("%s:(%s)opetion %d skip.\r\n", pProcName
		, (DT_FILTER_TCP == pDevExt->DeviceType)?"TCP":"UDP", pStack->MinorFunction));
	return STATUS_SUCCESS;
irpstop:
	KdPrint(("%s:(%s)opetion %d stop.\r\n", pProcName
		, (DT_FILTER_TCP == pDevExt->DeviceType)?"TCP":"UDP", pStack->MinorFunction));
	return status;
}
// �����ַ
NTSTATUS CheckUrl(char* pHttpPacket, int nHttpLen, tdi_foc_connection_ptr pAddress, BOOLEAN* pIsHttp)
{
	int					nFindLabel		= 0;
	char*				pNextLine		= pHttpPacket;
	char*				pCRLF			= NULL;
	NTSTATUS			status;
	LARGE_INTEGER		timeout;
	int					i				= 0;

// 	KdPrint(("[CheckUrl] find connect: %s\n", pHttpPacket));
	*pIsHttp = FALSE;
	if(_strnicmp(pHttpPacket, "GET", 3) != 0)
	{
		if(_strnicmp(pHttpPacket, "POST", 4) == 0
			|| _strnicmp(pHttpPacket, "CONNECT", 7) == 0)
		{
			*pIsHttp = TRUE;
		}
		return STATUS_SUCCESS;
	}
	*pIsHttp = TRUE;
	HostInfo.bHasInline = 0;
	RtlZeroMemory(HostInfo.szUrl, sizeof(HostInfo.szUrl));
	RtlZeroMemory(HostInfo.szUrlPath, sizeof(HostInfo.szUrlPath));
	for(i = 4; i < nHttpLen && i < ((int)sizeof(HostInfo.szUrlPath)-1); i++)
	{
		if('\x20' == pHttpPacket[i] || '\r' == pHttpPacket[i]
			|| '\n' == pHttpPacket[i])
		{
			break;
		}
		HostInfo.szUrlPath[i - 4] = pHttpPacket[i];
	}
	// �����Ƿ���Ƕ��ҳ
	while(NULL != pNextLine && (pNextLine - pHttpPacket) < nHttpLen)
	{
		if(_strnicmp(pNextLine, "Referer:", 8) == 0)
		{
			HostInfo.bHasInline = 1;
			nFindLabel++;
		}
		else if(_strnicmp(pNextLine, "Host:", 5) == 0)
		{
			pNextLine += 5;
			while('\x20' == *pNextLine)
				pNextLine++;
			pCRLF = strchr(pNextLine, '\r');
			if(NULL == pCRLF)
				pCRLF = strchr(pNextLine, '\n');
			if(NULL == pCRLF)
				pCRLF = pNextLine + strlen(pNextLine);
			strncpy(HostInfo.szUrl, pNextLine, min((int)sizeof(HostInfo.szUrl), (int)(pCRLF - pNextLine)));
			nFindLabel++;
		}
		if(2 == nFindLabel)
			break;
		// ת����һ��
		pCRLF = strchr(pNextLine, '\r');
		if(NULL == pCRLF)
			pCRLF = strchr(pNextLine, '\n');
		else
			pCRLF++;
		if(NULL != pCRLF && '\n' == *pCRLF)
			pCRLF++;
		pNextLine = pCRLF;
	}
// 	KdPrint(("[CheckUrl] real connect: %s\n", pHttpPacket));
	if(0 == HostInfo.szUrl[0])
		return STATUS_SUCCESS;	// û���ҵ���ַ

	HostInfo.PID = (ULONGLONG)PsGetCurrentProcessId();
	if(NULL != pAddress)
		HostInfo.nPort = pAddress->Port;
	else
		HostInfo.nPort = 0;
	// ��������ʹ����ͬ��URL�Ͳ���ȷ��
	if(strcmp(HostInfo.szUrl, storageurl.szUrl) == 0 
		&& HostInfo.PID == storageurl.PID
		&& HostInfo.bHasInline == storageurl.bHasInline)
	{
		goto jmp1;
	}
	// �洢�ϴ���Ϣ
	strcpy(storageurl.szUrl, HostInfo.szUrl);
	storageurl.PID = HostInfo.PID;
	storageurl.bHasInline = HostInfo.bHasInline;
	storageurl.nPort = HostInfo.nPort;

	KeClearEvent(UrlAllowOrNotEvent);
	KeSetEvent(EhomeUsrEvent, 0, TRUE);
	// �ȴ��û�ȷ��
	timeout.QuadPart = -10 * 1000 * 2000;
	status = KeWaitForSingleObject(UrlAllowOrNotEvent, Executive, KernelMode, FALSE, &timeout);
	if(STATUS_TIMEOUT == status)
	{
		// DbgPrint("timeout status:%x",KeWaitForSingleObject(UrlAllowOrNotEvent,Executive,KernelMode,FALSE,&ttimeout));
		KeSetEvent(UrlAllowOrNotEvent, 0, FALSE);
	}
	// ��������
jmp1:					
	if(!IsHttpAllow)
	{
		return STATUS_INVALID_CONNECTION;
	}
	// ����ʱ��Ҫ��¼��HOST��Ϣ
	if(NULL != gEHomeKeyword.noticeevent && 0 != gEHomeFilterRule.rule)
	{
		pAddress->pHost = ExAllocatePoolWithTag(NonPagedPool, strlen(HostInfo.szUrl)+1, 'ehom');
		if(NULL != pAddress->pHost)
		{
			RtlZeroMemory(pAddress->pHost, strlen(HostInfo.szUrl)+1);
			strcpy(pAddress->pHost, HostInfo.szUrl);
			pAddress->pid = (ULONGLONG)PsGetCurrentProcessId();
			pAddress->bInline = (0 != HostInfo.bHasInline)?TRUE:FALSE;
			KdPrint(("!!! EhomeNet.sys get connection url (%X)%s\n", pAddress->pHost, pAddress->pHost));
		}
	}

	return STATUS_SUCCESS;
}
// Ĭ����ǲ����
NTSTATUS Ehomedisp(PDEVICE_OBJECT pDevObj,PIRP irp)
{
	PDEVICE_EXTENTION		DevExt;
	PIO_STACK_LOCATION		stack		= IoGetCurrentIrpStackLocation(irp);

	InterlockedIncrement(&gRefCount);
	if(pDevObj == EhomeCtlDev)
	{
		// �����豸����ǲ����
		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		InterlockedDecrement(&gRefCount);
		return STATUS_SUCCESS;
	}
	// �����豸����IRP����
#ifdef NO_UNLOAD
	IoSkipCurrentIrpStackLocation(irp);
#else
	IoCopyCurrentIrpStackLocationToNext(irp);
	IoSetCompletionRoutine(irp, DispatchRoutineComplate, NULL, TRUE, TRUE, TRUE);
#endif
	DevExt = (PDEVICE_EXTENTION)pDevObj->DeviceExtension;
	if(DT_FILTER_TCP == DevExt->DeviceType && IRP_MJ_CREATE == stack->MajorFunction)
		EHomeTcpOpen(irp, stack);
	return IoCallDriver(DevExt->LowTcpDev, irp);
}
// IRP_MJ_DEVICE_CONTROL��ǲ����
NTSTATUS EhomeDevCtl(PDEVICE_OBJECT pDevObj,PIRP irp)
{
	PIO_STACK_LOCATION		stack;
	NTSTATUS				status				= STATUS_SUCCESS;
	PVOID					buf;
	ULONG					uIoControlCode;
	ULONG					uInSize;
	ULONG					uOutSize;
	ULONG					OnOrNot;
	PDEVICE_EXTENTION		DevExt;
	
	InterlockedIncrement(&gRefCount);
	if(pDevObj != EhomeCtlDev)
	{
		// �����豸������IPR����
#ifdef NO_UNLOAD
		IoSkipCurrentIrpStackLocation(irp);
#else
		IoCopyCurrentIrpStackLocationToNext(irp);
		IoSetCompletionRoutine(irp, DispatchRoutineComplate, NULL, TRUE, TRUE, TRUE);
#endif
		DevExt = (PDEVICE_EXTENTION)pDevObj->DeviceExtension;
		__try
		{
			status = IoCallDriver(DevExt->LowTcpDev,irp);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			// Windows 7 ����-δ֪����
			irp->IoStatus.Information = 0;
			status = irp->IoStatus.Status = STATUS_TIMEOUT;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
		}
		return status;
	}
	//KdPrint(("Enter Ehome devctl\n"));
	// �����豸����IRP����
	stack = IoGetCurrentIrpStackLocation(irp);
	// �����豸�����д��ʽ
	buf = irp->AssociatedIrp.SystemBuffer; 
	uIoControlCode = stack->Parameters.DeviceIoControl.IoControlCode;
	uInSize = stack->Parameters.DeviceIoControl.InputBufferLength;
	uOutSize = stack->Parameters.DeviceIoControl.OutputBufferLength;
	OnOrNot = 0;
	switch(uIoControlCode)
	{
	case IOCTL_DNS_SETEVENT:
		{	
			//KdPrint(("Enter Dns SetEvent\n"));
			ULONGLONG		EventHandle			= 0;

			if(8 == uInSize)
				EventHandle = *((PULONGLONG)buf);
			else
				EventHandle = (ULONGLONG)*((PULONG)buf);

			status = ObReferenceObjectByHandle((PVOID)EventHandle, SYNCHRONIZE, *ExEventObjectType
				, irp->RequestorMode, (PVOID*)&EhomeUsrEvent, NULL);
			IsHttpFilterOn = TRUE;
			gControlPID = PsGetCurrentProcess();
			if(!NT_SUCCESS(status))
			{
				IsHttpFilterOn = FALSE;
				//KdPrint(("GetEvent failed  %x  EventHandl:%x\n",status,EventHandle));
			}
		}
		break;
	case IOCTL_GET_DNS_INFO:
		{
			//KdPrint(("Http get info\n"));
			uOutSize = min(uOutSize, sizeof(HostInfo));
			memcpy(buf, &HostInfo, uOutSize);
			status = STATUS_SUCCESS;
		}
		break;
	case IOCTL_DNS_ALLOW_OR_NOT:
		{
			//KdPrint(("Http allowornot\n"));
			IsHttpAllow= *((ULONG*)buf);
			KeSetEvent(UrlAllowOrNotEvent, 0, FALSE);
			status = STATUS_SUCCESS;
		}
		break;
	case IOCTL_CONTROL_NETWORK:
		{
			CTRLNETWORK*	pCtrl		= ((CTRLNETWORK *)buf);

			if(uInSize < sizeof(CTRLNETWORK))
			{
				status = STATUS_INVALID_BLOCK_LENGTH;
				break;
			}
			if(pCtrl->code > 0)
			{
				gpCtrlNetWork->code = 1;
			}
			else if(0 == pCtrl->code)
			{
				strncpy(gpCtrlNetWork->szPaseProc, pCtrl->szPaseProc, 1020);
				_strlwr(gpCtrlNetWork->szPaseProc);
				gpCtrlNetWork->code = 0;
			}
			if(uOutSize >= 4)
			{
				*((LONG *)buf) = (0 != gpCtrlNetWork->code);
				uOutSize = 4;
			}
			else
			{
				uOutSize = 0;
			}
			status = STATUS_SUCCESS;
		}
		break;
	case IOCTL_CONTROL_CLEARCACHE:
		{
			memset(&storageurl, 0, sizeof(storageurl));
			status = STATUS_SUCCESS;
			uOutSize = 0;
		}
		break;
	case IOCTL_CONTROL_FILTER_RULE:
		{
			if(uInSize >= 4)
			{
				gEHomeFilterRule.rule = *((int *)buf);
			}
		}
		break;
	case IOCTL_CONTROL_FILTER_ADDKEYWORD:
		{
			if(uInSize > 0)
			{
				keyword_Add((char *)buf, uInSize);
			}
		}
		break;
	case IOCTL_CONTROL_FILTER_CLEARKEYWORD:
		{
			keyword_Clear();
		}
		break;
	case IOCTL_CONTROL_FILTER_SETEVENT:
		{
			ULONGLONG		EventHandle			= 0;

			if(8 == uInSize)
				EventHandle = *((PULONGLONG)buf);
			else
				EventHandle = (ULONGLONG)*((PULONG)buf);

			status = ObReferenceObjectByHandle((PVOID)EventHandle, SYNCHRONIZE, *ExEventObjectType
				, irp->RequestorMode, (PVOID*)&gEHomeKeyword.noticeevent, NULL);
			if( !NT_SUCCESS(status) )
			{
				gEHomeKeyword.noticeevent = NULL;
			}
		}
		break;
	case IOCTL_CONTROL_FILTER_GET_BLOCK:
		{
			PFILTER_KEYWORD_BLOCK		pfkb		= NULL;
			PLIST_ENTRY					plist		= NULL;
			KIRQL						oldIrql;

			plist = ExInterlockedRemoveHeadList(&gEHomeKeyword.headlist, &gEHomeKeyword.spinlock);
			if(NULL != plist)
			{
				pfkb = CONTAINING_RECORD(plist, FILTER_KEYWORD_BLOCK, list);
				uOutSize = min(uOutSize, sizeof(FILTERKEYWORDBLOCK));
				memcpy(buf, &pfkb->fkl, uOutSize);
				ExFreeToNPagedLookasideList(&gEHomeKeyword.lookaside, pfkb);
			}
			// �ж��Ƿ�������δ������
			KeAcquireSpinLock(&gEHomeKeyword.spinlock, &oldIrql);
			if(FALSE == IsListEmpty(&gEHomeKeyword.headlist))
			{
				KeSetEvent(gEHomeKeyword.noticeevent, 0, TRUE);
			}
			KeReleaseSpinLock(&gEHomeKeyword.spinlock, oldIrql);
		}
		break;
	}

	if(status == STATUS_SUCCESS)
		irp->IoStatus.Information = uOutSize;
	else
		irp->IoStatus.Information = 0;

	irp->IoStatus.Status = status;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	InterlockedDecrement(&gRefCount);
	return status;
}
// IRP_MJ_CLOSE��ǲ����
NTSTATUS EhomeCloseCleanup(PDEVICE_OBJECT pDevObj,PIRP irp)
{
	PEPROCESS				epro;
	PIO_STACK_LOCATION		stack;
	PDEVICE_EXTENTION		DevExt;
	
	InterlockedIncrement(&gRefCount);
	stack = IoGetCurrentIrpStackLocation(irp);
	if(pDevObj != EhomeCtlDev)
	{
		tdi_foc_ptr		pSockConnet		= tdi_foc_GetConnection(stack->FileObject, TRUE);
		// �����豸ֱ������
		tdi_foc_Erase(stack->FileObject);
#ifdef NO_UNLOAD
		IoSkipCurrentIrpStackLocation(irp);
#else
		IoCopyCurrentIrpStackLocationToNext(irp);
		IoSetCompletionRoutine(irp, DispatchRoutineComplate, NULL, TRUE, TRUE, TRUE);
#endif
		DevExt=(PDEVICE_EXTENTION)pDevObj->DeviceExtension;
		return IoCallDriver(DevExt->LowTcpDev,irp);
	}
	//KdPrint(("Enter ctlDev close"));
	IsHttpFilterOn = FALSE;
	IsHttpAllow = TRUE;
	gControlPID = NULL;
	gEHomeFilterRule.rule = 0; // ֹͣ���
	
	gEHomeKeyword.noticeevent = NULL;
	while(TRUE)
	{
		PFILTER_KEYWORD_BLOCK		pfkb		= NULL;
		PLIST_ENTRY					plist		= NULL;

		plist = ExInterlockedRemoveHeadList(&gEHomeKeyword.headlist, &gEHomeKeyword.spinlock);
		if(NULL == plist)
			break;
		pfkb = CONTAINING_RECORD(plist, FILTER_KEYWORD_BLOCK, list);
		ExFreeToNPagedLookasideList(&gEHomeKeyword.lookaside, pfkb);
	}

	irp->IoStatus.Status= STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp,IO_NO_INCREMENT);
	InterlockedDecrement(&gRefCount);
	return STATUS_SUCCESS;
}
USHORT my_ntohs(USHORT uPort)
{
    USHORT a = (uPort << 8) & 0xFF00;
    USHORT b = (uPort >> 8) & 0x00FF;

    return (a | b);    
} 
ULONG my_ntohl(ULONG ip)
{
	ULONG a=(ip<<24) &0xff000000;
	ULONG b=(ip<<8) &0x00ff0000;
	ULONG c=(ip>>8)&0x0000ff00;
	ULONG d=(ip>>24)&0x000000ff;
	return (a|b|c|d);
}
// TCP 80�˿����ӳɹ��������
NTSTATUS EhomeConnectComRoutine(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  irp,
    IN PVOID  Context
    )
{
	KIRQL					oldIrql;
	PIO_STACK_LOCATION		stack			= IoGetCurrentIrpStackLocation(irp);
	tdi_foc_ptr		pSockConnect	= NULL;
	TA_ADDRESS *			remote_addr;
	USHORT					port;
	ULONG					IPAdd;
	TDI_REQUEST_KERNEL *	req;
//	DbgPrint("Enter CompleteRoutine  Status is:%x\n",irp->IoStatus.Status);
	if(irp->PendingReturned)
		IoMarkIrpPending(irp);

	if(STATUS_SUCCESS != irp->IoStatus.Status)
		return irp->IoStatus.Status;

	req=(TDI_REQUEST_KERNEL *)&stack->Parameters;
	remote_addr = (((TRANSPORT_ADDRESS *)req->RequestConnectionInformation->RemoteAddress))->Address;
	port=my_ntohs(((TDI_ADDRESS_IP *)(remote_addr->Address))->sin_port);
	IPAdd=my_ntohl(((TDI_ADDRESS_IP *)(remote_addr->Address))->in_addr);
	//		DbgPrint("CompleteRoutine fileObj is:%x PORT is:%d \n",stack->FileObject,port);

	// ��ӵ��������
	pSockConnect = tdi_foc_GetConnection(stack->FileObject, FALSE);

	if(pSockConnect)
	{
		pSockConnect->connecation.IPAdd = IPAdd;
		pSockConnect->connecation.Port = port;
		pSockConnect->connecation.bChecked = FALSE;
		KdPrint(("[EhomeConnectComRoutine] find conenct: fileobj=%x\n", stack->FileObject));
	}
		
	InterlockedDecrement(&gRefCount);
	return STATUS_SUCCESS/*STATUS_CONTINUE_COMPLETION*/;
}
BOOL EhomeTDISend(PIRP irp,PIO_STACK_LOCATION stack)
{
	return TRUE;
}

// �Ƿ������Ľ���
BOOL IsSkipDisnetwork(char* pProcName)
{
	CHAR		szProcName[256]		= {"|"};

	strncpy(&szProcName[1], pProcName, sizeof(szProcName)-3);
	strcat(szProcName, "|");
	_strlwr(szProcName);
	return NULL != strstr(gpCtrlNetWork->szPaseProc, szProcName);
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Ĭ��������� 
NTSTATUS DispatchRoutineComplate(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
	PIO_STACK_LOCATION		stack		= IoGetCurrentIrpStackLocation(Irp);
	int						numkey		= 0;

	if(Irp->PendingReturned)
	{
		IoMarkIrpPending(Irp);
	}

	InterlockedDecrement(&gRefCount);
	return Irp->IoStatus.Status;
}
// ��ʱ����
void OnTimer(IN PDEVICE_OBJECT DeviceObject, IN PVOID Context)
{
	if(gRefCount > 0)
		return;
	IoStopTimer(DeviceObject);
	IoDeleteDevice(DeviceObject);
	InterlockedDecrement(&gEhomeClear);
	EhomeClear();
}
// ��������
void EhomeUnload(PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT			pNexObj;

	KdPrint(("Enter EhomeUnload\r\n"));
	pNexObj = pDriverObject->DeviceObject;
	while(NULL != pNexObj)
	{
		PDEVICE_OBJECT			pCurObj		= pNexObj;
		PDEVICE_EXTENTION		pDevExt		= (PDEVICE_EXTENTION)pCurObj->DeviceExtension;

		pNexObj = pCurObj->NextDevice;
		if(DT_EHOME == pDevExt->DeviceType)
		{
			IoDeleteSymbolicLink(&pDevExt->linkName);
			IoDeleteDevice(pCurObj);
			InterlockedDecrement(&gEhomeClear);
			EhomeClear();
		}
		else
		{
			if(NULL != pDevExt->LowTcpDev)
				IoDetachDevice(pDevExt->LowTcpDev);
			if(0 < gRefCount)
			{
				IoInitializeTimer(pCurObj, OnTimer, NULL);
				IoStartTimer(pCurObj);
			}
			else
			{
				IoDeleteDevice(pCurObj);
				InterlockedDecrement(&gEhomeClear);
				EhomeClear();
			}
		}
	}
}
// ����ڴ�
void EhomeClear()
{
	if(gEhomeClear > 0)
		return;

	if(NULL != UrlAllowOrNotEvent)
		ExFreePool(UrlAllowOrNotEvent);
	if(NULL != UrlNameMutex)
		ExFreePool(UrlNameMutex);
	if(NULL != gpCtrlNetWork)
		ExFreePool(gpCtrlNetWork);

	UrlAllowOrNotEvent = NULL;
	UrlNameMutex = NULL;
	gpCtrlNetWork = NULL;
}

/* ���˵�����HTTP������ */
void		HttpRequestEraseFlag(char* pHttpRequest, int nHttpLen)
{
	char*				pNextLine		= pHttpRequest;
	int					i				= 0;
	char*				pGzip			= NULL;

	// �����Ƿ��д����־
	while(NULL != pNextLine)
	{
		if(_strnicmp(pNextLine, "Accept-Encoding:", 16) == 0)
		{
			for(i = (int)(pNextLine - pHttpRequest) + 16
				; i < nHttpLen && '\x0' != pHttpRequest[i] && '\r' != pHttpRequest[i] && '\n' != pHttpRequest[i]
				; i++)
			{
				pHttpRequest[i] = '\x20';
			}
		}
		else if(_strnicmp(pNextLine, "Cookie:", 7) == 0)
		{
			// �ȸ����վ��������ֵ��������ҳ - �ض����Թȸ���վ
			for(i = (int)(pNextLine - pHttpRequest) + 7
				; i < (nHttpLen - 6) && '\x0' != pHttpRequest[i] && '\r' != pHttpRequest[i] && '\n' != pHttpRequest[i]
				; i++)
			{
				if(_strnicmp(pHttpRequest + i, "GZ=Z=1", 6) == 0)
				{
					int			k		= 0;

					for(k; k < 6; k++)
						pHttpRequest[i + k] = ';';
					break;
				}
			}
		}
		// ������һ��
		for(i = (int)(pNextLine - pHttpRequest); i < (nHttpLen - 1); i++)
		{
			if('\r' == pHttpRequest[i])
			{
				if('\n' == pHttpRequest[i+1])
				{
					pNextLine = pHttpRequest + (i + 2);
					break;
				}
			} 
			else if('\n' == pHttpRequest[i])
			{
				pNextLine = pHttpRequest + (i + 1);
				break;
			}
		}
		if(i >= (nHttpLen - 1))
			break; // �Ҳ�����һ��
	}
}

/*
 *	��TCP����ʱ, ��¼�������Ϣ
 */
void EHomeTcpOpen(PIRP pIrp, PIO_STACK_LOCATION pIrps)
{
	PFILE_FULL_EA_INFORMATION		pFileEa			= NULL;
	PFILE_OBJECT					pFileObj		= pIrps->FileObject;
	tdi_foc_ptr						pConnect		= NULL;

	pFileEa = (PFILE_FULL_EA_INFORMATION)pIrp->AssociatedIrp.SystemBuffer;
	if(NULL == pFileEa)
		return;
	if( TDI_CONNECTION_CONTEXT_LENGTH != pFileEa->EaNameLength )
		return;
	if( 0 != memcmp(TdiConnectionContext, pFileEa->EaName, TDI_CONNECTION_CONTEXT_LENGTH) )
		return;
	pConnect = tdi_foc_GetConnection(pFileObj, TRUE);
	if(NULL == pConnect)
		return;
	pConnect->connecation.pConnectContext = *( (CONNECTION_CONTEXT *)(pFileEa->EaName + (pFileEa->EaNameLength + 1)) );
}
