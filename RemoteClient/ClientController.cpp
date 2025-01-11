#include "pch.h"
#include "ClientController.h"

CClientController* CClientController::m_instance = NULL;
std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;

CClientController* CClientController::getInstance()
{
	if (m_instance == NULL)
	{
		m_instance = new CClientController();
		struct
		{
			UINT nMsg;
			MSGFUNC func;
		} MsgFuncs[] =
			{
				{WM_SEND_PACK, &CClientController::OnSendPack},
				{WM_END_DATA, &CClientController::OnSendData},
				{WM_SHOW_STATUS, &CClientController::OnShowSTtatus},
				{WM_SHOW_WATCH, &CClientController::OnShowWatcher},
				{(UINT)-1,NULL}
			};
		for (int i = 0; MsgFuncs[i].func != NULL; i++)
		{
			m_mapFunc[MsgFuncs[i].nMsg] = MsgFuncs[i].func;
		}
	}
	return m_instance;
}

int CClientController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientController::threadEntry, this, 0, &m_nThreadID);
	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg); //创建状态对话框
	return 0;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}

LRESULT CClientController::SendMessage(MSG msg)
{
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); //创建事件
	if (hEvent == NULL)return -2;
	MSGINFO info(msg);
	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM)&info, LPARAM(hEvent));
	WaitForSingleObject(hEvent, INFINITE); //等待事件
	return info.result; //返回结果
}

unsigned __stdcall CClientController::threadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadFunc();
	_endthreadex(0);
	return 0;
}

void CClientController::threadFunc()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg); //转换消息
		DispatchMessage(&msg); //分发消息

		if (msg.message == WM_SEND_MESSAGE) //如果是自定义消息
		{
			MSGINFO* pmsg = (MSGINFO*)msg.wParam;
			HANDLE hEvent = (HANDLE)msg.lParam;
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(pmsg->msg.message);
			if (it != m_mapFunc.end())
			{
				pmsg->result = (this->*(it->second))(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);
			}
			else
			{
				pmsg->result = -1;
			}
			SetEvent(hEvent); //设置事件
		}
		else //
		{
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end())
			{
				MSGFUNC func = it->second;
				(this->*func)(msg.message, msg.wParam, msg.lParam);
			}
		}
	}
}

LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}

LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}

LRESULT CClientController::OnShowSTtatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDLg.DoModal();
}
