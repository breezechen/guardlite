// TestDriverDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "TestDriver.h"
#include "TestDriverDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTestDriverDlg �Ի���




CTestDriverDlg::CTestDriverDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTestDriverDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_timeout = 0;
}

void CTestDriverDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_ST_INFO, m_strInfo);
}

BEGIN_MESSAGE_MAP(CTestDriverDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CTestDriverDlg ��Ϣ�������

BOOL CTestDriverDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetForegroundWindow();
	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	SetTimer(1, 1000, NULL);
	m_timeout = 10;

	CString			strTimeout;

	strTimeout.Format(_T("%d���Ӻ��Զ��رմ���"), m_timeout);
	SetDlgItemText(IDC_ST_TIMEOUT, strTimeout.GetBuffer());

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CTestDriverDlg::OnPaint()
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
HCURSOR CTestDriverDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CTestDriverDlg::OnTimer(UINT_PTR nIDEvent)
{
	if(1 == nIDEvent)
	{
		CString			strTimeout;

		m_timeout--;
		if(m_timeout <= 0)
		{
			EndDialog(IDCANCEL);
		}
		else
		{
			strTimeout.Format(_T("%d���Ӻ��Զ��رմ���"), m_timeout);
			SetDlgItemText(IDC_ST_TIMEOUT, strTimeout.GetBuffer());
		}
	}

	CDialog::OnTimer(nIDEvent);
}
