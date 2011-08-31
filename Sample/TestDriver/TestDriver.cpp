// TestDriver.cpp : ����Ӧ�ó��������Ϊ��
//

#include "stdafx.h"
#include "TestDriver.h"
#include "TestDriverDlg.h"
#include "../../common/GuardLiteCtrl.h"
#include <Psapi.h>

#pragma comment(lib, "Psapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTestDriverApp

BEGIN_MESSAGE_MAP(CTestDriverApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CTestDriverApp ����

CTestDriverApp::CTestDriverApp()
{
	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� CTestDriverApp ����

CTestDriverApp theApp;


// CTestDriverApp ��ʼ��

BOOL CTestDriverApp::InitInstance()
{
	// ���һ�������� Windows XP �ϵ�Ӧ�ó����嵥ָ��Ҫ
	// ʹ�� ComCtl32.dll �汾 6 ����߰汾�����ÿ��ӻ���ʽ��
	//����Ҫ InitCommonControlsEx()�����򣬽��޷��������ڡ�
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// ��������Ϊ��������Ҫ��Ӧ�ó�����ʹ�õ�
	// �����ؼ��ࡣ
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	// ��׼��ʼ��
	// ���δʹ����Щ���ܲ�ϣ����С
	// ���տ�ִ���ļ��Ĵ�С����Ӧ�Ƴ�����
	// ����Ҫ���ض���ʼ������
	// �������ڴ洢���õ�ע�����
	// TODO: Ӧ�ʵ��޸ĸ��ַ�����
	// �����޸�Ϊ��˾����֯��
	SetRegistryKey(_T("Ӧ�ó��������ɵı���Ӧ�ó���"));
	//////////////////////////////////////////////////////////////////////////
	// ������
	HANDLE			hGuardLite			= NULL;
	DWORD			dwRead				= 0;
	TCHAR			szTestType[128]				= {0};

// 	GetGuardType(MAKEGUARDTYPE(1, 1), szTestType);

	hGuardLite = CreateFile(_T("\\\\.\\GuardLite")
		, GENERIC_READ|GENERIC_WRITE
		, 0
		, NULL
		, OPEN_EXISTING
		, FILE_ATTRIBUTE_NORMAL
		, NULL);
	if(NULL == hGuardLite || INVALID_HANDLE_VALUE == hGuardLite)
	{
		CString				str;

		str.Format(_T("�򿪷���ʧ��: %d"), GetLastError());
		MessageBox(NULL, str.GetBuffer(), _T("����"), MB_ICONERROR | MB_OK);
		return FALSE;
	}
	// ��������
	DeviceIoControl(hGuardLite, GUARDLITE_CTRL_START, NULL, 0, NULL, 0, &dwRead, NULL);

	while(true)
	{
		GUARDLITEREQUEST			request		= {0};
		GUARDLITERERESPONSE			response	= {0};

		if(FALSE == DeviceIoControl(hGuardLite, GUARDLITE_CTRL_REQUEST
			, NULL, 0, &request, sizeof(request), &dwRead, NULL))
		{
			CString				str;

			str.Format(_T("��ȡ����ʧ��: %d"), GetLastError());
			MessageBox(NULL, str.GetBuffer(), _T("����"), MB_ICONERROR | MB_OK);
			break;
		}
		response.dwReuestID = request.dwRequestID;
		// ��ʾ��ʾ��
		CTestDriverDlg			dlg;
		HANDLE					hProc;
		TCHAR					szPath[MAX_PATH]		= {0};
		TCHAR					szType[128]				= {0};

		GetGuardType(request.dwGuardType, szType);

		hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, request.dwProcessID);
		GetProcessImageFileName(hProc, szPath, MAX_PATH);
		CloseHandle(hProc);
		//m_pMainWnd = &dlg;

		dlg.m_strInfo.Format(_T("�������: %s\r\n������: %s\r\n���·��: %s\r\n�����Ŀ¼: %s\r\nֵ: %s\r\n")
			, szType
			, szPath
			, request.szPath
			, request.szSubPath
			, request.szValue);

		INT_PTR nResponse = dlg.DoModal();
		//m_pMainWnd = NULL;
		if (nResponse == IDOK)
		{
			response.bAllowed = TRUE;
		}
		
		DeviceIoControl(hGuardLite, GUARDLITE_CTRL_RESPONSE, &response, sizeof(response)
			, NULL, 0, &dwRead, NULL);
	}
	
	CloseHandle(hGuardLite);
	// ���ڶԻ����ѹرգ����Խ����� FALSE �Ա��˳�Ӧ�ó���
	//  ����������Ӧ�ó������Ϣ�á�
	return FALSE;
}

void CTestDriverApp::GetGuardType(ULONG nType, TCHAR* pType)
{
	if(MASK_GUARDLITE_REGMON == (nType & MASK_GUARDLITE_REGMON))
	{
		switch(nType >> 16)
		{
		case MASK_SYSTEM_AUTORUN:
			_tcscpy(pType, _T("ע���-��������"));
			break;
		default:
			_tcscpy(pType, _T("ע���"));
			break;
		}
	}
	else if(MASK_GUARDLITE_FILEMON == (nType & MASK_GUARDLITE_REGMON))
	{
		_tcscpy(pType, _T("�ļ�"));
	}
	else if(MASK_GUARDLITE_SERVICESMON == (nType & MASK_GUARDLITE_REGMON))
	{
		_tcscpy(pType, _T("����"));
	}
	else
	{
		//_tcscpy(pType, _T("δ֪"));
		_ltot(nType & 0xffff, pType, 16);
	}
}