#pragma once
/*
 *	foc �� FileObject Context������ĸ��д
 */

/* ���Ӷ���ʹ�ýṹ�� */
typedef struct
{
	ULONG				IPAdd;
	USHORT				Port;
	BOOLEAN				bChecked;
} tdi_foc_connection, *tdi_foc_connection_ptr;

/* ��ַ����ʹ���ṹ�� */

/* TDI�ṹ�� */
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
	tdi_foc_connection		address;				/* ��ַ��Ϣ */
} tdi_foc, *tdi_foc_ptr;

// ��������
BOOLEAN						tdi_foc_Init();
tdi_foc_ptr			tdi_foc_Get(PFILE_OBJECT pAddressFileObj);
tdi_foc_ptr			tdi_foc_GetAddress(PFILE_OBJECT pAddressFileObj, BOOLEAN bCreate);
tdi_foc_ptr			tdi_foc_GetConnection(PFILE_OBJECT pConnectFileObj, BOOLEAN bCreate);
void						tdi_foc_Erase(PFILE_OBJECT pAddressFileObj);
void						tdi_foc_Release();
// ��չ����
BOOLEAN						tdi_foc_CheckConnection(PFILE_OBJECT fileObj, tdi_foc_connection_ptr* pAddress, tdi_foc_ptr* ppSockConnect);
