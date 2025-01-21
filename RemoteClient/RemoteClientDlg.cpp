// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientController.h"
#include "CWatchDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框


CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	  , m_server_address(0)
	  , m_port(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_port);
	DDX_Control(pDX, IDC_TREE_DIR, m_tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}


BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	// ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS2, &CRemoteClientDlg::OnIpnFieldchangedIpaddress2)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_OPEN_FILE, &CRemoteClientDlg::OnOpenFile)

	ON_BN_CLICKED(IDC_BTN_START__WATCH, &CRemoteClientDlg::OnBnClickedBtnStart)
	ON_WM_TIMER()
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CRemoteClientDlg::OnSendPacketAck)
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE); // 设置大图标
	SetIcon(m_hIcon, FALSE); // 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();
	// m_server_address = 0x7f000001; //
	m_server_address = 0xC0A8B185; // 192.168.177.133
	m_port = _T("9527"); //
	UpdateData(false);
	CClientController::getInstance()->UpdateAdress(m_server_address, atoi((LPCTSTR)m_port));
	m_dlgStatus.Create(IDD_DLG_STATUS, this); //创建状态对话框
	m_dlgStatus.ShowWindow(SW_HIDE); //隐藏状态对话框

	return TRUE; // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CRemoteClientDlg::OnBnClickedBtnTest()
{
	CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 1981);
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	bool ret = CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 1, true,NULL, 0);
	if (ret == 0)
	{
		AfxMessageBox(_T("命令处理失败"));
		return;
	}
}


void CRemoteClientDlg::LoadFIleCurrent()
{
	HTREEITEM htree = m_tree.GetSelectedItem(); //获取选中的树控件项
	CString strPath = GetPath(htree);
	m_List.DeleteAllItems(); //删除列表项
	int cmd = CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath,
	                                                            strPath.GetLength());
	PFILEINFO pfileinfo = (PFILEINFO)CClientSocket::GetInstance()->GetPacket().strData.c_str();
	while (pfileinfo->HasFile) //
	{
		TRACE("[%s] isdir %d\r\n", pfileinfo->szFIleName, pfileinfo->IsDirectory); //输出文件信息
		if (!pfileinfo->IsDirectory) //是目录
		{
			m_List.InsertItem(0, pfileinfo->szFIleName); //插入文件
		}

		cmd = CClientSocket::GetInstance()->DealCommand();
		TRACE(" ask:%d \r\n", cmd);
		if (cmd < 0)break;
		pfileinfo = (PFILEINFO)CClientSocket::GetInstance()->GetPacket().strData.c_str(); //
	}
	// pClient->CloseSocket();
}

void CRemoteClientDlg::Str2Tree(const std::string& drivers, CTreeCtrl& tree)
{
	m_tree.DeleteAllItems();
	std::string driver;
	for (size_t i = 0; i < drivers.size(); i++)
	{
		if (drivers[i] == ',')
		{
			driver += ":";
			HTREEITEM htemp = m_tree.InsertItem(driver.c_str(), TVI_ROOT, TVI_LAST); //插入根节点
			m_tree.InsertItem(0, htemp, TVI_LAST);
			driver.clear();
			continue;
		}
		driver += drivers[i];
	}
}

void CRemoteClientDlg::UpdateFileInfo(const FILEINFO& finfo, HTREEITEM hParent)
{
	TRACE("HasFIle ：%d, isdirectory: %d szFileName %s\r\n", finfo.HasFile, finfo.IsDirectory,
	      finfo.szFIleName);
	if (finfo.HasFile == FALSE)return;
	if (finfo.IsDirectory) //是目录
	{
		if (((CString)finfo.szFIleName == ".") || ((CString)finfo.szFIleName == ".."))
		//是当前目录或者上级目录
		{
			return;
		}
		HTREEITEM htemp = m_tree.InsertItem(finfo.szFIleName, hParent, TVI_LAST); //插入文件
		m_tree.InsertItem(0, htemp, TVI_LAST); //插入子目录)
		m_tree.Expand(hParent, TVE_EXPAND); //展开目录
	}
	else
	{
		m_List.InsertItem(0, finfo.szFIleName); //插入文件
	}
}

void CRemoteClientDlg::UpdateDownloadFile(const std::string& strData, FILE* pFile)
{
	static long long length = 0, index = 0;
	if (length == 0)
	{
		length = *(long long*)strData.c_str();
		if (length == 0)
		{
			AfxMessageBox(_T("文件长度为零或者无法读取文件！"));
			CClientController::getInstance()->DownloadEnd();
			return;
		}
	}
	else if (0 < length && index >= length)
	{
		fclose(pFile);
		length = 0;
		index = 0;
		CClientController::getInstance()->DownloadEnd();
	}
	else
	{
		fwrite(strData.c_str(), 1, strData.size(), pFile);
		index += strData.size();
		TRACE("");
		if (index >= length)
		{
			fclose(pFile);
			length = 0;
			index = 0;
			CClientController::getInstance()->DownloadEnd();
		}
	}
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse); //获取鼠标位置
	m_tree.ScreenToClient(&ptMouse); //转换为树控件的客户区坐标
	HTREEITEM hTreeSelected = m_tree.HitTest(ptMouse, 0); //获取鼠标所在的树控件项
	if (hTreeSelected == NULL) //没有选中
		return;
	DeleteTreeChildItem(hTreeSelected); //删除子项
	m_List.DeleteAllItems(); //删除列表项
	CString strPath = GetPath(hTreeSelected);
	TRACE("htreeSelected %08X \r\n");
	CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath,
	                                                  strPath.GetLength(), (WPARAM)hTreeSelected);
}

CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strPath, strTemp;
	do
	{
		strTemp = m_tree.GetItemText(hTree); //获取当前项的文本
		strPath = strTemp + "\\" + strPath; //拼接路径
		hTree = m_tree.GetParentItem(hTree); //获取父项
	}
	while (hTree != NULL);

	return strPath;
}

void CRemoteClientDlg::DeleteTreeChildItem(HTREEITEM hTree)
{
	HTREEITEM sub = NULL;
	do
	{
		sub = m_tree.GetChildItem(hTree); //获取子项
		if (sub != NULL)
			m_tree.DeleteItem(sub); //删除子项
	}
	while (sub != NULL);
}

void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;
	CPoint ptMouse, ptList; //鼠标位置
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);
	int ListSelected = m_List.HitTest(ptList); //获取鼠标所在的列表项
	if (ListSelected < 0) return;
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK); //加载菜单
	CMenu* pPupup = menu.GetSubMenu(0);
	if (pPupup)
	{
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this); //弹出菜单
	}
}


void CRemoteClientDlg::OnDownloadFile()
{
	int nListSelected = m_List.GetSelectionMark(); //获取选中的列表项
	CString strFile = m_List.GetItemText(nListSelected, 0); //获取选中的文件名
	HTREEITEM hSelected = m_tree.GetSelectedItem(); //获取选中的树控件项
	strFile = GetPath(hSelected) + strFile;
	int ret = CClientController::getInstance()->DownloadFile(strFile);
	if (ret != 0)
	{
		MessageBox(_T("下载失败"));
		TRACE("下载失败 ret =%d\r\n", ret);
	}
}


void CRemoteClientDlg::OnDeleteFile()
{
	HTREEITEM hSelected = m_tree.GetSelectedItem(); //获取选中的树控件项
	CString strPath = GetPath(hSelected); //获取选中的文件路径
	int nSelected = m_List.GetSelectionMark(); //获取选中的列表项
	CString strFile = m_List.GetItemText(nSelected, 0); //获取选中的文件名
	strFile = strPath + strFile;
	int ret = CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 9, true, (BYTE*)(LPCTSTR)strFile,
	                                                            strFile.GetLength());
	if (ret < 0)
	{
		AfxMessageBox(_T("删除文件失败"));
	}
	LoadFIleCurrent();
}


void CRemoteClientDlg::OnOpenFile()
{
	HTREEITEM hSelected = m_tree.GetSelectedItem(); //获取选中的树控件项
	CString strPath = GetPath(hSelected); //获取选中的文件路径
	int nSelected = m_List.GetSelectionMark(); //获取选中的列表项
	CString strFile = m_List.GetItemText(nSelected, 0); //获取选中的文件名
	strFile = strPath + strFile;
	int ret = CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 3, true, (BYTE*)(LPCTSTR)strFile,
	                                                            strFile.GetLength());
	if (ret < 0)
	{
		AfxMessageBox(_T("打开文件失败"));
	}
}


void CRemoteClientDlg::OnBnClickedBtnStart()
{
	CClientController::getInstance()->StartWatchScreen();
}


void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
}


void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	*pResult = 0;
	UpdateData(); //更新数据
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAdress(m_server_address, atoi((LPCTSTR)m_port));
}


void CRemoteClientDlg::OnEnChangeEditPort()
{
	UpdateData(); //更新数据
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAdress(m_server_address, atoi((LPCTSTR)m_port));
}

LRESULT CRemoteClientDlg::OnSendPacketAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || lParam == -2)
	{
		TRACE("socket is error %d \r\n", lParam);
	}
	else if (lParam == 1)
	{
		//对方关闭了套接字
		TRACE("socket is closed!\r\n");
	}
	else
	{
		if (wParam != NULL)
		{
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam;//释放内存
			wParam= NULL;//防止重复释放
			switch (head.sCmd)
			{
			case 1: //获取驱动信息
				Str2Tree(head.strData, m_tree);
				break;
			case 2: //获取文件信息
				UpdateFileInfo(*(PFILEINFO)head.strData.c_str(), (HTREEITEM)lParam);
				break;
			case 3:
				TRACE("run file done\r\n");
				break;
			case 4:
				UpdateDownloadFile(head.strData, (FILE*)lParam);
				break;
			case 9:
				TRACE("删除文件成功\r\n");
				break;
			case 1981:
				TRACE("测试连接成功\r\n");
				break;
			default:
				TRACE("unknow data received! %d\r\n", head.sCmd);
				break;
			}
		}
	}

	return 0;
}
