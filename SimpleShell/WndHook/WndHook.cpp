// WndHook.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "WndHook.h"
#include <stdio.h>
#include <tchar.h>

// ������������
#pragma data_seg(".wndhook")
#pragma data_seg()
#pragma comment(linker, "/SECTION:.wndhook,rws")

// ȫ�ֱ�������
HINSTANCE	g_hInst					= NULL;
HHOOK		g_hHookWnd				= NULL;

// ���ú�������
LRESULT CALLBACK WndHookCBTProc(int nCode, WPARAM wParam, LPARAM lParam);


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	if(DLL_PROCESS_DETACH == ul_reason_for_call)
	{
		WndHookDestroy();
	}
	else if(DLL_PROCESS_ATTACH == ul_reason_for_call)
	{
		g_hInst = hModule;
	}
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////

// ��ȡ���Ĵ�����Ϣ
DLLAPI int WndHookGetLastError()
{
	return GetLastError();
}

// ��ʼ�����ڹ���
DLLAPI BOOL WndHookInit()
{
	SetLastError(ERROR_WNDHOOK_SUCCESS);
	// �ж��Ƿ��Ѱ�װ����
	if(NULL != g_hHookWnd)
	{
		SetLastError(ERROR_WNDHOOK_ALREADINIT);
		return FALSE;
	}
	// ��װ����
	g_hHookWnd = SetWindowsHookEx(WH_CBT, WndHookCBTProc, g_hInst, 0);
	return NULL != g_hHookWnd;
}

// ע�����ڹ���
DLLAPI BOOL WndHookDestroy()
{
	SetLastError(ERROR_WNDHOOK_SUCCESS);

	if(NULL == g_hHookWnd)
	{
		SetLastError(ERROR_WNDHOOK_NOTINIT);
		return FALSE;
	}

	if(UnhookWindowsHookEx(g_hHookWnd))
	{
		g_hHookWnd = NULL;
		return TRUE;
	}
	return FALSE;
}

// ������Ϣ�ص�����
LRESULT CALLBACK WndHookCBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if(HCBT_CREATEWND == nCode)
	{
		// �������ڵ�ʱ��
		HWND				hWnd		= (HWND)wParam;
		HWND				hParent		= NULL;
		CBT_CREATEWND*		pcbt		= (CBT_CREATEWND *)lParam;

		hParent = GetParent(hWnd);
		if(NULL == hParent || GetDesktopWindow() == hParent)
		{
			if(NULL != pcbt->lpcs && (pcbt->lpcs->style & WS_MAXIMIZEBOX))
			{
				PostMessage(hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
			}
		}
	}
	else if(HCBT_MOVESIZE == nCode/* || HCBT_MINMAX == nCode*/)
	{
		// ������С
		HWND				hWnd			= (HWND)wParam;
		HWND				hParent			= NULL;
		RECT*				pRect			= (RECT*)lParam;

		hParent = GetParent(hWnd);
		if(NULL == hParent || GetDesktopWindow() == hParent)
		{
			// ������ı��С
			TCHAR			szClass[512]	= {0};

			GetClassName(hWnd, szClass, 512);
			if(_tcscmp(_T("#32770"), szClass) != 0)
			{
				return -1;
			}
		}
	}
	else if(HCBT_MINMAX == nCode)
	{
		HWND				hWnd			= (HWND)wParam;
		HWND				hParent			= NULL;

		hParent = GetParent(hWnd);
		if(NULL == hParent || GetDesktopWindow() == hParent)
		{
			// ��������С��
			TCHAR			szClass[512]	= {0};
			DWORD			dwOpt			= LOWORD(lParam);

			GetClassName(hWnd, szClass, 512);
			if(/*SW_RESTORE == dwOpt ||*/ SW_MINIMIZE == dwOpt || SW_SHOWMINIMIZED == dwOpt
				|| SW_SHOWMINNOACTIVE == dwOpt || SW_FORCEMINIMIZE == dwOpt)
			{
				return -1;
			}
		}
	}

	return ::CallNextHookEx(g_hHookWnd, nCode, wParam, lParam);
}