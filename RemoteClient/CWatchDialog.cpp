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
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point)
{
	//800 450
	CRect clientRect;
	ScreenToClient(&point); //转换为相对坐标
	m_picture.GetWindowRect(clientRect); //获取控件的坐标
	return CPoint(point.x * 3840 / clientRect.Width(), point.y * 2160 / clientRect.Height());
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


void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
	//封装鼠标事件
	MOUSEEV event;
	event.ptXY = remotePoint; //鼠标坐标
	event.nButton = 0; //左键
	event.nAction = 2; //双击
	CClientSocket* pClient = CClientSocket::GetInstance();
	CPacket packet(5, (BYTE*)&event, sizeof(event));
	pClient->Send(packet);
	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	// CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
	// //封装鼠标事件
	// MOUSEEV event;
	// event.ptXY = remotePoint; //鼠标坐标
	// event.nButton = 0; //左键
	// event.nAction = 4; //弹起
	// CClientSocket* pClient = CClientSocket::GetInstance();
	// CPacket packet(5, (BYTE*)&event, sizeof(event));
	// pClient->Send(packet);


	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
	//封装鼠标事件
	MOUSEEV event;
	event.ptXY = remotePoint; //鼠标坐标
	event.nButton = 2; //右键
	event.nAction = 2; //双击
	CClientSocket* pClient = CClientSocket::GetInstance();
	CPacket packet(5, (BYTE*)&event, sizeof(event));
	pClient->Send(packet);


	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
	//封装鼠标事件
	MOUSEEV event;
	event.ptXY = remotePoint; //鼠标坐标
	event.nButton = 2; //右键
	event.nAction = 3; //按下
	CClientSocket* pClient = CClientSocket::GetInstance();
	CPacket packet(5, (BYTE*)&event, sizeof(event));
	pClient->Send(packet);


	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
	//封装鼠标事件
	MOUSEEV event;
	event.ptXY = remotePoint; //鼠标坐标
	event.nButton = 2; //右键
	event.nAction = 4; //弹起
	CClientSocket* pClient = CClientSocket::GetInstance();
	CPacket packet(5, (BYTE*)&event, sizeof(event));
	pClient->Send(packet);


	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
	//封装鼠标事件
	MOUSEEV event;
	event.ptXY = remotePoint; //鼠标坐标
	event.nButton = 0; //左键
	event.nAction = 1; //移动
	CClientSocket* pClient = CClientSocket::GetInstance();
	CPacket packet(5, (BYTE*)&event, sizeof(event));
	pClient->Send(packet);


	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
	CPoint point;
	GetCursorPos(&point); //获取鼠标坐标
	CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
	//封装鼠标事件
	MOUSEEV event;
	event.ptXY = remotePoint; //鼠标坐标
	event.nButton = 0; //左键
	event.nAction = 3; //按下
	CClientSocket* pClient = CClientSocket::GetInstance();
	CPacket packet(5, (BYTE*)&event, sizeof(event));
	pClient->Send(packet);
}
