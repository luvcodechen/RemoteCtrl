#pragma once
#include <direct.h>
#include <map>
#include  "ServerSocket.h"
#include <atlimage.h>
#include <stdio.h>
#include <io.h>
#include "MyTool.h"
#include "LockDialog.h"
#include "resource.h"
#include "Packet.h"
#pragma warning(disable:4996)//忽略fopen函数的警告
class CCommand
{
public:
	CCommand();

	~CCommand()
	{
	}

	int ExecuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket);

	static void RunCommand(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		CCommand* thiz = (CCommand*)arg;
		if (status > 0)
		{
			int ret = thiz->ExecuteCommand(status, lstPacket, inPacket);
			if (ret != 0)
			{
				TRACE("命令执行失败:%d ret=%d\r\n", status, ret);
			}
		}
		else
		{
			MessageBox(NULL, L"无法正常接入用户，自动重试", L"接入用户失败", MB_OK | MB_ICONERROR);
		}
	}

protected:
	typedef int (CCommand::*CMDFUNC)(std::list<CPacket>&, CPacket& inPacket); //成员函数指针
	std::map<int, CMDFUNC> m_mapCmd; //命令处理函数映射表
	CLockDialog dlg;
	unsigned threadid;

protected:
	static unsigned _stdcall threadLockDlg(void* arg)
	{
		CCommand* thiz = (CCommand*)arg;
		thiz->threadLockDLgMain();
		_endthread();
		return 0;
	}

	void threadLockDLgMain()
	{
		dlg.Create(IDD_DIALOG_INFO, NULL); //创建对话框
		dlg.ShowWindow(SW_SHOW); //显示对话框
		CRect rect; //矩形

		rect.left = 0; //左上角坐标
		rect.top = 0; //左上角坐标
		rect.right = GetSystemMetrics(SM_CXSCREEN); //
		rect.bottom = GetSystemMetrics(SM_CYSCREEN); //全屏显示
		dlg.MoveWindow(rect); //移动对话框
		CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
		if (pText)
		{
			CRect rtText;
			pText->GetWindowRect(rtText);
			int nWidth = rtText.Width() / 2;
			int nHeight = rtText.Height() / 2;
			rtText.left = rect.Width() / 2 - nWidth;
			rtText.top = rect.Height() / 2 - nHeight;
			rtText.right = rect.Width() / 2 + nWidth;
			rtText.bottom = rect.Height() / 2 + nHeight;
			pText->MoveWindow(rtText);
		}
		dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); //置顶
		ShowWindow(FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE); //隐藏任务栏
		ShowCursor(false); //隐藏鼠标
		dlg.GetWindowRect(rect); //获取对话框矩形
		ClipCursor(rect); //限制鼠标活动区域
		MSG msg; //消息
		while (GetMessage(&msg, NULL, 0, 0)) //获取消息
		{
			TranslateMessage(&msg); //翻译消息
			DispatchMessage(&msg); //分发消息
			if (msg.message == WM_KEYDOWN) //关闭消息
			{
				if (msg.wParam == 0x41) //A键
					break;
			}
		}
		ShowWindow(FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW); //显示任务栏
		ShowCursor(true);
		dlg.DestroyWindow(); //销毁对话框
	}

	int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) //1==>A盘 2==>B盘 3==>C盘 .. 26==>Z盘
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
		if (result.size() > 0 && result[result.size() - 1] != ',')
		{
			result += ',';
		}
		lstPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));
		return 0;
	}


	int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData;
		if (_chdir(strPath.c_str()) != 0)
		{
			FILEINFO finfo;
			finfo.HasFile = FALSE;
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));

			OutputDebugString(_T("没有权限访问目录！"));
			return -2;
		}
		_finddata_t fdata; //文件信息
		int hfind = 0;
		if ((hfind = _findfirst("*", &fdata)) == -1)
		{
			OutputDebugString(_T("没有文件！"));
			FILEINFO finfo;
			finfo.HasFile = FALSE;
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));

			return -3;
		}

		do
		{
			FILEINFO finfo;
			finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0; //是否是目录
			memcpy(finfo.szFIleName, fdata.name, strlen(fdata.name)); //文件名
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			// CPacket packet(2, (BYTE*)&finfo, sizeof(finfo));
			// CServerSocket::GetInstance()->Send(packet);
			// lstFileInfos.push_back(finfo);
		}
		while (_findnext(hfind, &fdata) == 0);
		FILEINFO finfo;
		finfo.HasFile = FALSE;
		lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
		return 0;
	}

	int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket) //打开文件
	{
		std::string strPath = inPacket.strData;
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL); //打开文件
		lstPacket.push_back(CPacket(3, NULL, 0));
		return 0;
	}

	int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket) //下载文件
	{
		std::string strPath = inPacket.strData;
		long long data = 0;
		FILE* pFile = nullptr; //文件指针
		errno_t err = fopen_s(&pFile, strPath.c_str(), "rb"); //打开文件
		if (err != 0) //打开文件失败
		{
			lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
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
			lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
			fseek(pFile, 0, SEEK_SET); //将文件指针移动到文件开头
			char buffer[1024] = "";
			size_t rlen = 0;
			do
			{
				rlen = fread(buffer, 1, sizeof(buffer), pFile);
				lstPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));
			}
			while (rlen >= 1024);
			fclose(pFile);
		}
		lstPacket.push_back(CPacket(4, NULL, 0));
		return 0;
	}

	int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		MOUSEEV mouse;
		memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV)); //获取鼠标事件

		SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
		DWORD nFlags = 0;
		switch (mouse.nButton)
		{
		case 0: //左键
			nFlags = 0x01;
			break;
		case 1: //右键
			nFlags = 0x02;
			break;
		case 2: //中键
			nFlags = 0x04;
			break;
		case 4: //没有按键
			nFlags = 0x08;
			break;
		}
		// if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y); //设置鼠标位置
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
		TRACE("mouse event: %08X  x %d y %d\r\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);
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
		case 0x08:
			//单纯的鼠标移动
			//mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());//已弃用
			SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
			break;
		}
		lstPacket.push_back(CPacket(5, NULL, 0));


		return 0;
	}

	int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		CImage screen; //屏幕截图 GDI
		HDC hScreen = ::GetDC(NULL); //获取屏幕DC
		int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL); //每个像素的位数
		int nWidth = GetDeviceCaps(hScreen, HORZRES); //水平分辨率
		int nHeight = GetDeviceCaps(hScreen, VERTRES); //垂直分辨率
		screen.Create(nWidth, nHeight, nBitPerPixel); //创建一个与屏幕相同大小的图片
		BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY); //将屏幕内容拷贝到图片
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

			lstPacket.push_back(CPacket(6, pData, nSize)); //打包数据
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

	int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		if (dlg.m_hWnd == nullptr || dlg.m_hWnd == INVALID_HANDLE_VALUE)
		{
			// _beginthread(threadLockDlg, 0, nullptr);
			_beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid); //创建线程
		}
		lstPacket.push_back(CPacket(7, NULL, 0));


		return 0;
	}

	int UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		// dlg.SendMessage(WM_KEYDOWN, 0x41, 0x001E0001);
		// SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x41, 0x001E0001);
		PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0); //发送消息
		lstPacket.push_back(CPacket(8, NULL, 0)); //解锁
		return 0;
	}

	int TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		lstPacket.push_back(CPacket(1981, NULL, 0));
		return 0;
	}


	int DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData;
		TCHAR sPath[MAX_PATH] = _T(""); //文件路径
		// mbstowcs(sPath, strPath.c_str(), strPath.size());//中文乱码
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(),
		                    strPath.size(), sPath, sizeof(sPath) / sizeof(TCHAR)); //转换为宽字符
		DeleteFile(sPath); //删除文件
		lstPacket.push_back(CPacket(9, NULL, 0));

		return 0;
	}
};
