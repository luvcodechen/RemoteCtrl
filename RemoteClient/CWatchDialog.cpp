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
	m_nObjWidth = -1;
	m_nObjHeight = -1;
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
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen)
{
	//800 450
	CRect clientRect;
	if (isScreen)
	{
		// CStatic *pStatic = (CStatic*)GetDlgItem(IDC_WATCH);//获取控件指针
		// if(pStatic)
		// {
		// 	CRect rect;
		// 	pStatic->GetWindowRect(rect);
		// 	ScreenToClient(&point); //转换为相对坐标
		// 	point.y-=rect.top;//减去标题栏高度
		// }
		ScreenToClient(&point); //转换为相对坐标
	}
	// ScreenToClient(&point); //转换为相对坐标
	TRACE("x %d y %d\r\n", point.x, point.y);
	m_picture.GetWindowRect(clientRect); //获取控件的坐标
	TRACE("width %d height %d\r\n", clientRect.Width(), clientRect.Height());
	return CPoint(point.x * m_nObjWidth / clientRect.Width(), point.y * m_nObjHeight / clientRect.Height());
}

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
	if (nIDEvent == 0) //定时器
	{
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); //获取父窗口指针
		if (pParent->isFull())
		{
			CRect rect;
			// pParent->getImage().BitBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0,SRCCOPY); //将图片显示到控件上
			m_picture.GetWindowRect(rect); //获取控件大小
			if (m_nObjWidth == -1)
			{
				m_nObjWidth = pParent->getImage().GetWidth(); //获取图片宽度
			}
			if (m_nObjHeight == -1)
			{
				m_nObjHeight = pParent->getImage().GetHeight(); //获取图片高度
			}
			pParent->getImage().StretchBlt(
				m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(),SRCCOPY); //拉伸图片
			m_picture.InvalidateRect(NULL); //刷新控件
			pParent->getImage().Destroy(); //销毁图片
			pParent->SetImageStatus(); //设置图片状态
		}
	}
	CDialog::OnTimer(nIDEvent);
}


void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1)
	{
		CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
		//封装鼠标事件
		MOUSEEV event;
		event.ptXY = remotePoint; //鼠标坐标
		event.nButton = 0; //左键
		event.nAction = 1; //双击
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
		CDialog::OnLButtonDblClk(nFlags, point);
	}
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1)
	{
		TRACE("x %d y %d\r\n", point.x, point.y);
		CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
		TRACE("x %d y %d\r\n", point.x, point.y);
		//封装鼠标事件
		MOUSEEV event;
		event.ptXY = remotePoint; //鼠标坐标
		event.nButton = 0; //左键
		event.nAction = 2; //按下
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
		TRACE("OnLButtonDown===========================================\r\n");
		CDialog::OnLButtonDown(nFlags, point);
	}
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1)
	{
		CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
		//封装鼠标事件
		MOUSEEV event;
		event.ptXY = remotePoint; //鼠标坐标
		event.nButton = 0; //左键
		event.nAction = 3; //弹起
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);

		CDialog::OnLButtonUp(nFlags, point);
	}
}

void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1)
	{
		CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
		//封装鼠标事件
		MOUSEEV event;
		event.ptXY = remotePoint; //鼠标坐标
		event.nButton = 1; //右键
		event.nAction = 1; //双击
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
		TRACE("OnRButtonDown===========================================\r\n");

		CDialog::OnRButtonDblClk(nFlags, point);
	}
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1)
	{
		CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
		//封装鼠标事件
		MOUSEEV event;
		event.ptXY = remotePoint; //鼠标坐标
		event.nButton = 1; //右键
		event.nAction = 3; //按下
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);


		CDialog::OnRButtonDown(nFlags, point);
	}
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1)
	{
		CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
		//封装鼠标事件
		MOUSEEV event;
		event.ptXY = remotePoint; //鼠标坐标
		event.nButton = 1; //右键
		event.nAction = 3; //弹起
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);

		CDialog::OnRButtonUp(nFlags, point);
	}
}

//void CWatchScreenDlg::OnMouseMove(UINT nFlags, CPoint point)
//{
//	// TODO: 在此添加消息处理程序代码和/或调用默认值
//	CPoint Serpoint = CliPointToSerPoint(point);
//	MOUSEINFO mouseinfo;
//	mouseinfo.Mousepos.x = Serpoint.x;
//	mouseinfo.Mousepos.y = Serpoint.y;
//	mouseinfo.MouseButton = 3;
//	mouseinfo.MouseAction = 4;
//	CClientSocket::pInvoker->InitSockAddr(IP("127.0.0.1", 9527));
//	CClientSocket::pInvoker->MsgSend(CPacketCli(6, (BYTE*)&mouseinfo, sizeof(mouseinfo)));
//	CClientSocket::pInvoker->MsgRecv();
//
//
//	//CClientSocket::pInvoker->CloseCliSock();
//	CDialog::OnMouseMove(nFlags, point);
//}

void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1)
	{
		CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
		//封装鼠标事件
		MOUSEEV event;
		event.ptXY = remotePoint; //鼠标坐标
		event.nButton = 4; //没有按键 8
		event.nAction = 4; //默认break 0
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); //TODO:网络通信和对话框有耦合
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
		TRACE("移动鼠标触发===========================================\r\n");
		CDialog::OnMouseMove(nFlags, point);
	}
}


void CWatchDialog::OnStnClickedWatch()
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1)
	{
		CPoint point;
		GetCursorPos(&point); //获取鼠标坐标
		CPoint remotePoint = UserPoint2RemoteScreenPoint(point, true);
		//封装鼠标事件
		MOUSEEV event;
		event.ptXY = remotePoint; //鼠标坐标
		event.nButton = 0; //左键
		event.nAction = 0; //单机
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
		TRACE("Danji触发===========================================\r\n");
	}
}


void CWatchDialog::OnOK()
{
	//CDialog::OnOK();//屏蔽回车键
}


void CWatchDialog::OnBnClickedBtnLock()
{
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 7 << 1 | 1);
}


void CWatchDialog::OnBnClickedBtnUnlock()
{
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 8 << 1 | 1);
}
