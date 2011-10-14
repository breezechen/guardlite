#pragma once

#ifdef _WINDLL
#define DLLAPI		extern "C" _declspec(dllexport)
#else
#define DLLAPI		extern "C" _declspec(dllimport)
#endif

/*
 *	��ȡ��������Ϣ
 */
DLLAPI int WndHookGetLastError();

/*
 *	��ʼ�����ڹ���
 */
DLLAPI BOOL WndHookInit();

/*
 *	ע�����ڹ���
 */
DLLAPI BOOL WndHookDestroy();

#define ERROR_WNDHOOK_SUCCESS				0
#define ERROR_WNDHOOK_ALREADINIT			-1
#define ERROR_WNDHOOK_NOTINIT				-2