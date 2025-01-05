// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"

// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG1, pParent)
{
}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0, 45, NULL);
	return TRUE; // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0) //定时器
	{
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); //获取父窗口指针
		if (pParent->isFull())
		{
			CRect rect;
			// pParent->getImage().BitBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0,SRCCOPY); //将图片显示到控件上
			m_picture.GetWindowRect(rect); //获取控件大小
			pParent->getImage().StretchBlt(
				m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(),SRCCOPY); //拉伸图片
			m_picture.InvalidateRect(NULL); //刷新控件
			pParent->getImage().Destroy(); //销毁图片
			pParent->SetImageStatus(); //设置图片状态
		}
	}
	CDialog::OnTimer(nIDEvent);
}
