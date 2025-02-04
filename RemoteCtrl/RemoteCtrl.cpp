// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"


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


int main()
{
	if (CMyTool::IsAdmin())
	{
		if (!CMyTool::Init())return 1;
		if (ChooseAutoInvoke(INVOKE_PATH))
		{
			CCommand cmd;
			int ret = CServerSocket::GetInstance()->Run(&CCommand::RunCommand, &cmd);
			switch (ret)
			{
			case -1:
				MessageBox(NULL, L"网络初始化失败", L"错误", MB_OK | MB_ICONERROR);
				break;
			case -2:
				MessageBox(NULL, L"多次无法正常接入用户", L"接入用户失败", MB_OK | MB_ICONERROR);
				break;
			}
		}
	}
	else
	{
		if (CMyTool::RunAsAdmin() == false)
		{
			CMyTool::ShowError();
			return 1;
		}
	}
	return 0;
}
