#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include <map>
#include "resource.h"
#include "ClientSocket.h"
#include "MyTool.h"

#define WM_SEND_DATA (WM_USER+2) //发送数据
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


	// 1 查看磁盘分区 2 查看指定目录下的文件 3 打开文件 4 下载文件 5 鼠标操作 6 发送屏幕内容 7 锁机 8 解锁 9 删除文件 1981 测试连接
	//return :状态 
	// 实现
	bool SendCommandPack(
		HWND hWnd,// 数据包收到后，需要应答的窗口句柄
		int nCmd,
		bool bAutoClose = true,
		BYTE* pData = NULL,
		size_t nLength = 0);

	int GetImage(CImage& image)
	{
		CClientSocket* pClient = CClientSocket::GetInstance();
		return CMyTool::Byte2Image(image, pClient->GetPacket().strData);
	}

	int DownloadFile(CString strPath);

	void StartWatchScreen()
	{
		m_isClosed = false;
		// m_watchDLg.SetParent(&m_remoteDlg);
		m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreen, 0, this);
		m_watchDLg.DoModal();
		m_isClosed = true;
		WaitForSingleObject(m_hThreadWatch, 500);
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
			TRACE("m_instance delete\r\n");
		}
	}

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
	bool m_isClosed; //监视是否关闭
	class Chelper
	{
	public:
		Chelper()
		{
			// CClientController::getInstance(); //调用GetInstance
		}

		~Chelper()
		{
			CClientController::DestroyInstance(); //调用DestroyInstance
		}
	}; //静态变量初始化
	static Chelper m_helper; //静态变量
};
