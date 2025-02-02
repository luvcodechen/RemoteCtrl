// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "ClientController.h"

// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG1, pParent)
{
	m_isFull = false;
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
	ON_MESSAGE(WM_SEND_PACK_ACK, &CWatchDialog::OnSendPacketAck)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen)
{
	//800 450
	CRect clientRect;
	if (!isScreen)
	{
		// CStatic *pStatic = (CStatic*)GetDlgItem(IDC_WATCH);//获取控件指针
		// if(pStatic)
		// {
		// 	CRect rect;
		// 	pStatic->GetWindowRect(rect);
		// 	ScreenToClient(&point); //转换为相对坐标
		// 	point.y-=rect.top;//减去标题栏高度
		// }
		ClientToScreen(&point); //转换为屏幕坐标
	}
	m_picture.ScreenToClient(&point); //转换为相对坐标
	TRACE("x %d y %d\r\n", point.x, point.y);
	m_picture.GetWindowRect(clientRect); //获取控件的坐标
	TRACE("width %d height %d\r\n", clientRect.Width(), clientRect.Height());
	return CPoint(point.x * m_nObjWidth / clientRect.Width(), point.y * m_nObjHeight / clientRect.Height());
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	// SetTimer(0, 45, NULL);//设置定时器
	return TRUE; // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// if (nIDEvent == 0) //定时器
	// {
	// 	// CClientController* pParent = CClientController::getInstance();
	// 	if (m_isFull)
	// 	{
	// 		CRect rect;
	// 		// pParent->getImage().BitBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0,SRCCOPY); //将图片显示到控件上
	// 		m_picture.GetWindowRect(rect); //获取控件大小
	// 		m_nObjWidth = m_image.GetWidth(); //获取图片宽度
	// 		m_nObjHeight = m_image.GetHeight(); //获取图片高度
	// 		m_image.StretchBlt(
	// 			m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(),SRCCOPY); //拉伸图片
	// 		m_picture.InvalidateRect(NULL); //刷新控件
	// 		m_image.Destroy(); //销毁图片
	// 		m_isFull = false;
	// 		TRACE("更新图片完成\r\n");
	// 	}
	// }
	CDialog::OnTimer(nIDEvent);
}


LRESULT CWatchDialog::OnSendPacketAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || lParam == -2)
	{
		//TODO:错误处理
		delete (CPacket*)wParam;
		wParam = NULL;
	}
	else if (lParam == 1)
	{
		//对方关闭了套接字
		delete (CPacket*)wParam;
		wParam = NULL;

	}
	else
	{
		CPacket* pPack = (CPacket*)wParam;
		if (pPack != NULL)
		{
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam;
			wParam= NULL;
			switch (head.sCmd)
			{
			case 6:
				{
					CMyTool::Byte2Image(m_image, head.strData);
					CRect rect;
					m_picture.GetWindowRect(rect); //获取控件大小
					m_nObjWidth = m_image.GetWidth(); //获取图片宽度
					m_nObjHeight = m_image.GetHeight(); //获取图片高度
					m_image.StretchBlt(
						m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY); //拉伸图片
					m_picture.InvalidateRect(NULL); //刷新控件
					m_image.Destroy(); //销毁图片
					m_isFull = false;
					break;
				}
			case 5:
				TRACE("远程端应答鼠标操作\r\n");
				break;
			case 7:
			case 8:
			default:
				break;
			}
		}
	}

	return 0;
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
		CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnLButtonDblClk(nFlags, point);
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
		CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
		TRACE("OnLButtonDown===========================================\r\n");
	}
	CDialog::OnLButtonDown(nFlags, point);
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
		CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}

	CDialog::OnLButtonUp(nFlags, point);
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
		CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
		TRACE("OnRButtonDown===========================================\r\n");
	}
	CDialog::OnRButtonDblClk(nFlags, point);
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
		CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnRButtonDown(nFlags, point);
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
		CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnRButtonUp(nFlags, point);
}

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
		CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
		TRACE("mouse move move move move move move move ========================\r\n");
	}
	CDialog::OnMouseMove(nFlags, point);
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
		CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
		TRACE("OnStnClickedWatch===========================================\r\n");
	}
}


void CWatchDialog::OnOK()
{
	//CDialog::OnOK();//屏蔽回车键
}


void CWatchDialog::OnBnClickedBtnLock()
{
	CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 7);
}


void CWatchDialog::OnBnClickedBtnUnlock()
{
	CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 8);
}
