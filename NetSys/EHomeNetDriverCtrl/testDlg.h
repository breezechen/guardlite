// testDlg.h : ͷ�ļ�
//

#pragma once
#include "afxcmn.h"
#include <map>
#include <string>


// CtestDlg �Ի���
class CtestDlg : public CDialog
{
// ����
public:
	CtestDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_TEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON		m_hIcon;
	HANDLE		m_hSysFile;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
public:
	afx_msg void OnBnClickedBtStart();
public:
	afx_msg void OnBnClickedBtStop();
	BOOL	StartDriver();
public:
	CListCtrl m_list;
public:
	afx_msg void OnBnClickedBtClear();

	std::map<DWORD, BOOL>			m_mapMonProc;
	std::map<std::string, BOOL>		m_mapUrl;

	void		DoSysUrlMon();
	BOOL		CheckURL(ULONG pid, CHAR* pUrl, BOOL bInline, int nPort);
	BOOL		FindKeywordBlock(ULONG pid, ULONG port, LONG rule, char* pHost);
	static DWORD WINAPI SysThread(LPVOID lpParameter);
public:
	afx_msg void OnBnClickedBtNetwork();
public:
	int m_nRule;
public:
	CString m_strKeyword;
public:
	afx_msg void OnBnClickedButton2();
public:
	afx_msg void OnBnClickedButton3();
};
