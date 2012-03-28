#include "EhomeNet.h"
#include "Keyword.h"

extern URLINFO					HostInfo;
extern ULONG					IsHttpAllow;
extern PKEVENT					UrlAllowOrNotEvent;
extern PKEVENT					EhomeUsrEvent;
extern ULONG					IsHttpFilterOn;
extern PEPROCESS				gControlPID;
extern CTRLNETWORK*				gpCtrlNetWork;
extern URLINFO					storageurl;
extern CHAR*					gNetRedirectHead;
extern PKEVENT					gEHomeNewworkEvent;
/*
 *	�õ��������Ϣ
 *  �������ݣ� (URLINFO) һ��URLINFO�ṹ��
 */
NTSTATUS EHomeCtrlGetDnsInfo(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
							 , IN PVOID pInBuff, IN ULONG nInSize
							 , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	*pOutSize = min(*pOutSize, sizeof(HostInfo));
	memcpy(pOutBuff, &HostInfo, *pOutSize);

	return STATUS_SUCCESS;
}
/*
 *	�������ʻ�ܾ�
 *  ��������: (LONG) 0(��ֹ)��1(����)
 */
NTSTATUS EHomeCtrlSetDnsAllowOrNot(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
							 , IN PVOID pInBuff, IN ULONG nInSize
							 , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	IsHttpAllow= *((ULONG*)pInBuff);
	KeSetEvent(UrlAllowOrNotEvent, 0, FALSE);

	return STATUS_SUCCESS;
}
/*
 *	���ü���¼�
 *  ��������: (HANDEL) �ⲿ�¼����
 */
NTSTATUS EHomeCtrlSetDnsEvent(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
								   , IN PVOID pInBuff, IN ULONG nInSize
								   , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	ULONGLONG		EventHandle			= 0;
	NTSTATUS		status;

	if(8 == nInSize)
	{
		EventHandle = *((PULONGLONG)pInBuff);
	}
	else
	{
		EventHandle = (ULONGLONG)*((PULONG)pInBuff);
	}

	status = ObReferenceObjectByHandle((PVOID)EventHandle, SYNCHRONIZE, *ExEventObjectType
		, pIrp->RequestorMode, (PVOID*)&EhomeUsrEvent, NULL);
	IsHttpFilterOn = TRUE;
	gControlPID = PsGetCurrentProcess();
	if(!NT_SUCCESS(status))
	{
		IsHttpFilterOn = FALSE;
	}

	return STATUS_SUCCESS;
}
/*
 *	���������¼�
 *  �������: (CTRLNETWORK)
 *  ��������: (LONG) ���ض���״̬, 0:�������״̬, 1:��������״̬
 */
NTSTATUS EHomeCtrlSetNetwork(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
							  , IN PVOID pInBuff, IN ULONG nInSize
							  , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	CTRLNETWORK*	pCtrl		= ((CTRLNETWORK *)pInBuff);

	if(nInSize < sizeof(CTRLNETWORK))
	{
		return STATUS_INVALID_BLOCK_LENGTH;
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
	if(*pOutSize >= 4)
	{
		*((LONG *)pOutBuff) = (0 != gpCtrlNetWork->code);
		*pOutSize = 4;
	}
	else
	{
		*pOutSize = 0;
	}
	return STATUS_SUCCESS;
}
/*
 *	�����������������
 */
NTSTATUS EHomeCtrlClearCache(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
							 , IN PVOID pInBuff, IN ULONG nInSize
							 , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	memset(&storageurl, 0, sizeof(storageurl));

	return STATUS_SUCCESS;
}
/*
 *	���ùؼ��ֹ��˹���
 *  <0, ���ֹؼ��ֶϿ�����
 *  >0, ���ֹؼ����滻Ϊ*��
 *  0, ֹͣ�ؼ��ֹ���, Ĭ���ǹرյ�
 */
NTSTATUS EHomeCtrlSetFilterRule(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
							 , IN PVOID pInBuff, IN ULONG nInSize
							 , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	if(nInSize >= 4)
	{
		gEHomeFilterRule.rule = *((int *)pInBuff);
	}

	return STATUS_SUCCESS;
}
/*
 *	��ӹؼ��ֹ���
 *  ����Ҫ���˵Ĺؼ���
 */
NTSTATUS EHomeCtrlAddKeyword(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
								, IN PVOID pInBuff, IN ULONG nInSize
								, OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	if(nInSize > 0)
	{
		keyword_Add((char *)pInBuff, nInSize);
	}

	return STATUS_SUCCESS;
}
/*
 *	�������
 */
NTSTATUS EHomeCtrlClearKeyword(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
							 , IN PVOID pInBuff, IN ULONG nInSize
							 , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	keyword_Clear();

	return STATUS_SUCCESS;
}
/*
 *	���ùؼ��ֶ�����ʾ�¼�
 *  ��������: (HANDEL) �ⲿ�¼����
 */
NTSTATUS EHomeCtrlSetFilterEvent(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
							   , IN PVOID pInBuff, IN ULONG nInSize
							   , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	ULONGLONG		EventHandle			= 0;
	NTSTATUS		status;

	if(8 == nInSize)
		EventHandle = *((PULONGLONG)pInBuff);
	else
		EventHandle = (ULONGLONG)*((PULONG)pInBuff);

	status = ObReferenceObjectByHandle((PVOID)EventHandle, SYNCHRONIZE, *ExEventObjectType
		, pIrp->RequestorMode, (PVOID*)&gEHomeKeyword.noticeevent, NULL);
	if( !NT_SUCCESS(status) )
	{
		gEHomeKeyword.noticeevent = NULL;
	}

	return STATUS_SUCCESS;
}
/*
 *	��ȡһ��������ʾ�ؼ���
 *  ��������: Filter_block�ṹ
 */
NTSTATUS EHomeCtrlGetFilterBlock(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
								 , IN PVOID pInBuff, IN ULONG nInSize
								 , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	PFILTER_KEYWORD_BLOCK		pfkb		= NULL;
	PLIST_ENTRY					plist		= NULL;
	KIRQL						oldIrql;

	plist = ExInterlockedRemoveHeadList(&gEHomeKeyword.headlist, &gEHomeKeyword.spinlock);
	if(NULL != plist)
	{
		pfkb = CONTAINING_RECORD(plist, FILTER_KEYWORD_BLOCK, list);
		*pOutSize = min(*pOutSize, sizeof(FILTERKEYWORDBLOCK));
		memcpy(pOutBuff, &pfkb->fkl, *pOutSize);
		ExFreeToNPagedLookasideList(&gEHomeKeyword.lookaside, pfkb);
	}
	else
	{
		*pOutSize = 0;
		return STATUS_SUCCESS;
	}
	// �ж��Ƿ�������δ������
	KeAcquireSpinLock(&gEHomeKeyword.spinlock, &oldIrql);
	if(FALSE == IsListEmpty(&gEHomeKeyword.headlist))
	{
		KeSetEvent(gEHomeKeyword.noticeevent, 0, FALSE);
	}
	KeReleaseSpinLock(&gEHomeKeyword.spinlock, oldIrql);

	return STATUS_SUCCESS;
}
/*
 *	�ض�����ֹ��ҳ��ָ����ҳ��
 *  ��������: ת��ָ��ҳ������ͷ
 */
NTSTATUS EHomeCtrlSetDnsRedirect(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
								 , IN PVOID pInBuff, IN ULONG nInSize
								 , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	RtlCopyMemory(gNetRedirectHead, pInBuff, min(1023, nInSize));
	IsHttpAllow = FALSE;
	KeSetEvent(UrlAllowOrNotEvent, 0, FALSE);

	return STATUS_SUCCESS;
}
/*
 *	���ö���֪ͨ�¼�
 *  ��������: (HANDEL) �ⲿ�¼����
 */
NTSTATUS EHomeCtrlSetNetworkEvent(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrps
								 , IN PVOID pInBuff, IN ULONG nInSize
								 , OUT PVOID pOutBuff, IN OUT ULONG *pOutSize)
{
	ULONGLONG		EventHandle			= 0;

	if(8 == nInSize)
	{
		EventHandle = *((PULONGLONG)pInBuff);
	}
	else
	{
		EventHandle = (ULONGLONG)*((PULONG)pInBuff);
	}

	ObReferenceObjectByHandle((PVOID)EventHandle, SYNCHRONIZE, *ExEventObjectType
		, pIrp->RequestorMode, (PVOID*)&gEHomeNewworkEvent, NULL);

	return STATUS_SUCCESS;
}
//////////////////////////////////////////////////////////////////////////
// ������ƴ���ص�����
EHOME_CONTROL_CALLBACK	gEHomeControlCallback[] = {
	{ IOCTL_GET_DNS_INFO , EHomeCtrlGetDnsInfo }
	, { IOCTL_DNS_ALLOW_OR_NOT, EHomeCtrlSetDnsAllowOrNot }
	, { IOCTL_DNS_SETEVENT, EHomeCtrlSetDnsEvent }
	, { IOCTL_CONTROL_NETWORK, EHomeCtrlSetNetwork }
	, { IOCTL_CONTROL_CLEARCACHE, EHomeCtrlClearCache }
	, { IOCTL_CONTROL_FILTER_RULE, EHomeCtrlSetFilterRule }
	, { IOCTL_CONTROL_FILTER_ADDKEYWORD, EHomeCtrlAddKeyword }
	, { IOCTL_CONTROL_FILTER_CLEARKEYWORD, EHomeCtrlClearKeyword }
	, { IOCTL_CONTROL_FILTER_SETEVENT, EHomeCtrlSetFilterEvent }
	, { IOCTL_CONTROL_FILTER_GET_BLOCK, EHomeCtrlGetFilterBlock }
	, { IOCTL_DNS_REDIRECT, EHomeCtrlSetDnsRedirect }
	, { IOCTL_CONTROL_NETWORK_SETEVENT, EHomeCtrlSetNetworkEvent }
	, { 0, NULL }
};

