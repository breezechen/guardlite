/*
 *	TDI ��չ����
 */
#include "EhomeNet.h"
#include "EhomeDevCtl.h"
#include <TdiKrnl.h>
#include "Keyword.h"
#include "TdiFileObjectContext.h"

void		EHomeFilterRecvData(IN tdi_foc_ptr pAddressContext, IN PVOID pData, IN ULONG nLen, OUT BOOLEAN* pbContinue);
BOOLEAN		CheckIsTextHtmlData(IN CHAR* pData, IN ULONG nLen, IN BOOLEAN* pIsHttp);
NTSTATUS	EHomeClientEventReceive(IN PVOID  TdiEventContext, IN CONNECTION_CONTEXT  ConnectionContext
								 , IN ULONG  ReceiveFlags, IN ULONG  BytesIndicated, IN ULONG  BytesAvailable
								 , OUT ULONG  *BytesTaken, IN PVOID  Tsdu, OUT PIRP  *IoRequestPacket);
NTSTATUS	EHomeClientEventChainedReceive(IN PVOID  TdiEventContext, IN CONNECTION_CONTEXT  ConnectionContext
								 , IN ULONG  ReceiveFlags, IN ULONG  ReceiveLength, IN ULONG  StartingOffset
								 , IN PMDL  Tsdu, IN PVOID  TsduDescriptor);

void		EHomeReplaceKeyword(IN PVOID pData, IN ULONG nLen);
NTSTATUS	tdi_close_connect(PFILE_OBJECT pFileObject);

// �����¼����
NTSTATUS		EHomeTDISetEventHandler(PIRP pIrp, PIO_STACK_LOCATION pStack)
{
	PTDI_REQUEST_KERNEL_SET_EVENT			pTdiEvent			= NULL;
	tdi_foc_ptr								pSocketContext		= NULL;

	pTdiEvent = (PTDI_REQUEST_KERNEL_SET_EVENT)&pStack->Parameters;
	pSocketContext = tdi_foc_GetAddress(pStack->FileObject, TRUE);
	if(NULL == pSocketContext || NULL == pTdiEvent)
	{
		KdPrint(("[EHomeTDISetEventHandler] pSocketContext: %d, pTdiEvent: %d\n", pSocketContext, pTdiEvent));
		return STATUS_SUCCESS;
	}

	if(TDI_EVENT_RECEIVE == pTdiEvent->EventType)
	{
		pSocketContext->address.event_receive_handler = pTdiEvent->EventHandler;
		pSocketContext->address.event_receive_context = pTdiEvent->EventContext;
		if(NULL != pTdiEvent->EventHandler)
		{
			pTdiEvent->EventHandler = EHomeClientEventReceive;
			pTdiEvent->EventContext = pStack->FileObject;
		}
		//KdPrint(("[EHomeTDISetEventHandler] TDI_EVENT_RECEIVE pTdiEvent->EventHandler: %d\n", pTdiEvent->EventHandler));
	}
	else if(TDI_EVENT_CHAINED_RECEIVE == pTdiEvent->EventType)
	{
		pSocketContext->address.event_chained_handler = pTdiEvent->EventHandler;
		pSocketContext->address.event_chained_context = pTdiEvent->EventContext;
 		if(NULL != pTdiEvent->EventHandler)
		{
			pTdiEvent->EventHandler = EHomeClientEventChainedReceive;
			pTdiEvent->EventContext = pStack->FileObject;
 		}
		//KdPrint(("[EHomeTDISetEventHandler] TDI_EVENT_CHAINED_RECEIVE pTdiEvent->EventHandler: %d\n", pTdiEvent->EventHandler));
	}

	return STATUS_SUCCESS;
}

// ���տͻ���
NTSTATUS EHomeClientEventReceive(IN PVOID  TdiEventContext, IN CONNECTION_CONTEXT  ConnectionContext
								 , IN ULONG  ReceiveFlags, IN ULONG  BytesIndicated, IN ULONG  BytesAvailable
								 , OUT ULONG  *BytesTaken, IN PVOID  Tsdu, OUT PIRP  *IoRequestPacket)
{
	tdi_foc_ptr								pSocketContext		= NULL;
	NTSTATUS								status				= STATUS_SUCCESS;
	char*									pData				= NULL;
	BOOLEAN									bContinue			= TRUE;

	//KdPrint(("[EHomeClientEventReceive] len:%d, data: %s\n", BytesIndicated, Tsdu));
	pSocketContext = tdi_foc_GetAddress((PFILE_OBJECT)TdiEventContext, FALSE);
	if(NULL == pSocketContext || NULL == pSocketContext->address.event_receive_handler)
	{
		KdPrint(("[EHomeClientEventReceive] pSocketContext: %d\n", pSocketContext));
		return STATUS_SUCCESS;
	}
	// ����ǶϿ��ľ�ִ��ȡ������
	if( FALSE != pSocketContext->bStopOption )
	{
		return STATUS_DATA_NOT_ACCEPTED;
	}
	// �滻�ؼ���
	if( FALSE != pSocketContext->bIsHttp && 0 != gEHomeFilterRule.rule )
	{
		EHomeFilterRecvData( pSocketContext, Tsdu, BytesIndicated, &bContinue );
		if( FALSE == bContinue )
		{
			pSocketContext->bStopOption = TRUE;
			return STATUS_DATA_NOT_ACCEPTED;
		}
	}
	// ����ԭ���Ľ�������
	status = ((PTDI_IND_RECEIVE)pSocketContext->address.event_receive_handler)
		/*TdiDefaultReceiveHandler*/(pSocketContext->address.event_receive_context
		, ConnectionContext
		, ReceiveFlags
		, BytesIndicated
		, BytesAvailable
		, BytesTaken
		, Tsdu
		, IoRequestPacket);
	if( FALSE == pSocketContext->bIsHttp || 0 == gEHomeFilterRule.rule )
		return status;
	if(*BytesTaken == BytesAvailable)
		return status; // ȫ���������
	// ��֤�����Ƿ��д���
	if(STATUS_MORE_PROCESSING_REQUIRED  != status && STATUS_SUCCESS != status)
		return status; // ������ص��� STATUS_DATA_NOT_ACCEPTED ��ʾ����Ĳ�������Ч
	// ������ɶ˿�
	if(NULL != *IoRequestPacket)
	{
		PIO_STACK_LOCATION		irps				= NULL;
		tdi_client_irp_ctx*		new_ctx				= NULL;

		irps = IoGetCurrentIrpStackLocation(*IoRequestPacket);
		if( NULL == irps ) 
		{
			return status;
		}
		// �ı�ص�����
		new_ctx = (tdi_client_irp_ctx *)ExAllocatePoolWithTag(NonPagedPool, sizeof(tdi_client_irp_ctx), 'ehom');
		if(NULL == new_ctx)
		{
			return status; // �����ڴ�ʧ��
		}

		RtlZeroMemory( new_ctx, sizeof(tdi_client_irp_ctx) );
		new_ctx->addrobj = pSocketContext->pAddressFileObj;
		if(NULL != irps->CompletionRoutine)
		{
			new_ctx->completion = irps->CompletionRoutine;
			new_ctx->context = irps->Context;
			new_ctx->old_control = irps->Control;
		}
		// ��������¼�
		irps->CompletionRoutine = tdi_client_irp_complete;
		irps->Context = new_ctx;
		irps->Control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;
	}
	return status;
}

NTSTATUS tdi_client_irp_complete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
	tdi_client_irp_ctx *		ctx					= (tdi_client_irp_ctx *)Context;
	tdi_foc_ptr					pSockContext		= NULL;
	NTSTATUS					status;

	if( 0 != gEHomeFilterRule.rule && NULL != ctx)
	{
		pSockContext = tdi_foc_GetAddress(ctx->addrobj, FALSE);
		if( NULL != pSockContext && FALSE != pSockContext->bStopOption )
		{
			Irp->IoStatus.Information = 0;
			Irp->IoStatus.Status = STATUS_INVALID_CONNECTION;
		}
	}
	
	if ( 0 != gEHomeFilterRule.rule && Irp->IoStatus.Status == STATUS_SUCCESS 
		&& NULL != pSockContext && pSockContext->bIsHttp ) 
	{
		PVOID		pData			= MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
		BOOLEAN		bContinue		= TRUE;	
		
		//KdPrint(("[tdi_client_irp_complete] len:%d, data: %s\n", Irp->IoStatus.Information, pData));
		EHomeFilterRecvData( pSockContext, pData, (ULONG)Irp->IoStatus.Information, &bContinue);
		if(FALSE == bContinue)
		{
			if( NULL != pSockContext )
				pSockContext->bStopOption = TRUE;
			Irp->IoStatus.Information = 0;
			Irp->IoStatus.Status = STATUS_INVALID_CONNECTION;
		}
	}
	if(NULL == ctx)
		return STATUS_SUCCESS;	// û��������ʱ�˳�
	// call original completion
	if (ctx->completion != NULL) 
	{
		// call old completion (see the old control)
		BOOLEAN b_call = FALSE;

		if (Irp->Cancel) 
		{
			// cancel
			if (ctx->old_control & SL_INVOKE_ON_CANCEL)
				b_call = TRUE;
		} 
		else 
		{
			if (Irp->IoStatus.Status >= STATUS_SUCCESS) 
			{
				// success
				if (ctx->old_control & SL_INVOKE_ON_SUCCESS)
					b_call = TRUE;
			} else 
			{
				// error
				if (ctx->old_control & SL_INVOKE_ON_ERROR)
					b_call = TRUE;
			}
		}

		if (b_call) 
		{
			status = (ctx->completion)(DeviceObject, Irp, ctx->context);

			KdPrint(("[tdi_flt] tdi_client_irp_complete: original handler: 0x%x; status: 0x%x\n",
				ctx->completion, status));
		} 
		else
		{
			status = STATUS_SUCCESS;
		}
	}

	ExFreePoolWithTag(ctx, 'ehom');
	return status;
}

NTSTATUS EHomeClientEventChainedReceive(IN PVOID  TdiEventContext, IN CONNECTION_CONTEXT  ConnectionContext
										, IN ULONG  ReceiveFlags, IN ULONG  ReceiveLength, IN ULONG  StartingOffset
										, IN PMDL  Tsdu, IN PVOID  TsduDescriptor)
{
	tdi_foc_ptr								pSocketContext		= NULL;
	NTSTATUS								status				= STATUS_SUCCESS;
	char*									pData				= NULL;
	BOOLEAN									bContinue			= TRUE;

	pSocketContext = tdi_foc_GetAddress((PFILE_OBJECT)TdiEventContext, FALSE);
	if(NULL == pSocketContext || NULL == pSocketContext->address.event_receive_handler)
	{
		KdPrint(("[EHomeClientEventChainedReceive] pSocketContext: %d\n", pSocketContext));
		return STATUS_SUCCESS;
	}
	if(NULL != Tsdu)
	{
		pData = (char *)MmGetSystemAddressForMdlSafe(Tsdu, NormalPagePriority);
		//KdPrint(("[EHomeClientEventChainedReceive] len:%d, data: %s\n", ReceiveLength, (char *)pData + StartingOffset));
	}
	// ����HTTPЭ��ĵ�һ�����ذ�
	if( NULL != pData && !CheckIsTextHtmlData((char *)pData + StartingOffset, ReceiveLength, &pSocketContext->bIsHttp) )
	{
		pSocketContext->bIsHttp = FALSE;
	}
	// �����ȡ��Ϣ�ľ�ֹͣ����
	if( FALSE != pSocketContext->bStopOption )
	{
		return STATUS_DATA_NOT_ACCEPTED;
	}
	else if( FALSE != pSocketContext->bIsHttp && 0 != gEHomeFilterRule.rule )
	{
		EHomeFilterRecvData(pSocketContext, (char *)pData + StartingOffset, ReceiveLength, &bContinue);
		if( FALSE == bContinue )
		{
			pSocketContext->bStopOption = TRUE;
			return STATUS_DATA_NOT_ACCEPTED;
		}
	}
	// ����ԭ���Ľ�������
	status = ((PTDI_IND_CHAINED_RECEIVE)pSocketContext->address.event_chained_handler)
		/*TdiDefaultChainedReceiveHandler*/(
		pSocketContext->address.event_chained_context
		, ConnectionContext
		, ReceiveFlags
		, ReceiveLength
		, StartingOffset
		, Tsdu
		, TsduDescriptor);

	return status;
}

/*�滻�ؼ���*/
void EHomeReplaceKeyword(IN PVOID pData, IN ULONG nLen)
{
	__try
	{
		char*			pKeyWord		= (char *)pData;
		int				nKeywordLen		= 0;
		int				nSurplusLen		= nLen;
		int				i;

		while( keyword_Find(pKeyWord, nSurplusLen, &pKeyWord, &nKeywordLen) )
		{
			for(i = 0; i < nKeywordLen; i++)
				pKeyWord[i] = '*';
			pKeyWord += 4;
			nSurplusLen = nLen - (int)( pKeyWord - (char *)pData);
			if(nSurplusLen < 0)
				break;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		KdPrint(("[EHomeReplaceKeyword] EXCEPTION_EXECUTE_HANDLER\n"));
	}
}

// ��������
NTSTATUS TdiCall (IN PIRP pIrp, IN PDEVICE_OBJECT pDeviceObject, IN OUT PIO_STATUS_BLOCK pIoStatusBlock )
{
	KEVENT kEvent;                                                    // signaling event
	NTSTATUS dStatus = STATUS_INSUFFICIENT_RESOURCES;                 // local status

	KeInitializeEvent ( &kEvent, NotificationEvent, FALSE );          // reset notification event
	pIrp->UserEvent = &kEvent;                                        // pointer to event
	pIrp->UserIosb = pIoStatusBlock;                               // pointer to status block
	dStatus = IoCallDriver ( pDeviceObject, pIrp );                   // call next driver
	if ( dStatus == STATUS_PENDING )                                  // make all request synchronous
	{
		(void)KeWaitForSingleObject (
			(PVOID)&kEvent,                                 // signaling object
			Executive,                                      // wait reason
			KernelMode,                                     // wait mode
			TRUE,                                           // alertable
			NULL );                                         // timeout
	}
	dStatus = pIoStatusBlock->Status;                          
	return ( dStatus );                                               // return with status
}

/*���˹ؼ���*/
void EHomeFilterRecvData(IN tdi_foc_ptr pAddressContext, IN PVOID pData, IN ULONG nLen, OUT BOOLEAN* pbContinue)
{
	*pbContinue = TRUE;
	if(gEHomeFilterRule.rule > 0)
	{
		EHomeReplaceKeyword(pData, nLen);
	}
	else if(gEHomeFilterRule.rule < 0)
	{
		char*					pKeyword			= NULL;
		int						nKeywordLen			= 0;
		PIRP					pIrp				= NULL;
		IO_STATUS_BLOCK			IoStatusBlock		= {0};

		if( FALSE == keyword_Find(pData, nLen, &pKeyword, &nKeywordLen) )
			return;
 		// �Ͽ�����
		*pbContinue = FALSE;
		// ��ӶϿ����Ӽ�¼
		if( NULL != gEHomeKeyword.noticeevent )
		{
			PFILTER_KEYWORD_BLOCK	pfkb				= NULL;
			tdi_foc_ptr				pConnect			= tdi_foc_GetConnection(pAddressContext->pConnectFileObj, FALSE);

			if(NULL == pConnect)
				return;
			pfkb = ExAllocateFromNPagedLookasideList(&gEHomeKeyword.lookaside);
			if(NULL == pfkb)
				return;
			RtlZeroMemory(pfkb, sizeof(FILTER_KEYWORD_BLOCK));
			pfkb->fkl.nPort = pConnect->connecation.Port;
			pfkb->fkl.pid = pConnect->connecation.pid;
			pfkb->fkl.rule = gEHomeFilterRule.rule;
			pfkb->fkl.bInline = pConnect->connecation.bInline;
			if(NULL != pConnect->connecation.pHost)
			{
				strncpy(pfkb->fkl.szHost, pConnect->connecation.pHost
					, min(strlen(pConnect->connecation.pHost), 127));
			}
			ExInterlockedInsertTailList(&gEHomeKeyword.headlist, &pfkb->list, &gEHomeKeyword.spinlock);
			KeSetEvent(gEHomeKeyword.noticeevent, 0, FALSE);
		}
	}
}

/* ������һ�����صİ��� �ǲ���text/htmlЭ�� */
BOOLEAN CheckIsTextHtmlData(IN CHAR* pData, IN ULONG nLen, IN BOOLEAN* pIsHttp)
{
	int					nFindLabel			= 0;
	char*				pNextLine			= pData;
	char*				pCRLF				= NULL;
	char				szContentType[32]	= {0};
	int					i;
	int					nLastLen;

	if( 0 != memcmp(pData, "HTTP/1", 6) )
		return TRUE;

	// �����Ƿ���Content-Type��Ϣ
	while(NULL != pNextLine && (pNextLine - pData) < (int)nLen)
	{
		if(_strnicmp(pNextLine, "Content-Type:", 13) == 0)
		{
			nFindLabel++;
			pNextLine += 13;
			nLastLen = nLen - (ULONG)(pNextLine - pData);
			for(i = 0; i < (int)(sizeof(szContentType) - 1) && i < nLastLen; i++)
			{
				if( !pNextLine[i] || '\r' == pNextLine[i] || '\n' == pNextLine[i] )
					break;
				szContentType[i] = pNextLine[i];
			}
		}
		if(1 == nFindLabel)
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
	
	if(0 == szContentType[0])
		return TRUE;
	_strlwr(szContentType);
	if( NULL == strstr(szContentType, "text/html") )
	{
		KdPrint(("[CheckIsTextHtmlData] Content-Type: %s\n", szContentType));
		return FALSE;
	}
	if(NULL != pIsHttp)
	{
		*pIsHttp = TRUE; // ��ֹ��������
	}
	return TRUE;
}

/* �ر����� */
NTSTATUS tdi_close_connect(PFILE_OBJECT pFileObject)
{
	return STATUS_SUCCESS;
}
