// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include "conio.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
// #pragma comment( linker, "/subsystem:windows /entry:WinMainCRTStartup" )
// #pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )
// #pragma comment( linker, "/subsystem:console /entry:mainCRTStartup" )
// #pragma comment( linker, "/subsystem:console /entry:WinMainCRTStartup" )

// 唯一的应用程序对象

#define INVOKE_PATH _T("C:\\Users\\15787\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")
CWinApp theApp;

using namespace std;


bool ChooseAutoInvoke(const CString& strPath)
{
	if (PathFileExists(strPath))
	{
		return true;
	}

	CString strInfo = _T("该程序只允许用于合法用途\n");
	strInfo += _T("继续运行程序，将使得机器处于被监控状态\n");
	strInfo += _T("如若取消，请按取消键 \n");
	strInfo += _T("按下“是”，该程序将被复制到机器上并随着系统启动而运行 \n");
	strInfo += _T("按下“否”，该程序只运行一次，不会在系统留下痕迹 \n");
	int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
	if (ret == IDYES)
	{
		// CMyTool::WriteRegisterTable(strPath);
		if (!CMyTool::WriteStartupDir(strPath))
		{
			MessageBox(NULL, _T("复制文件失败，是否权限不足\r\n"),_T("错误"),MB_ICONERROR | MB_TOPMOST);
			return false;
		}
	}
	else if (ret == IDCANCEL)
	{
		return false;
	}
	return true;
}

enum
{
	IocpListEmpty,
	IocpListPush,
	IocpListPop
};

typedef struct IocpParam
{
	int nOperator; //操作
	std::string strData; //数据
	_beginthread_proc_type cbFunc; //回调函数
	IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL)
	{
		nOperator = op;
		strData = sData;
		cbFunc = cb;
	}

	IocpParam()
	{
		nOperator = -1;
		strData = "";
	}
} IOCP_PARAM;

void threadQueueEntry(HANDLE HIOCP)
{
	std::list<std::string> lststring;
	DWORD dwTransferred = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED* pOverlapped = NULL;
	while (GetQueuedCompletionStatus(HIOCP, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE))
	{
		if (CompletionKey == 0 && dwTransferred == NULL)
		{
			printf("exit threadQueueEntry\r\n");
			break;
		}
		IOCP_PARAM* pParam = (IOCP_PARAM*)CompletionKey;
		if (pParam->nOperator = IocpListPush)
		{
			lststring.push_back(pParam->strData);
		}
		else if (pParam->nOperator = IocpListPop)
		{
			std::string* pStr = NULL;
			if (lststring.size() > 0)
			{
				pStr = new std::string(lststring.front());
				lststring.pop_front();
			}
			if (pParam->cbFunc)
			{
				pParam->cbFunc(pStr);
			}
		}
		else if (pParam->nOperator = IocpListEmpty)
		{
			lststring.clear();
		}
		delete pParam;
		pParam = NULL;
	}
	_endthread();
}

void func(void* arg)
{
	std::string* pstr = (std::string*)arg;
	if (pstr != NULL)
	{
		printf("pop from list:%s\r\n", pstr->c_str()); //输出
		delete pstr;
		pstr = NULL;
	}
	else
	{
		printf("list is no data\r\n");
	}
}

int main()
{
	if (!CMyTool::Init())return 1;
	printf("press any key to exit ..\r\n");
	HANDLE hIOCP = INVALID_HANDLE_VALUE; //	IO Completion Port
	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1); //创建IOCP
	HANDLE hThread = (HANDLE)_beginthread(threadQueueEntry, 0, hIOCP);

	ULONGLONG tick = GetTickCount64();
	while (_kbhit() != 0) // 完成端口 把请求和实现 分离 了
	{
		if (GetTickCount64() - tick > 1300)
		{
			PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM),
			                           (ULONG_PTR)new IOCP_PARAM(IocpListPop, "hello world"), NULL); //唤醒完成端口
		}
		if (GetTickCount64() - tick > 2000)
		{
			PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM),
			                           (ULONG_PTR)new IOCP_PARAM(IocpListPush, "hello world"), NULL); //唤醒完成端口
			tick = GetTickCount64();
		}
		Sleep(1);
	}
	if (hIOCP != NULL) //关闭IOCP
	{
		//TODO：唤醒完成端口
		PostQueuedCompletionStatus(hIOCP, 0, NULL, NULL); //唤醒完成端口
		WaitForSingleObject(hThread, INFINITE);
	}
	CloseHandle(hIOCP);
	printf("exit done!\r\n");
	::exit(0);
	// if (CMyTool::IsAdmin())
	// {
	// 	if (!CMyTool::Init())return 1;
	// 	if (ChooseAutoInvoke(INVOKE_PATH))
	// 	{
	// 		CCommand cmd;
	// 		int ret = CServerSocket::GetInstance()->Run(&CCommand::RunCommand, &cmd);
	// 		switch (ret)
	// 		{
	// 		case -1:
	// 			MessageBox(NULL, L"网络初始化失败", L"错误", MB_OK | MB_ICONERROR);
	// 			break;
	// 		case -2:
	// 			MessageBox(NULL, L"多次无法正常接入用户", L"接入用户失败", MB_OK | MB_ICONERROR);
	// 			break;
	// 		}
	// 	}
	// }
	// else
	// {
	// 	if (CMyTool::RunAsAdmin() == false)
	// 	{
	// 		CMyTool::ShowError();
	// 		return 1;
	// 	}
	// }
	return 0;
}
