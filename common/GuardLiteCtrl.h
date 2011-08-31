/*
 *	�豸������: \\??\\GuardLite
 */
#pragma once

#ifndef _NTDDK_
#include <winioctl.h>
#endif

/*
 *	�������Ƽ��
 *
 *  ����LONG, ��
 *  ���LONG����
 */
#define GUARDLITE_CTRL_START			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800 \
											, METHOD_BUFFERED, FILE_ANY_ACCESS)

/*
 *	�رտ��Ƽ��
 *
 *  ����LONG, ��
 *  ���LONG����
 */
#define GUARDLITE_CTRL_STOP				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801 \
											, METHOD_BUFFERED, FILE_ANY_ACCESS)

/*
 *	��ѯ����״̬
 *  
 *  ����: ��
 *  ���: 0��δ�������  1, �رռ��
 */
#define GUARDLITE_CTRL_STATUS			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802 \
											, METHOD_BUFFERED, FILE_ANY_ACCESS)

/*
 *	��ȡ��ص�������
 *
 *  ����: ��
 *  ���: GUARDLITEREQUEST �ṹ
 */
#define GUARDLITE_CTRL_REQUEST			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803 \
											, METHOD_BUFFERED, FILE_ANY_ACCESS)

/*
 *	���ü�ص�������
 *
 *  ����: GUARDLITERERESPONSE �ṹ
 *  ���: ��
 */
#define GUARDLITE_CTRL_RESPONSE			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804 \
											, METHOD_BUFFERED, FILE_ANY_ACCESS)

// ע�����
#define MASK_GUARDLITE_REGMON			0x1
// �ļ����
#define MASK_GUARDLITE_FILEMON			0x2
// ������
#define MASK_GUARDLITE_SERVICESMON		0x4

#pragma pack(push, 1)

// Read�ṹ��
typedef struct _GuardLiteRequest{
	ULONG		dwMonType;			// �������
	ULONG		dwRequestID;		// �˴μ�ص�ID��ʶ
	ULONG		dwProcessID;		// Ŀ�����ID
	WCHAR		szPath[256];		// ���Ŀ¼
	WCHAR		szSubPath[512];		// �����Ŀ¼
}GUARDLITEREQUEST, PGUARDLITEREQUEST;
// Write�ṹ��
typedef struct _GuardLiteResponse{
	ULONG		dwReuestID;			// �����ID
	ULONG		bAllowed;			// �Ƿ�����
}GUARDLITERERESPONSE, *PGUARDLITERERESPONSE;

#pragma pack(pop)

