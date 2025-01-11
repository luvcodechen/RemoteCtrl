#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include <map>
#include "resource.h"
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

protected:
	CClientController():
		m_statusDlg(&m_remoteDlg),
		m_watchDLg(&m_remoteDlg)
	{
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;
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
	unsigned m_nThreadID;

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
