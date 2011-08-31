/*
 *	��������ģ��
 */
#include "GuardLite.h"
#include "Public.h"

// ���IRP����
NTSTATUS		AddIrpToQueue(PIRP pIrp)
{
	PIO_STACK_LOCATION		pStack			= NULL;

	pStack = IoGetCurrentIrpStackLocation(pIrp);
	if(pStack->Parameters.DeviceIoControl.OutputBufferLength < (LONG)sizeof(GUARDLITEREQUEST))
	{
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return pIrp->IoStatus.Status;
	}
	// ����IRP_READ_STACK
	if(0 > IrpReadStackPush(pIrp))
	{
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_STACK_OVERFLOW;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return pIrp->IoStatus.Status;
	}
	pIrp->IoStatus.Status = STATUS_PENDING;
	IoMarkIrpPending(pIrp);
	// ��ȡ�������
	DealIrpAndPackQueue();

	return pIrp->IoStatus.Status;
}
// ���PACK����
PINNERPACK_LIST		AddPackToQueue(ULONG ulType, LPCWSTR lpPath, LPCWSTR lpSubPath)
{
	PINNERPACK_LIST			pList	= NULL;

	// �����ڴ�ռ�
	pList = ExAllocateFromNPagedLookasideList(&gPackQueue.lookaside);
	if(NULL == pList)
		return NULL;
	RtlZeroMemory(&pList->innerPack, sizeof(GUARDLITEINNERPACK));
	pList->innerPack.Read = FALSE;
	pList->innerPack.Timeout = FALSE;
	pList->innerPack.Access = FALSE;
	KeInitializeEvent(&pList->innerPack.Event, NotificationEvent, FALSE);
	pList->innerPack.Pack.dwRequestID = InterlockedDecrement(&gPackQueue.ulWaitID);
	pList->innerPack.Pack.dwProcessID = (ULONG)PsGetCurrentProcessId();
	pList->innerPack.Pack.dwMonType = ulType;
	wcsncpy(pList->innerPack.Pack.szPath, lpPath, arrayof(pList->innerPack.Pack.szPath));
	wcsncpy(pList->innerPack.Pack.szSubPath, lpSubPath, arrayof(pList->innerPack.Pack.szSubPath));
	// �������
	KeWaitForSingleObject(&gPackQueue.mutex, Executive, KernelMode, FALSE, NULL);
	InsertTailList(&gPackQueue.list, &pList->list);
	KeReleaseMutex(&gPackQueue.mutex, FALSE);
	// �������
	DealIrpAndPackQueue();

	return pList;
}
// �������
NTSTATUS		DealIrpAndPackQueue()
{
	PINNERPACK_LIST			pPackList		= NULL;
	PLIST_ENTRY				pCurList		= gPackQueue.list.Flink;
	PIRP					pIrp			= NULL;
	
	KeWaitForSingleObject(&gPackQueue.mutex, Executive, KernelMode, FALSE, NULL);
	for(; pCurList != &gPackQueue.list; pCurList = pCurList->Flink)
	{
		pPackList = CONTAINING_RECORD(pCurList, INNERPACK_LIST, list);
		if(FALSE != pPackList->innerPack.Read || FALSE != pPackList->innerPack.Timeout)
			continue;
		pIrp = IrpReadStackPop();
		if(NULL == pIrp)
			break;
		RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, &pPackList->innerPack.Pack, sizeof(GUARDLITEREQUEST));
		pPackList->innerPack.Read = TRUE;
		// ���IRP
		pIrp->IoStatus.Information = (ULONG)sizeof(GUARDLITEREQUEST);
		pIrp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	}
	KeReleaseMutex(&gPackQueue.mutex, FALSE);

	return STATUS_SUCCESS;
}
// Ӧ�����
NTSTATUS	ResponseToQueue(PIRP pIrp)
{
	PIO_STACK_LOCATION				pStack;
	PGUARDLITERERESPONSE			pResponse;

	pStack = IoGetCurrentIrpStackLocation(pIrp);
	if(pStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(GUARDLITERERESPONSE))
	{
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return pIrp->IoStatus.Status;
	}
	pResponse = (PGUARDLITERERESPONSE)pIrp->AssociatedIrp.SystemBuffer;
	
	SetPackForQuery(pResponse->dwReuestID, (BOOLEAN)pResponse->bAllowed);

	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return pIrp->IoStatus.Status;
}
// ɾ��Pack
void		RemovePackToQueue(PINNERPACK_LIST pQuery)
{
	PINNERPACK_LIST			pPackList		= NULL;
	PLIST_ENTRY				pCurList		= gPackQueue.list.Flink;
	PIRP					pIrp			= NULL;

	pQuery->innerPack.Timeout = TRUE;
	KeWaitForSingleObject(&gPackQueue.mutex, Executive, KernelMode, FALSE, NULL);
	for(; pCurList != &gPackQueue.list; pCurList = pCurList->Flink)
	{
		pPackList = CONTAINING_RECORD(pCurList, INNERPACK_LIST, list);
		if(pQuery != pPackList)
			continue;
		// �Ӷ�����ɾ��
		pCurList->Blink->Flink = pCurList->Flink;
		pCurList->Flink->Blink = pCurList->Blink;
		ExFreeToNPagedLookasideList(&gPackQueue.lookaside, pPackList);
		break;
	}
	KeReleaseMutex(&gPackQueue.mutex, FALSE);
}
// �����Ƿ�ͨ������
void		SetPackForQuery(ULONG nWaitID, BOOLEAN Access)
{
	PINNERPACK_LIST			pPackList		= NULL;
	PLIST_ENTRY				pCurList		= gPackQueue.list.Flink;
	PIRP					pIrp			= NULL;

	KeWaitForSingleObject(&gPackQueue.mutex, Executive, KernelMode, FALSE, NULL);
	for(; pCurList != &gPackQueue.list; pCurList = pCurList->Flink)
	{
		pPackList = CONTAINING_RECORD(pCurList, INNERPACK_LIST, list);
		if(nWaitID != pPackList->innerPack.Pack.dwRequestID)
			continue;
		// �����¼�
		pPackList->innerPack.Access = Access;
		KeSetEvent(&pPackList->innerPack.Event, IO_NO_INCREMENT, FALSE);
		break;
	}
	KeReleaseMutex(&gPackQueue.mutex, FALSE);
}
// IRP������ջ
LONG		IrpReadStackPush(PIRP pIrp)
{
	KIRQL			irql;
	LONG			lRet		= -1L;

	KeAcquireSpinLock(&gIrpReadStack.spinkLock, &irql);
	if(gIrpReadStack.lPos < (sizeof(gIrpReadStack.irp) - 1))
	{
		gIrpReadStack.irp[++gIrpReadStack.lPos] = pIrp;
		lRet = gIrpReadStack.lPos;
	}
	KeReleaseSpinLock(&gIrpReadStack.spinkLock, irql);

	return lRet;
}
// IRP���г�ջ
PIRP		IrpReadStackPop()
{
	KIRQL			irql;
	PIRP			pIrp		= NULL;

	KeAcquireSpinLock(&gIrpReadStack.spinkLock, &irql);
	if(gIrpReadStack.lPos > -1)
	{
		pIrp = gIrpReadStack.irp[gIrpReadStack.lPos--];
	}
	KeReleaseSpinLock(&gIrpReadStack.spinkLock, irql);

	return pIrp;
}
// �õ�HASH����·��
ULONG					GetHashUprPath(LPCWSTR lpPath)
{
	ULONG			ul			= 0;
	ULONG			i;
	ULONG			uSize;
	WCHAR			wc;

	uSize = wcslen(lpPath);
	for(i = 0; i < uSize; i++)
	{
		wc = lpPath[i];
		if(wc >= L'a' && wc <= L'z')
			wc = wc - L'a' + L'A';

		ul = (ul << 3) + wc;
		ul = ul >> 29;
		if(ul)
		{
			ul ^= ul;
		}
	}

	return ul;
}