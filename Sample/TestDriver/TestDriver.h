// TestDriver.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CTestDriverApp:
// �йش����ʵ�֣������ TestDriver.cpp
//

class CTestDriverApp : public CWinApp
{
public:
	CTestDriverApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CTestDriverApp theApp;