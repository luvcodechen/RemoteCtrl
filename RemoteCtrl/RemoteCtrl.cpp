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

CWinApp theApp;

using namespace std;

void WriteRegisterTable(const CString strPath)
{
	CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	char szPath[MAX_PATH] = "";
	char sSys[MAX_PATH] = ""; //系统目录
	std::string strExe = "\\RemoteCtrl.exe ";
	GetCurrentDirectoryA(MAX_PATH, szPath); //获取当前目录
	GetSystemDirectoryA(sSys, sizeof(sSys)); //获取系统目录
	std::string strCmd = "mklink " + std::string(sSys) + strExe + (std::string)szPath + strExe;
	int ret;
	system(strCmd.c_str());
	HKEY hkey = NULL;
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hkey);
	if (ret != ERROR_SUCCESS)
	{
		RegCloseKey(hkey);
		MessageBox(NULL, _T("设置开机自启失败"), _T("错误"), MB_TOPMOST | MB_ICONERROR);
		::exit(0);
	}
	TCHAR sSysPath[MAX_PATH] = _T(""); //系统目录
	GetSystemDirectoryW(sSysPath, MAX_PATH); //获取系统目录

	ret = RegSetValueEx(hkey, _T("RemoteCtrl"), 0, REG_SZ, (BYTE*)(LPCTSTR)strPath,
	                    strPath.GetLength() * sizeof(TCHAR)); //写入注册表
	if (ret != ERROR_SUCCESS)
	{
		RegCloseKey(hkey);
		MessageBox(NULL, _T("设置开机自启失败,是否权限不足、\r\n程序启动失败"), _T("错误"), MB_TOPMOST | MB_ICONERROR);
		::exit(0);
	}
	RegCloseKey(hkey);
}

void WriteStartupDir(const CString strPath)
{
	// CString strPath = _T("C:\\Users\\15787\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup");
	CString strCmd = GetCommandLine();
	strCmd.Replace(_T("\""), _T(""));
	BOOL ret = CopyFile(strCmd, strPath,FALSE);
	if (ret == FALSE)
	{
		MessageBox(NULL, _T("复制文件失败，是否权限不足"), _T("错误"), MB_TOPMOST | MB_ICONERROR);
		::exit(0);
	}
}

void ChooseAutoInvoke()
{
	CString strPath = CString(_T(
		"C:\\Users\\15787\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe"));
	if (PathFileExists(strPath))
	{
		return;
	}

	CString strInfo = _T("该程序只允许用于合法用途\n");
	strInfo += _T("继续运行程序，将使得机器处于被监控状态\n");
	strInfo += _T("如若取消，请按取消键 \n");
	strInfo += _T("按下“是”，该程序将被复制到机器上并随着系统启动而运行 \n");
	strInfo += _T("按下“否”，该程序只运行一次，不会在系统留下痕迹 \n");
	int ret = MessageBox(NULL, strInfo, _T("提示"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
	if (ret == IDYES)
	{
		// WriteRegisterTable(strPath);
		WriteStartupDir(strPath);
	}
	else if (ret == IDCANCEL)
	{
		::exit(0);
	}
	return;
}

void ShowError()
{
	LPWSTR lpMessageBuf = NULL;
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,NULL, GetLastError(),
	              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	              (LPWSTR)&lpMessageBuf, 0,NULL); //获取错误信息
	OutputDebugString(lpMessageBuf); //输出错误信息

	LocalFree(lpMessageBuf); //释放内存
	exit(0);
}

bool IsAdmin()
{
	HANDLE hToken = NULL; //令牌句柄
	if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY, &hToken)) //打开进程令牌
	{
		ShowError();
		return false;
	}
	TOKEN_ELEVATION eve; //令牌提升
	DWORD len = 0;
	if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == false) //获取令牌信息
	{
		ShowError();
		return false;
	}
	CloseHandle(hToken);
	if (len == sizeof(eve))
	{
		return eve.TokenIsElevated;
	}
	printf("length of tokeninformation is %d\r\n", len);
	return false;
}

void RunAsAdmin()
{
	HANDLE hToken = NULL;
	BOOL ret = LogonUser(L"Administrator",NULL,NULL,LOGON32_LOGON_BATCH,LOGON32_PROVIDER_DEFAULT, &hToken);
	//获取管理员权限
	if (!ret)
	{
		ShowError();
		MessageBox(NULL, _T("登陆错误"), _T("程序错误"), 0);
		::exit(0);
	}
	OutputDebugString(L"Logon administrator success !\r\n");
	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi = {0};
	TCHAR sPath[MAX_PATH] = _T("");
	GetCurrentDirectory(MAX_PATH, sPath);
	CString strCmd = sPath;
	strCmd += _T("\\RemoteCtrl.exe");
	// ret = CreateProcessWithTokenW(hToken,LOGON_WITH_PROFILE,NULL, (LPWSTR)(LPCWSTR)strCmd, CREATE_UNICODE_ENVIRONMENT,
	// NULL, NULL, &si, &pi); //创建进程
	ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, (LPWSTR)(LPCWSTR)strCmd,
	                              CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
	CloseHandle(hToken);
	if (!ret)
	{
		ShowError();
		MessageBox(NULL, strCmd, _T("程序错误"), 0);
		::exit(0);
	}
	WaitForSingleObject(pi.hProcess, INFINITE); //等待进程结束
	CloseHandle(pi.hProcess); //关闭进程句柄
	CloseHandle(pi.hThread); //关闭线程句柄
}

int main()
{
	int nRetCode = 0;
	HMODULE hModule = ::GetModuleHandle(nullptr);

	if (hModule != nullptr)
	{
		// 初始化 MFC 并在失败时显示错误
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: 在此处为应用程序的行为编写代码。
			wprintf(L"错误: MFC 初始化失败\n");
			nRetCode = 1;
		}
		else
		{
			if (IsAdmin())
			{
				OutputDebugString(L"current is run as administrator !\r\n");
				// MessageBox(NULL, _T("管理员"),_T("用户状态"), 0);
			}
			else
			{
				OutputDebugString(L"current is not run as administrator !\r\n");
				RunAsAdmin();
				// MessageBox(NULL, _T("普通用户"), _T("用户状态"), 0);
				return nRetCode;
			}
			CCommand cmd;
			ChooseAutoInvoke();
			int ret = CServerSocket::GetInstance()->Run(&CCommand::RunCommand, &cmd);
			switch (ret)
			{
			case -1:
				MessageBox(NULL, L"网络初始化失败", L"错误", MB_OK | MB_ICONERROR);
				exit(0);
				break;
			case -2:
				MessageBox(NULL, L"多次无法正常接入用户", L"接入用户失败", MB_OK | MB_ICONERROR);
				exit(0);
				break;
			}
		}
	}
	else
	{
		// TODO: 更改错误代码以符合需要
		wprintf(L"错误: GetModuleHandle 失败\n");
		nRetCode = 1;
	}

	return nRetCode;
}
