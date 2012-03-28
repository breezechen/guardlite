// testDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "test.h"
#include "testDlg.h"
#include <WinIoCtl.h>
// #include "../../EHome/EHomeDriver/EhomeNet1188/EhomeDevCtl.h"		// �������ƶ���ͷ�ļ�
#include "../EhomeDevCtl.h"
#include <Psapi.h>
#include <shlwapi.h>
#include <Winsvc.h>


#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "shlwapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CtestDlg �Ի���




CtestDlg::CtestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CtestDlg::IDD, pParent)
	, m_nRule(0)
	, m_strKeyword(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hSysFile = NULL;
}

void CtestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
	DDX_Radio(pDX, IDC_RADIO1, m_nRule);
	DDX_Text(pDX, IDC_EDIT1, m_strKeyword);
}

BEGIN_MESSAGE_MAP(CtestDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, &CtestDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BT_START, &CtestDlg::OnBnClickedBtStart)
	ON_BN_CLICKED(IDC_BT_STOP, &CtestDlg::OnBnClickedBtStop)
	ON_BN_CLICKED(IDC_BT_CLEAR, &CtestDlg::OnBnClickedBtClear)
	ON_BN_CLICKED(IDC_BT_NETWORK, &CtestDlg::OnBnClickedBtNetwork)
	ON_BN_CLICKED(IDC_BUTTON2, &CtestDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CtestDlg::OnBnClickedButton3)
END_MESSAGE_MAP()


// CtestDlg ��Ϣ�������

BOOL CtestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	GetDlgItem(IDC_BT_STOP)->EnableWindow(FALSE);

	m_list.InsertColumn(0, _T("������"), 0, 100);
	m_list.InsertColumn(1, _T("����URL"), 0, 160);
	m_list.InsertColumn(2, _T("�Ƿ�����"), 0, 50);
	m_list.InsertColumn(3, _T("���"), 0, 50);
	m_list.InsertColumn(4, _T("�˿�"), 0, 50);

	m_mapUrl["baidu.com"] = TRUE;


	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CtestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ��������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù����ʾ��
//
HCURSOR CtestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CtestDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	// OnOK();
}

void CtestDlg::OnBnClickedBtStart()
{
	if(FALSE == StartDriver())
	{
		MessageBox(_T("��������ʧ��"));
		return;
	}

	// TODO: Add your control notification handler code here
	CString			strMsg;

	m_hSysFile = CreateFile(_T("\\\\.\\EHomeNetDev"), GENERIC_READ|GENERIC_WRITE
		, FILE_SHARE_READ
		, NULL
		, OPEN_EXISTING
		, FILE_ATTRIBUTE_NORMAL
		, NULL);

	if(INVALID_HANDLE_VALUE == m_hSysFile)
	{
		strMsg.Format(_T("�򿪼��ʧ��: %d"), GetLastError());
		MessageBox(strMsg);
		return;
	}
	GetDlgItem(IDC_BT_START)->EnableWindow(FALSE);
	GetDlgItem(IDC_BT_STOP)->EnableWindow(TRUE);
	CreateThread(NULL, 0, SysThread, this, 0, NULL);
}

void CtestDlg::OnBnClickedBtStop()
{
	GetDlgItem(IDC_BT_STOP)->EnableWindow(FALSE);
	GetDlgItem(IDC_BT_START)->EnableWindow(TRUE);
	CloseHandle(m_hSysFile);
	m_hSysFile = NULL;
}

void CtestDlg::OnBnClickedBtClear()
{
	m_list.DeleteAllItems();
}

// ����߳�
DWORD CtestDlg::SysThread(LPVOID lpParameter)
{
	CtestDlg*		pThis		= (CtestDlg *)lpParameter;

	pThis->DoSysUrlMon();
	return 0;
}

void CtestDlg::DoSysUrlMon()
{
	HANDLE			hEvent[2]	= {0};
	URLINFO			info		= {0};
	DWORD			dwRead		= 0;
	LONG			lRet		= 0;
	DWORD			dwWait		= 0;

	hEvent[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
	hEvent[1] = CreateEvent(NULL, FALSE, FALSE, NULL);

	DeviceIoControl(m_hSysFile, IOCTL_DNS_SETEVENT, &hEvent[0], sizeof(HANDLE)
		, NULL, 0, &dwRead, NULL);
	DeviceIoControl(m_hSysFile, IOCTL_CONTROL_FILTER_SETEVENT, &hEvent[1], sizeof(HANDLE)
		, NULL, 0, &dwRead, NULL);
	do
	{
		dwWait = WaitForMultipleObjects(2, hEvent, FALSE, 1);
		if(WAIT_TIMEOUT == dwWait)
		{
			if(NULL == m_hSysFile || INVALID_HANDLE_VALUE == m_hSysFile)
				break;
			continue;
		}
		if(WAIT_OBJECT_0 == dwWait)
		{
			DeviceIoControl(m_hSysFile, IOCTL_GET_DNS_INFO, NULL, 0
				, &info, sizeof(info), &dwRead, NULL);
			lRet = CheckURL(info.PID, info.szUrl, info.bHasInline, info.nPort);
			if(FALSE == lRet)
			{
				char*		pHead		= "HTTP/1.1 302 Object moved\r\n"
					"Location: http://www.google.com.hk\r\n"
					"Content-Length: 0\r\n"
					"Content-Type: text/html\r\n\r\n";
				DeviceIoControl(m_hSysFile, IOCTL_DNS_REDIRECT, pHead, (DWORD)strlen(pHead)
					, NULL, 0, &dwRead, NULL);
			}
			else
			{
				DeviceIoControl(m_hSysFile, IOCTL_DNS_ALLOW_OR_NOT, &lRet, sizeof(lRet)
					, NULL, 0, &dwRead, NULL);
			}
		}
		else if((WAIT_OBJECT_0 + 1) == dwWait)
		{
			FILTERKEYWORDBLOCK			fkb		= {0};

			DeviceIoControl(m_hSysFile, IOCTL_CONTROL_FILTER_GET_BLOCK, NULL, 0
				, &fkb, sizeof(fkb), &dwRead, NULL);
			FindKeywordBlock(fkb.pid, fkb.nPort, fkb.rule, fkb.szHost);
		}
	}while(TRUE);

	CloseHandle(hEvent);
}

BOOL CtestDlg::CheckURL(ULONG pid, CHAR* pUrl, BOOL bInline, int nPort)
{
	HANDLE			hProc					= NULL;
	TCHAR			szPath[MAX_PATH]		= {0};
	int				nItem					= 0;
	CHAR			szURL[128]				= {0};
	BOOL			bRet					= TRUE;
	char*			pos						= NULL;
	int				nLen					= 0;
	TCHAR			szPort[32]				= {0};

	if(m_mapMonProc.end() != m_mapMonProc.find(pid))
	{
		return m_mapMonProc[pid];
	}
	// ��ȡ������Ϣ
	hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
	GetProcessImageFileName(hProc, szPath, MAX_PATH);
	CloseHandle(hProc);
	if(StrStrI(szPath, _T("firefox.exe")) ==  NULL)
	{
		m_mapMonProc[pid] = TRUE;
		return TRUE;
	}
	// ��Ӽ�¼
	nItem = m_list.InsertItem(m_list.GetItemCount(), szPath);
	pos = strrchr(pUrl, '.');
	if(NULL != pos)
	{
		strncpy(szURL, pUrl, pos - pUrl);
		pos = strrchr(szURL, '.');
		if(NULL == pos)
			pos = szURL;
		else
			pos++;
		nLen = pos - szURL;
	}
	else
	{
		nLen = 0;
	}
	strcpy(szURL, pUrl+nLen);
	m_list.SetItemText(nItem, 1, CA2T(pUrl));
	if(m_mapUrl.end() != m_mapUrl.find(szURL))
	{
		bRet = FALSE;
		m_list.SetItemText(nItem, 3, _T("��ֹ"));
	}
	else
	{
		m_list.SetItemText(nItem, 3, _T("����"));
	}
	if(bInline)
	{
		m_list.SetItemText(nItem, 2, _T("��"));
	}
	else
	{
		m_list.SetItemText(nItem, 2, _T("��"));
	}
	_itot(nPort, szPort, 10);
	m_list.SetItemText(nItem, 4, szPort);
	return bRet;
}

BOOL CtestDlg::FindKeywordBlock(ULONG pid, ULONG port, LONG rule, char* pHost)
{
	CString			str;

	str.Format(_T("pid: %d, port: %d, host: %S"), pid, port, pHost);
	m_list.InsertItem(m_list.GetItemCount(), str.GetBuffer());
	return TRUE;
}

void CtestDlg::OnBnClickedBtNetwork()
{
	CString		str;
	CString		strMsg;
	DWORD		dwRecv		= 0;

	if(NULL == m_hSysFile || INVALID_HANDLE_VALUE == m_hSysFile)
	{
		m_hSysFile = CreateFile(_T("\\\\.\\EHomeNetDev"), GENERIC_READ|GENERIC_WRITE
			, FILE_SHARE_READ
			, NULL
			, OPEN_EXISTING
			, FILE_ATTRIBUTE_NORMAL
			, NULL);
	}
	if(INVALID_HANDLE_VALUE == m_hSysFile)
	{
		strMsg.Format(_T("�򿪼��ʧ��: %d"), GetLastError());
		MessageBox(strMsg);
		return;
	}

	GetDlgItemText(IDC_BT_NETWORK, str);
	if(str == _T("����"))
	{
		CHAR				szData[1024]	= {0};
		CTRLNETWORK*		network			= (CTRLNETWORK *)szData;

		network->code = 0;
		strcpy(network->szPaseProc, "|firefox.exe||");
		SetDlgItemText(IDC_BT_NETWORK, _T("����"));
		DeviceIoControl(m_hSysFile, IOCTL_CONTROL_NETWORK, network, sizeof(szData), NULL, 0, &dwRecv, NULL);
	}
	else
	{
		CTRLNETWORK			network			= {0};

		network.code = 1;
		DeviceIoControl(m_hSysFile, IOCTL_CONTROL_NETWORK, &network, sizeof(network), NULL, 0, &dwRecv, NULL);
		SetDlgItemText(IDC_BT_NETWORK, _T("����"));
	}
}
// ��������
BOOL CtestDlg::StartDriver()
{
	SC_HANDLE			hSCM					= NULL;
	SC_HANDLE			hService				= NULL;
	BOOL				bRet					= FALSE;
	CHAR				szSysPath[MAX_PATH]		= {""};
	CHAR*				pSysName				= "EHomeNet.sys";
	CHAR*				pSysLink				= "EhomeNet";
	CHAR*				pSplit					= NULL;
	DWORD				Tag						= 8;
	int					i						= 0;

	if(NULL != m_hSysFile && INVALID_HANDLE_VALUE != m_hSysFile)
		return TRUE;

	hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(NULL == hSCM)
	{
		//NLog::FolderFileLog(DBG_DIR, DBG_FILE, "[CUrlProc::StartDriver] OpenSCManager failed: %d"
		//	, GetLastError());
		goto end;
	}

	//if(IsWow64())
	//	pSysName = "EHomeNet64.sys";
	//NFile::GetFileInModulePath(szSysPath, pSysName);
	GetModuleFileNameA(NULL, szSysPath, sizeof(szSysPath)/sizeof(szSysPath[0]));
	pSplit = strrchr(szSysPath, '\\');
	if(NULL != pSplit)
	{
		strcpy(pSplit+1, pSysName);
	}
	hService = CreateServiceA(hSCM, pSysLink, pSysLink, SERVICE_ALL_ACCESS
		, SERVICE_KERNEL_DRIVER, SERVICE_SYSTEM_START, SERVICE_ERROR_NORMAL
		, szSysPath, "PNP_TDI", &Tag, NULL, NULL, NULL);
	if(NULL == hService && ERROR_SERVICE_EXISTS != GetLastError())
	{
// 		NLog::FolderFileLog(DBG_DIR, DBG_FILE, "[CUrlProc::StartDriver] create driver '%s' failed: %d"
// 			, pSysName, GetLastError());
		goto end;
	}
	if(NULL == hService && GetLastError() == ERROR_SERVICE_EXISTS)
	{
		hService = OpenServiceA(hSCM, pSysLink, SERVICE_ALL_ACCESS);
		if(NULL == hService)
		{
// 			NLog::FolderFileLog(DBG_DIR, DBG_FILE, "[CUrlProc::StartDriver] open driver '%s' failed: %d"
// 				, pSysName, GetLastError());
			goto end;
		}
	}
	if(NULL != hService)
	{
		bRet = StartService(hService, 0, NULL); 
		if(FALSE == bRet && ERROR_SERVICE_ALREADY_RUNNING == GetLastError())
			bRet = TRUE;
	}
	// �˳�
end:
	if(FALSE == bRet)
	{
// 		NLog::FolderFileLog(DBG_DIR, DBG_FILE, "[CUrlProc::StartDriver] start driver '%s' failed: %d"
// 			, pSysName, GetLastError());
	}
	if(NULL != hService)
		CloseServiceHandle(hService);
	if(NULL != hSCM)
		CloseServiceHandle(hSCM);
	// ���������
	for(i = 0; /*bRet && */i < 3; i++)
	{
		m_hSysFile = ::CreateFile(_T("\\\\.\\EhomeNetDev"), GENERIC_READ | GENERIC_WRITE
			, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(INVALID_HANDLE_VALUE != m_hSysFile && NULL != m_hSysFile)
			break;

// 		NLog::FolderFileLog(DBG_DIR, DBG_FILE, "[CUrlProc::StartDriver] open '\\\\.\\EhomeNetDev' try NO.%d failed: %d"
// 			, i, GetLastError());
		Sleep(500);
	}

	return NULL != m_hSysFile && INVALID_HANDLE_VALUE != m_hSysFile;
}

void CtestDlg::OnBnClickedButton2()
{
	if(NULL == m_hSysFile || INVALID_HANDLE_VALUE == m_hSysFile)
	{
		MessageBox(_T("δ�򿪼��"));
		return;
	}
	int				nRule			= 0;
	DWORD			dwRecv			= 0;

	UpdateData(TRUE);
	// �رչؼ��ֹ���
	DeviceIoControl(m_hSysFile, IOCTL_CONTROL_FILTER_RULE, &nRule, sizeof(nRule), NULL, 0, &dwRecv, NULL);
	DeviceIoControl(m_hSysFile, IOCTL_CONTROL_FILTER_CLEARKEYWORD, NULL, 0, NULL, 0, &dwRecv, NULL);
	if(0 == m_nRule)
		nRule = 1;
	else
		nRule = -1;
	// ��ӹؼ���
	WCHAR*			pStr		= m_strKeyword.GetBuffer();
	WCHAR*			pCRLF		= 0;

	while(*pStr)
	{
		pCRLF = wcschr(pStr, L'\r');
		if(NULL == pCRLF)
		{
			pCRLF = wcschr(pStr, L'\n');
			if(NULL != pCRLF)
			{
				*pCRLF = 0;
				pCRLF++;
			}
			else
			{
				pCRLF = pStr + wcslen(pStr);
			}
		}
		else
		{
			*pCRLF = 0;
			pCRLF += 2;
		}
		// ��ʼ��ӹ�����
		CHAR		szChar[512]			= {0};

		// UTF8
		memset(szChar, 0, sizeof(szChar));
		WideCharToMultiByte(CP_UTF8, 0, pStr, wcslen(pStr), szChar, sizeof(szChar), NULL, NULL);
		DeviceIoControl(m_hSysFile, IOCTL_CONTROL_FILTER_ADDKEYWORD, szChar, strlen(szChar), NULL, 0, &dwRecv, NULL);
		// GB2312
		memset(szChar, 0, sizeof(szChar));
		WideCharToMultiByte(936, 0, pStr, wcslen(pStr), szChar, sizeof(szChar), NULL, NULL);
		DeviceIoControl(m_hSysFile, IOCTL_CONTROL_FILTER_ADDKEYWORD, szChar, strlen(szChar), NULL, 0, &dwRecv, NULL);
		// ������һ���ؼ���
		pStr = pCRLF;
	}
	// �������
	DeviceIoControl(m_hSysFile, IOCTL_CONTROL_FILTER_RULE, &nRule, sizeof(nRule), NULL, 0, &dwRecv, NULL);
}

void CtestDlg::OnBnClickedButton3()
{
	if(NULL == m_hSysFile || INVALID_HANDLE_VALUE == m_hSysFile)
	{
		MessageBox(_T("δ�򿪼��"));
		return;
	}
	int				nRule			= 0;
	DWORD			dwRecv			= 0;

	// �رչؼ��ֹ���
	DeviceIoControl(m_hSysFile, IOCTL_CONTROL_FILTER_RULE, &nRule, sizeof(nRule), NULL, 0, &dwRecv, NULL);
}
