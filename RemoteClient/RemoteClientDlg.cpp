// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"

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

int CRemoteClientDlg::SendCommandPack(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
{
	UpdateData(); //更新数据
	CClientSocket* pClient = CClientSocket::GetInstance();
	bool ret = pClient->InitSocket(m_server_address, atoi((LPCTSTR)m_port));
	if (!ret)
	{
		AfxMessageBox(_T("连接服务器失败"));
		return -1;
	}
	CPacket packet(nCmd, pData, nLength);
	ret = pClient->Send(packet);
	TRACE("send ret:%d \r\n", ret);
	int cmd = pClient->DealCommand();
	TRACE("ack:%d \r\n", cmd);
	if (bAutoClose)
		pClient->CloseSocket();
	return cmd;
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
	m_server_address = 0x7f000001; //
	m_port = _T("9527"); //
	UpdateData(false);
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
	//TODO: 在此添加控件通知处理程序代码
	SendCommandPack(1981);
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	// TODO: 在此添加控件通知处理程序代码
	int ret = SendCommandPack(1);
	if (ret == -1)
	{
		AfxMessageBox(_T("命令处理失败"));
		return;
	}
	CClientSocket* pClient = CClientSocket::GetInstance();
	std::string drivers = pClient->GetPacket().strData;
	std::string driver;
	m_tree.DeleteAllItems();
	for (size_t i = 0; i < drivers.size(); i++)
	{
		if (drivers[i] == ',')
		{
			driver += ":";
			HTREEITEM htemp = m_tree.InsertItem(driver.c_str(),TVI_ROOT,TVI_LAST); //插入根节点
			m_tree.InsertItem(0, htemp, TVI_LAST);
			driver.clear();
			continue;
		}
		driver += drivers[i];
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
	if (m_tree.ItemHasChildren(hTreeSelected) == NULL)return; //没有子项
	DeleteTreeChildItem(hTreeSelected); //删除子项
	m_List.DeleteAllItems(); //删除列表项
	CString strPath = GetPath(hTreeSelected);
	int cmd = SendCommandPack(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pfileinfo = (PFILEINFO)CClientSocket::GetInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::GetInstance();
	while (pfileinfo->HasFile) //
	{
		TRACE("[%s] isdir %d\r\n", pfileinfo->szFIleName, pfileinfo->IsDirectory); //输出文件信息
		if (pfileinfo->IsDirectory) //是目录
		{
			if (((CString)pfileinfo->szFIleName == ".") || ((CString)pfileinfo->szFIleName == "..")) //是当前目录或者上级目录
			{
				cmd = pClient->DealCommand();
				TRACE(" ask:%d \r\n", cmd);
				if (cmd < 0)break;
				pfileinfo = (PFILEINFO)pClient->GetPacket().strData.c_str(); //
				continue;
			}
			HTREEITEM htemp = m_tree.InsertItem(pfileinfo->szFIleName, hTreeSelected, TVI_LAST); //插入文件
			m_tree.InsertItem(0, htemp, TVI_LAST); //插入子目录)
		}
		else
		{
			m_List.InsertItem(0, pfileinfo->szFIleName); //插入文件
		}

		cmd = pClient->DealCommand();
		TRACE(" ask:%d \r\n", cmd);
		if (cmd < 0)break;
		pfileinfo = (PFILEINFO)pClient->GetPacket().strData.c_str(); //
	}
	pClient->CloseSocket();
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
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
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

	CFileDialog dlg(FALSE, "*",
	                strFile,
	                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
	                NULL, this);
	if (dlg.DoModal() == IDOK)
	{
		FILE* fp = fopen(dlg.GetPathName(), "wb+");
		if (fp == NULL)
		{
			AfxMessageBox("本地没有权限保存该文件|文件无法创建");
			return;
		}
		HTREEITEM hSelected = m_tree.GetSelectedItem(); //获取选中的树控件项
		strFile = GetPath(hSelected) + strFile;
		TRACE("%s \r\n", (LPCTSTR)strFile);
		int ret = SendCommandPack(4, false, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
		if (ret < 0)
		{
			AfxMessageBox(_T("下载文件失败"));
			TRACE("执行下载失败 ret=%d\r\n", ret);
			return;
		}
		CClientSocket* pClient = CClientSocket::GetInstance();
		long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
		if (nLength == 0)
		{
			AfxMessageBox("文件长度为0或无法下载");
			return;
		}
		long long count = 0;

		while (count < nLength)
		{
			ret = pClient->DealCommand();
			if (ret < 0)
			{
				AfxMessageBox(_T("传输失败"));
				TRACE("传输失败 ret=%d\r\n", ret);
				break;
			}
			fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), fp);
			count += pClient->GetPacket().strData.size();
		}
		AfxMessageBox("下载成功");
		fclose(fp);
		pClient->CloseSocket();
	}
}


void CRemoteClientDlg::OnDeleteFile()
{
	// TODO: 在此添加命令处理程序代码
}


void CRemoteClientDlg::OnOpenFile()
{
	// TODO: 在此添加命令处理程序代码
}
