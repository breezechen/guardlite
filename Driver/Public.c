/*
 *	��������ģ��
 */
#include "GuardLite.h"

// ���IRP����
NTSTATUS		AddIrpToQueue(PIRP pIrp)
{
	NTSTATUS			status		= STATUS_SUCCESS;
	PIRP_LIST			irpList		= NULL;
	PPACK_LIST			packList	= NULL;
	PLIST_ENTRY			pList		= NULL;
	VOID*				pData		= 0;
	LONG				nLen		= 0;
	PIO_STACK_LOCATION	stack;

	// ��֤IRP���ݴ�С
	stack = IoGetCurrentIrpStackLocation(pIrp);
	nLen = stack->Parameters.Read.Length;
	if(nLen < sizeof(GUARDLITEPACK))
	{
		status = STATUS_BUFFER_OVERFLOW;
		return status;
	}
	// ����IRP��ȡ����
	KeWaitForSingleObject(&gIrpPackMutex, Executive, KernelMode, FALSE, NULL);
	if(IsListEmpty(&gPackQueue.list))
	{
		// û��Ҫ���������
		status = STATUS_PENDING;
		irpList = (PIRP_LIST)ExAllocateFromNPagedLookasideList(&gIrpQueue.lookaside);
		if(NULL == irpList)
		{
			status = STATUS_BUFFER_OVERFLOW;
			pIrp->IoStatus.Information = 0;
			pIrp->IoStatus.Status = status;
		}
		else
		{
			IoMarkIrpPending(pIrp);
			irpList->pIrp = pIrp;
			InsertTailList(&gIrpQueue.list, &irpList->list);
		}
	}
	else
	{
		status = STATUS_SUCCESS;
		pList = RemoveHeadList(&gPackQueue.list);
		packList = CONTAINING_RECORD(pList, PACK_LIST, list);
		pData = pIrp->AssociatedIrp.SystemBuffer;
		RtlCopyMemory(pData, &packList->query.Pack, sizeof(GUARDLITEPACK));
		InterlockedIncrement(&gPackWaitID);
		packList->query.ulID = gPackWaitID;
		InsertHeadList(&gPackWaitList, pList);
		// ������������
		DealIrpAndPackQueue();
	}
	KeReleaseMutex(&gIrpPackMutex, FALSE);
	return status;
}
// ���PACK����
NTSTATUS		AddPackToQueue(PGUARDLITEQUERY pQuery)
{
	KeWaitForSingleObject(&gIrpPackMutex, Executive, KernelMode, FALSE, NULL);
	KeReleaseMutex(&gIrpPackMutex, FALSE);
	return STATUS_SUCCESS;
}
// �������
NTSTATUS		DealIrpAndPackQueue()
{
	return STATUS_SUCCESS;
}