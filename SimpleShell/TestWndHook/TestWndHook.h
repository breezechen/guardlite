// TestWndHook.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CTestWndHookApp:
// �йش����ʵ�֣������ TestWndHook.cpp
//

class CTestWndHookApp : public CWinApp
{
public:
	CTestWndHookApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CTestWndHookApp theApp;