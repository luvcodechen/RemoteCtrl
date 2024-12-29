// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
//#pragma comment( linker, "/subsystem:windows /entry:WinMainCRTStartup" )
//#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )
//#pragma comment( linker, "/subsystem:console /entry:mainCRTStartup" )
//#pragma comment( linker, "/subsystem:console /entry:WinMainCRTStartup" )

// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

void Dump(BYTE* pData, size_t nSize) //将数据转换为16进制字符串
{
	std::string strOut;
	for (size_t i = 0; i < nSize; i++) //将数据转换为16进制字符串
	{
		char buf[8] = "";
		if (i > 0 && i % 16 == 0) //每16个字节换行
		{
			strOut += "\n";
		}
		snprintf(buf, sizeof(buf), "%02X", pData[i] & 0xFF); //将一个字节转换为16进制字符串
		strOut += buf;
	}
	strOut += "\n";
	OutputDebugStringA(strOut.c_str());
}

int MakeDriverInfo() //1==>A盘 2==>B盘 3==>C盘 .. 26==>Z盘
{
	std::string result;
	for (int i = 1; i <= 26; i++)
	{
		if (_chdrive(i) == 0)
		{
			if (result.size() > 0)
			{
				result += ',';
			}
			result += 'A' + i - 1;
		}
	}
	CPacket packet(1, (BYTE*)result.c_str(), result.size()); //打包数据
	Dump((BYTE*)packet.Data(), packet.Size());
	// CServerSocket::GetInstance()->Send(packet);
	return 0;
}

#include <stdio.h>
#include <io.h>
#include <list>

typedef struct file_info
{
	file_info()
	{
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasFile = TRUE;
		memset(szFIleName, 0, sizeof(szFIleName));
	}

	BOOL IsInvalid; //是否是无效的 TRUE 是 FALSE 不是
	BOOL IsDirectory; //是否是目录 TRUE 是 FALSE 不是
	BOOL HasFile; //是否有文件 TRUE 是 FALSE 不是
	char szFIleName[256];
} FILEINFO, *PFILEINFO;

int MakeDirectoryInfo()
{
	std::string strPath;
	// std::list<file_info> lstFileInfos;
	if (CServerSocket::GetInstance()->GetFilePath(strPath) == false)
	{
		OutputDebugString(_T("当前的命令，不是获取文件列表，命令解析错误"));
		return -1;
	}
	if (_chdir(strPath.c_str()) != 0)
	{
		FILEINFO finfo;
		finfo.IsInvalid = TRUE;
		finfo.IsDirectory = TRUE;
		finfo.HasFile = FALSE;
		memcpy(finfo.szFIleName, strPath.c_str(), strPath.size());
		// lstFileInfos.push_back(finfo);
		CPacket packet(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::GetInstance()->Send(packet);
		OutputDebugString(_T("没有权限访问目录！"));
		return -2;
	}
	_finddata_t fdata; //文件信息
	int hfind = 0;
	if ((hfind = _findfirst("*", &fdata)) == -1)
	{
		OutputDebugString(_T("没有文件！"));
		return -3;
	}

	do
	{
		FILEINFO finfo;
		finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0; //是否是目录
		memcpy(finfo.szFIleName, fdata.name, strlen(fdata.name)); //文件名
		CPacket packet(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::GetInstance()->Send(packet);
		// lstFileInfos.push_back(finfo);
	}
	while (_findnext(hfind, &fdata) == 0);
	FILEINFO finfo;
	finfo.HasFile = FALSE;
	CPacket packet(2, (BYTE*)&finfo, sizeof(finfo));
	CServerSocket::GetInstance()->Send(packet);
	return 0;
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
			// TODO: 在此处为应用程序的行为编写代码。
			// socket、bind,listen,accept,read,write,close

			//套接字初始化
			// server;
			// CServerSocket* pserver = CServerSocket::GetInstance();
			// int count = 0;
			// if (pserver->InitSocket() == false)
			// {
			// 	MessageBox(NULL, L"网络初始化失败", L"错误", MB_OK | MB_ICONERROR);
			// 	exit(0);
			// }
			// while (CServerSocket::GetInstance() != nullptr)
			// {
			// 	if (pserver->AcceptSocket() == false)
			// 	{
			// 		if (count >= 3)
			// 		{
			// 			MessageBox(NULL, L"多次无法正常接入用户", L"接入用户失败", MB_OK | MB_ICONERROR);
			// 			exit(0);
			// 		}
			// 		MessageBox(NULL, L"无法正常接入用户，自动重试", L"接入用户失败", MB_OK | MB_ICONERROR);
			// 		count++;
			// 	}
			// 	int ret = pserver->DealCommand();
			// 	//TODO:处理命令
			// }
			int nCmd = 1;
			switch (nCmd)
			{
			case 1: //获取所有盘符
				MakeDriverInfo();
				break;
			case 2: // 查看指定目录下的文件
				MakeDirectoryInfo();
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
