/*
 *	TDI��TCP���Ӳ�����HASH����
 */

#include "EhomeNet.h"
#include "TdiSocketContext.h"

#define HASH_SIZE				0x1000
#define CALC_HASH(fileobj)		(((ULONG)(fileobj) >> 5) % HASH_SIZE)

#pragma data_seg()
PLIST_ENTRY						gTdiContextHashTable		= NULL;

#pragma data_seg()
NPAGED_LOOKASIDE_LIST			gTdiContextHashLookaside	= {0};

#pragma data_seg()
KSPIN_LOCK						gTdiContextHashSpinLock		= {0};

/*
 *	��ʼ��TABLE
 */
BOOLEAN TdiSocketContextInit()
{
	int			i;

	ExInitializeNPagedLookasideList(&gTdiContextHashLookaside, NULL, NULL
		, 0, sizeof(TDI_SOCKET_CONTEXT), 'ehom', 0);
	gTdiContextHashTable = (PLIST_ENTRY)ExAllocatePoolWithTag(NonPagedPool, sizeof(LIST_ENTRY) * HASH_SIZE, 'ehom');
	if(NULL == gTdiContextHashTable)
	{
		KdPrint(("[TdiSocketContextInit] ExAllocatePool failed.\n"));
		return FALSE;
	}
	// ��ʼ��������
	KeInitializeSpinLock(&gTdiContextHashSpinLock);
	// ��ʼ��ÿ������
	for(i = 0; i < HASH_SIZE; i++)
	{
		InitializeListHead(&gTdiContextHashTable[i]);
	}

	return TRUE;
}

/*
 *	��ȡ�ṹ��
 */
PTDI_SOCKET_CONTEXT TdiSocketContextGet(PFILE_OBJECT pAddressFileObj)
{
	int						nIndex		= CALC_HASH(pAddressFileObj);
	PTDI_SOCKET_CONTEXT		pRet		= NULL;
	PTDI_SOCKET_CONTEXT		pTemp		= NULL;
	KIRQL					irql;
	PLIST_ENTRY				pList		= NULL;

	if(NULL == gTdiContextHashTable || NULL == pAddressFileObj)
		return NULL;
	KeAcquireSpinLock(&gTdiContextHashSpinLock, &irql);
	// �����е�TABLE�����
	for(pList = gTdiContextHashTable[nIndex].Blink
		; pList != &gTdiContextHashTable[nIndex]
		; pList = pList->Blink)
	{
		pTemp = CONTAINING_RECORD(pList, TDI_SOCKET_CONTEXT, list);
		if(pAddressFileObj == pTemp->pAddressFileObj)
		{
			pRet = pTemp;
			break;
		}
	}
	KeReleaseSpinLock(&gTdiContextHashSpinLock, irql);

	return pRet;
}
/*
 *	��ȡ��ַ����
 */
PTDI_SOCKET_CONTEXT TdiSocketContextGetAddress(PFILE_OBJECT pAddressFileObj, BOOLEAN bCreate)
{
	PTDI_SOCKET_CONTEXT			pSocketContext		= TdiSocketContextGet(pAddressFileObj);

	if(NULL == pSocketContext && FALSE != bCreate)
	{
		int						nIndex				= CALC_HASH(pAddressFileObj);

		pSocketContext = (PTDI_SOCKET_CONTEXT)ExAllocateFromNPagedLookasideList(&gTdiContextHashLookaside);
		if(NULL == pSocketContext)
		{
			KdPrint(("[TdiSocketContextGet] ExAllocateFromNPagedLookasideList Failed %x to %d\n", pAddressFileObj, nIndex));
			return NULL;
		}
		RtlZeroMemory(pSocketContext, sizeof(TDI_SOCKET_CONTEXT));
		pSocketContext->pAddressFileObj = pAddressFileObj;
		pSocketContext->bIsAddressFileObj = TRUE;
		ExInterlockedInsertHeadList(&gTdiContextHashTable[nIndex], &pSocketContext->list, &gTdiContextHashSpinLock);
		KdPrint(("[TdiSocketContextGetConnect] insert %x to %d\n", pAddressFileObj, nIndex));
	}
	else if(NULL != pSocketContext)
	{
		if(FALSE == pSocketContext->bIsAddressFileObj)
			return NULL;
	}
	return pSocketContext;
}
/*
 *	��ȡ���Ӷ���
 */
PTDI_SOCKET_CONTEXT TdiSocketContextGetConnect(PFILE_OBJECT pConnectFileObj, BOOLEAN bCreate)
{
	PTDI_SOCKET_CONTEXT			pSocketContext		= TdiSocketContextGet(pConnectFileObj);

	if(NULL == pSocketContext && FALSE != bCreate)
	{
		int						nIndex				= CALC_HASH(pConnectFileObj);

		pSocketContext = (PTDI_SOCKET_CONTEXT)ExAllocateFromNPagedLookasideList(&gTdiContextHashLookaside);
		if(NULL == pSocketContext)
		{
			KdPrint(("[TdiSocketContextGet] ExAllocateFromNPagedLookasideList Failed %x to %d\n", pConnectFileObj, nIndex));
			return NULL;
		}
		RtlZeroMemory(pSocketContext, sizeof(TDI_SOCKET_CONTEXT));
		pSocketContext->pAddressFileObj = pConnectFileObj;
		ExInterlockedInsertHeadList(&gTdiContextHashTable[nIndex], &pSocketContext->list, &gTdiContextHashSpinLock);
		KdPrint(("[TdiSocketContextGetConnect] insert %x to %d\n", pConnectFileObj, nIndex));
	}
	else if(NULL != pSocketContext)
	{
		if(FALSE != pSocketContext->bIsAddressFileObj)
			return NULL;
	}
	return pSocketContext;
}
/*
 *	ɾ���ṹ��
 */
void				TdiSocketContextErase(PFILE_OBJECT pAddressFileObj)
{
	int						nIndex		= CALC_HASH(pAddressFileObj);
	PTDI_SOCKET_CONTEXT		pRet		= NULL;
	PTDI_SOCKET_CONTEXT		pTemp		= NULL;
	KIRQL					irql;
	PLIST_ENTRY				pList		= NULL;

	if(NULL == gTdiContextHashTable)
		return;
	KeAcquireSpinLock(&gTdiContextHashSpinLock, &irql);
	// �����е�TABLE�����
	for(pList = gTdiContextHashTable[nIndex].Blink
		; pList != &gTdiContextHashTable[nIndex]
		; pList = pList->Blink)
	{
		pTemp = CONTAINING_RECORD(pList, TDI_SOCKET_CONTEXT, list);
		if(pAddressFileObj == pTemp->pAddressFileObj)
		{
			pList->Flink->Blink = pList->Blink;
			pList->Blink->Flink = pList->Flink;
			ExFreeToNPagedLookasideList(&gTdiContextHashLookaside, pTemp);
			KdPrint(("[TdiSocketContextErase] delete %x from %d\n", pAddressFileObj, nIndex));
			break;
		}
	}
	
	KeReleaseSpinLock(&gTdiContextHashSpinLock, irql);
}

/*
 *	�ͷŽṹ��
 */
void		TdiSocketContextRelease()
{
	int						i;
	PTDI_SOCKET_CONTEXT		pTemp		= NULL;
	KIRQL					irql;

	if(NULL == gTdiContextHashTable)
		return;
	KeAcquireSpinLock(&gTdiContextHashSpinLock, &irql);
	// �����е�TABLE�����
	for(i = 0; i < HASH_SIZE; i++)
	{
		while(FALSE == IsListEmpty(&gTdiContextHashTable[i]))
		{
			pTemp = CONTAINING_RECORD(RemoveHeadList(&gTdiContextHashTable[i]), TDI_SOCKET_CONTEXT, list);
			ExFreeToNPagedLookasideList(&gTdiContextHashLookaside, pTemp);
		}
	}
	KeReleaseSpinLock(&gTdiContextHashSpinLock, irql);
}

/* �Ƿ�HTTP���� */
BOOLEAN IsTcpRequest(PFILE_OBJECT fileObj, PASSOCIATE_ADDRESS* pAddress, PTDI_SOCKET_CONTEXT* ppSockConnect)
{
	*pAddress = NULL;
	*ppSockConnect = TdiSocketContextGetConnect(fileObj, FALSE);
	if(NULL != *ppSockConnect)
	{
		*pAddress = &( (*ppSockConnect)->address );
	}
	return NULL != *pAddress;
}
