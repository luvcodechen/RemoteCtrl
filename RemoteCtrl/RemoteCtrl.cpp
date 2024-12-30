// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <atlimage.h>
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

int RunFile() //打开文件
{
	std::string strPath;
	if (CServerSocket::GetInstance()->GetFilePath(strPath) == false)
	{
		OutputDebugString(_T("当前的命令，不是获取文件列表，命令解析错误"));
		return -1;
	}
	ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL); //打开文件
	CPacket packet(3, NULL, 0);
	CServerSocket::GetInstance()->Send(packet);
	return 0;
}
#pragma warning(disable:4996)//忽略fopen函数的警告
int DownloadFile() //下载文件
{
	std::string strPath;
	CServerSocket::GetInstance()->GetFilePath(strPath);
	long long data = 0;
	FILE* pFile = nullptr; //文件指针
	errno_t err = fopen_s(&pFile, strPath.c_str(), "rb"); //打开文件
	if (err != 0) //打开文件失败
	{
		CPacket packet(4, (BYTE*)&data, 8);
		CServerSocket::GetInstance()->Send(packet);
		return -1;
	}
	// pFile = fopen(strPath.c_str(), "rb");  上面的代码等价于这行代码，但是这行代码不安全，容易被攻击，可以采取措施忽略警告信息
	// if (pFile == nullptr) //打开文件失败
	// {
	// 	CPacket packet(4, (BYTE*)&data, 8);
	// 	CServerSocket::GetInstance()->Send(packet);
	// 	return -1;
	// }
	if (pFile != nullptr)
	{
		fseek(pFile, 0, SEEK_END); //将文件指针移动到文件末尾
		data = _ftelli64(pFile); //获取文件大小
		CPacket head(4, (BYTE*)&data, 8);
		fseek(pFile, 0, SEEK_SET); //将文件指针移动到文件开头
		char buffer[1024] = "";
		size_t rlen = 0;
		do
		{
			rlen = fread(buffer, 1, sizeof(buffer), pFile);
			CPacket packet(4, (BYTE*)buffer, rlen);
			CServerSocket::GetInstance()->Send(packet);
		}
		while (rlen >= 1024);
		fclose(pFile);
	}
	CPacket packet(4, NULL, 0);
	CServerSocket::GetInstance()->Send(packet);
	return 0;
}

int MouseEvent()
{
	MOUSEEV mouse;
	if (CServerSocket::GetInstance()->GetMouseEvent(mouse))
	{
		DWORD nFlags = 0;
		switch (mouse.nButton)
		{
		case 0: //左键
			nFlags = 1;
			break;
		case 1: //右键
			nFlags = 2;
			break;
		case 2: //中键
			nFlags = 4;
			break;
		case 4: //没有按键
			nFlags = 8;
			break;
		}
		if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y); //设置鼠标位置
		switch (mouse.nAction)
		{
		case 0: //单机
			nFlags |= 0x10;
			break;
		case 1: //双击
			nFlags |= 0x20;
			break;
		case 2: //按下
			nFlags |= 0x40;
			break;
		case 3: //弹起
			nFlags |= 0x80;
			break;
		default:
			break;
		}

		switch (nFlags)
		{
		case 0x21: //左键双击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x11: //左键单击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x41: //左键按下
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x81: //左键弹起
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x22: //右键双击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x12: //右键单击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x42: //右键按下
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x82: //右键弹起
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x24: //中键双击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x14: //中键单击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x44: //中键按下
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x84: //中键弹起
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x8:
			//单纯的鼠标移动
			mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
			break;
		}
		CPacket packet(5, NULL, 0);
		CServerSocket::GetInstance()->Send(packet);
	}
	else
	{
		OutputDebugString(_T("获取鼠标事件失败"));
		return -1;
	}


	return 0;
}

int SendScreen()
{
	CImage screen; //屏幕截图 GDI
	HDC hScreen = ::GetDC(NULL); //获取屏幕DC
	int nBitPerPixel = GetDeviceCaps(hScreen,BITSPIXEL); //每个像素的位数
	int nWidth = GetDeviceCaps(hScreen,HORZRES); //水平分辨率
	int nHeight = GetDeviceCaps(hScreen,VERTRES); //垂直分辨率
	screen.Create(nWidth, nHeight, nBitPerPixel); //创建一个与屏幕相同大小的图片
	BitBlt(screen.GetDC(), 0, 0, 3840, 2100, hScreen, 0, 0, SRCCOPY); //将屏幕内容拷贝到图片
	ReleaseDC(NULL, hScreen); //释放屏幕DC
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0); //分配内存
	if (hMem == NULL)
	{
		OutputDebugString(_T("分配内存失败"));
		return -1;
	}
	IStream* pStream = NULL;
	HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream); //创建流
	if (ret == S_OK)
	{
		screen.Save(pStream, Gdiplus::ImageFormatJPEG); //保存图片
		LARGE_INTEGER bg = {0}; //大整数
		pStream->Seek(bg, STREAM_SEEK_SET, nullptr); //将流指针移动到文件开头
		PBYTE pData = (PBYTE)GlobalLock(hMem); //锁定内存
		SIZE_T nSize = GlobalSize(hMem); //获取内存大小

		CPacket pack(6, pData, nSize); //打包数据
		CServerSocket::GetInstance()->Send(pack); //发送数据
		GlobalUnlock(hMem); //解锁内存
	}
	pStream->Release(); //释放流
	GlobalFree(hMem); //释放内存
	// DWORD tick = GetTickCount64();
	// screen.Save(_T("screen.jpg"), Gdiplus::ImageFormatJPEG); //保存图片
	// TRACE(_T("保存图片耗时：%d\r\n"), GetTickCount64() - tick);
	// tick = GetTickCount64();
	// screen.Save(_T("screen.png"), Gdiplus::ImageFormatPNG); //保存图片
	// TRACE(_T("保存图片耗时：%d\r\n"), GetTickCount64() - tick);
	screen.ReleaseDC(); //释放图片DC

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
			int nCmd = 6;
			switch (nCmd)
			{
			case 1: //获取所有盘符
				MakeDriverInfo();
				break;
			case 2: // 查看指定目录下的文件
				MakeDirectoryInfo();
				break;
			case 3: //打开文件
				RunFile();
				break;
			case 4:
				DownloadFile();
				break;
			case 5: //鼠标操作
				MouseEvent();
				break;
			case 6: // 发送屏幕内容==>屏幕截图
				SendScreen();
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
