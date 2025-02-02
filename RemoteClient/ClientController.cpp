#include "pch.h"
#include "ClientController.h"

CClientController* CClientController::m_instance = NULL;
std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController::Chelper CClientController::m_helper;

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


bool CClientController::SendCommandPack(HWND hWnd, int nCmd, bool bAutoClose, BYTE* pData, size_t nLength,
                                        WPARAM wParam)
{
	TRACE("cmd:%d %s start %lld \r\n", nCmd, __FUNCTION__, GetTickCount64()); //打印调试信息
	CClientSocket* pClient = CClientSocket::GetInstance();
	bool ret = pClient->SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);
	return ret;
}

void CClientController::DownloadEnd()
{
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor(); //隐藏等待光标
	m_remoteDlg.MessageBox(_T("下载完成"), _T("完成"));
}

int CClientController::DownloadFile(CString strPath)
{
	CFileDialog dlg(FALSE, "*",
	                strPath,
	                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
	                NULL, &m_remoteDlg);
	if (dlg.DoModal() == IDOK)
	{
		m_strRemote = strPath;
		m_strLocal = dlg.GetPathName();
		FILE* pFile = fopen(m_strLocal, "wb+");
		if (pFile == NULL)
		{
			AfxMessageBox("本地没有权限保存该文件|文件无法创建");
			return -1;
		}
		SendCommandPack(m_remoteDlg, 4, false, (BYTE*)(LPCTSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
		m_remoteDlg.BeginWaitCursor();
		m_statusDlg.m_info.SetWindowText("正在下载文件，请稍后...");
		m_statusDlg.ShowWindow(SW_SHOW); //显示状态对话框
		m_statusDlg.CenterWindow(&m_remoteDlg); //居中显示
		m_statusDlg.SetActiveWindow(); //激活状态对话框
	}


	return 0;
}

void CClientController::threadWatchScreen()
{
	Sleep(50);
	ULONGLONG nTick = GetTickCount64();
	while (!m_isClosed)
	{
		if (m_watchDLg.isFull() == false)
		{
			if (GetTickCount64() - nTick < 1000)
			{
				Sleep(1000 - (DWORD)(GetTickCount64() - nTick)); //控制发送频率
			}
			nTick = GetTickCount64(); //更新时间
			int ret = SendCommandPack(m_watchDLg.GetSafeHwnd(), 6, true,NULL, 0);
			if (ret == 1)
			{
				// TRACE("获取图像成功\r\n");
			}
			else
			{
				TRACE("获取图像失败ret= %d \r\n", ret);
			}
		}
		// Sleep(1);
	}
}

void CClientController::threadWatchScreen(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScreen();
	_endthread();
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

LRESULT CClientController::OnShowSTtatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDLg.DoModal();
}
