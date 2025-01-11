#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include <map>
#include "resource.h"
#include "ClientSocket.h"
#include "MyTool.h"
#define WM_SEND_PACK (WM_USER+1)//发送包数据
#define WM_END_DATA (WM_USER+2) //发送数据
#define WM_SHOW_STATUS (WM_USER+3) //显示状态
#define WM_SHOW_WATCH (WM_USER+4) //显示监控
#define WM_SEND_MESSAGE (WM_USER+0x1000) //自定义消息处理

class CClientController
{
public:
	//获取全局唯一对象
	static CClientController* getInstance();
	//初始化
	int InitController();
	//启动
	int Invoke(CWnd*& pMainWnd);
	// 发送消息
	LRESULT SendMessage(MSG msg);
	//更新网络服务器的地址
	void UpdateAdress(int nIP, int nPort)
	{
		CClientSocket::GetInstance()->UpdateAddress(nIP, nPort);
	}

	int DealCommand()
	{
		return CClientSocket::GetInstance()->DealCommand();
	}

	void CloseSocket()
	{
		CClientSocket::GetInstance()->CloseSocket();
	}

	bool SendPacket(const CPacket& pack)
	{
		CClientSocket* pClient = CClientSocket::GetInstance();
		if (pClient->InitSocket() == false)return false;
		pClient->Send(pack);
	}

	// 1 查看磁盘分区 2 查看指定目录下的文件 3 打开文件 4 下载文件 5 鼠标操作 6 发送屏幕内容 7 锁机 8 解锁 9 删除文件 1981 测试连接
	//return :命令号 小于0则失败
	// 实现
	int SendCommandPack(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0)
	{
		CClientSocket* pClient = CClientSocket::GetInstance();
		if (pClient->InitSocket() == false)return false;
		pClient->Send(CPacket(nCmd, pData, nLength));
		int cmd = DealCommand();
		TRACE("ack:%d \r\n", cmd);
		if (bAutoClose)
			CloseSocket();
		return cmd;
	}

	int GetImage(CImage& image)
	{
		CClientSocket* pClient = CClientSocket::GetInstance();
		return CMyTool::Byte2Image(image, pClient->GetPacket().strData);
	}

	int DownloadFile(CString strPath)
	{
		CFileDialog dlg(FALSE, "*",
		                strPath,
		                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		                NULL, &m_remoteDlg);
		if (dlg.DoModal() == IDOK)
		{
			m_strRemote = strPath;
			m_strLocal = dlg.GetPathName();
			m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadEntryForDownFile, 0, this);
			if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT)
			{
				return -1;
			}
			m_remoteDlg.BeginWaitCursor();
			m_statusDlg.m_info.SetWindowText("正在下载文件，请稍后...");
			m_statusDlg.ShowWindow(SW_SHOW); //显示状态对话框
			m_statusDlg.CenterWindow(&m_remoteDlg); //居中显示
			m_statusDlg.SetActiveWindow(); //激活状态对话框
		}


		return 0;
	}
	void StartWatchScreen()
	{
		m_isClosed=false;
		CWatchDialog dlg(&m_remoteDlg);
		m_hThreadWatch=(HANDLE)_beginthread(&CClientController::threadWatchScreen,0,this);
		dlg.DoModal();
		m_isClosed=true;
		WaitForSingleObject(m_hThreadWatch,500);
	}

protected:
	void threadWatchScreen();
	static void threadWatchScreen(void* arg);	
	void threadDownlownFile();
	static void threadEntryForDownFile(void* arg);

	CClientController():
		m_statusDlg(&m_remoteDlg),
		m_watchDLg(&m_remoteDlg)
	{
		m_hThreadDownload = INVALID_HANDLE_VALUE;
		m_hThread = INVALID_HANDLE_VALUE;
		m_hThreadWatch = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;
		m_isClosed = true;
	}

	~CClientController()
	{
		WaitForSingleObject(m_hThread, 100); //等待线程结束
	}

	static unsigned __stdcall threadEntry(void* arg);
	void threadFunc();

	static void DestroyInstance()
	{
		if (m_instance)
		{
			delete m_instance;
			m_instance = NULL;
		}
	}

	LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowSTtatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	typedef struct MsgInfo
	{
		MsgInfo(MSG m)
		{
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
		}

		MsgInfo(const MsgInfo& mi)
		{
			result = mi.result;
			memcpy(&msg, &mi.msg, sizeof(MSG));
		}

		MsgInfo& operator=(const MsgInfo& mi)
		{
			if (this != &mi)
			{
				result = mi.result;
				memcpy(&msg, &mi.msg, sizeof(MSG));
			}
			return *this;
		}

		MSG msg;
		LRESULT result;
	} MSGINFO;

	typedef LRESULT (CClientController::*MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lparam);
	static std::map<UINT, MSGFUNC> m_mapFunc;
	CWatchDialog m_watchDLg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;
	static CClientController* m_instance;
	HANDLE m_hThread;
	HANDLE m_hThreadDownload;
	HANDLE m_hThreadWatch;
	unsigned m_nThreadID;
	CString m_strRemote; //下载文件的远程路径
	CString m_strLocal; //下载文件的本地路径
	bool m_isClosed;//监视是否关闭
	class Chelper
	{
	public:
		Chelper()
		{
			CClientController::getInstance(); //调用GetInstance
		}

		~Chelper()
		{
			CClientController::DestroyInstance(); //调用DestroyInstance
		}
	}; //静态变量初始化
	static Chelper m_helper; //静态变量
};
