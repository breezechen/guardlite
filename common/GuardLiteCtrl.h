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
 *  ����LONG, ��������flag�����û����Ӧ��FLAG������ؽ�ʧ��
 *  ���LONG��ʵ�ʿ�����flagλ
 */
#define GUARDLITE_CTRL_START			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800 \
											, METHOD_BUFFERED, FILE_ANY_ACCESS)

/*
 *	�رտ��Ƽ��
 *
 *  ����LONG, ��������flag�����û����Ӧ��FLAG�رռ�ؽ������������Ϊ0��ʾֻ��ѯ
 *  ���LONG���رյļ�غ�ʵ�ʿ�����flagλ
 */
#define GUARDLITE_CTRL_STOP				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801 \
											, METHOD_BUFFERED, FILE_ANY_ACCESS)

// ע�����
#define MASK_GUARDLITE_REGMON			0x1
// �ļ����
#define MASK_GUARDLITE_FILEMON			0x2
// ������
#define MASK_GUARDLITE_SERVICESMON		0x4

#pragma pack(push, 1)
typedef struct _GuardLitePack{
	ULONG		dwMonType;
	ULONG		dwProcessID;
	WCHAR		szPath[256];
	WCHAR		szSubPath[512];
}GUARDLITEPACK;
#pragma pack(pop)

