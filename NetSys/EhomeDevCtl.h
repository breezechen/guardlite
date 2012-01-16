#pragma once


//////////////////////////////////////////////////////////////////////////
// ��������
/*
 *	�õ��������Ϣ
 *  �������ݣ� (URLINFO) һ��URLINFO�ṹ��
 */
#define IOCTL_GET_DNS_INFO			CTL_CODE(FILE_DEVICE_UNKNOWN, \
			0x0800, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
/*
 *	�������ʻ�ܾ�
 *  ��������: (LONG) 0(��ֹ)��1(����)
 */
#define IOCTL_DNS_ALLOW_OR_NOT		CTL_CODE(FILE_DEVICE_UNKNOWN,\
            0x0801, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
/*
 *	���ü���¼�
 *  ��������: (HANDEL) �ⲿ�¼����
 */
#define IOCTL_DNS_SETEVENT			CTL_CODE(FILE_DEVICE_UNKNOWN,\
            0x0802, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
/*
 *	���������¼�
 *  �������: (CTRLNETWORK)
 *  ��������: (LONG) ���ض���״̬, 0:�������״̬, 1:��������״̬
 */
#define IOCTL_CONTROL_NETWORK		CTL_CODE(FILE_DEVICE_UNKNOWN,\
			0x0803, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
 *	�����������������
 */
#define IOCTL_CONTROL_CLEARCACHE	CTL_CODE(FILE_DEVICE_UNKNOWN,\
		0x0804, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
//////////////////////////////////////////////////////////////////////////
#define NAMELENGTH     128
#pragma pack(push, 1)
// ����һ��URL�����ǵ����ݽṹ, IOCTL_GET_DNS_INFO��������
typedef struct _URLInfo{
	ULONGLONG	PID;					// ����PID, ����64λ
	ULONG		bHasInline;				// �Ƿ������ӣ�0��ʾ��������,1��ʾ������
	ULONG		nPort;					// �˿ں�
	CHAR		szUrl[NAMELENGTH];		// HOST����ͷ
	CHAR		szUrlPath[1024];		// URL������о�
} URLINFO;

// �����¼��������
typedef struct _CtrlNetwork{
	LONG		code;					// 0: �Ͽ�����, 1: ��������, -1: ��ѯ״̬
	CHAR		szPaseProc[1];			// ������ؽ��������б�, ��ʽ"|proc1.exe|proc2.exe|...\0"��󳤶�1020
} CTRLNETWORK;
#pragma pack(pop)

