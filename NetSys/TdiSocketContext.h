#pragma once

/* ������Ϣ */
typedef struct _ASSOCIATE_ADDRESS
{
	ULONG				IPAdd;
	USHORT				Port;
	BOOLEAN				bChecked;
}ASSOCIATE_ADDRESS, *PASSOCIATE_ADDRESS;
/* replaced context */
typedef struct {
	LIST_ENTRY				list;
	BOOLEAN					bIsAddressFileObj;		/* �Ƿ񱾵ض˿� */
	PFILE_OBJECT			pAddressFileObj;		/* ���ص�ַ���� */
	PFILE_OBJECT			pConnectFileObj;		/* ���Ӷ˿ڶ��� */
	BOOLEAN					bIsHttp;				/* �Ƿ�HTTPЭ�� */
	PVOID					event_receive_handler;
	PVOID					event_receive_context;
	PVOID					event_chained_handler;
	PVOID					event_chained_context;
	ASSOCIATE_ADDRESS		address;				/* ��ַ��Ϣ */
} TDI_SOCKET_CONTEXT, *PTDI_SOCKET_CONTEXT;


BOOLEAN				TdiSocketContextInit();
PTDI_SOCKET_CONTEXT	TdiSocketContextGet(PFILE_OBJECT pAddressFileObj);
PTDI_SOCKET_CONTEXT TdiSocketContextGetAddress(PFILE_OBJECT pAddressFileObj, BOOLEAN bCreate);
PTDI_SOCKET_CONTEXT	TdiSocketContextGetConnect(PFILE_OBJECT pConnectFileObj, BOOLEAN bCreate);
void				TdiSocketContextErase(PFILE_OBJECT pAddressFileObj);
void				TdiSocketContextRelease();
BOOLEAN				IsTcpRequest(PFILE_OBJECT fileObj, PASSOCIATE_ADDRESS* pAddress, PTDI_SOCKET_CONTEXT* ppSockConnect);
