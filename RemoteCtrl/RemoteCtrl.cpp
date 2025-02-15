// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include "conio.h"
#include "MyQueue.h"
#include <MSWSock.h>
#include "MyServer.h"
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

void threadmain(HANDLE HIOCP)
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
}

void threadQueueEntry(HANDLE HIOCP)
{
	threadmain(HIOCP);
	_endthread(); //代码到此位置，会导致本地对象无法调用析构进行释放，从而导致内存泄漏
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


void test()
{
	MyQueue<std::string> lstStrings;
	ULONGLONG tick0 = GetTickCount64(), tick = GetTickCount64(), total = GetTickCount64();
	printf("press any key to exit ..\r\n");
	while (GetTickCount64() - total <= 1000)
	{
		lstStrings.PushBack("hello world");
		tick0 = GetTickCount64();
	}
	printf("exit done! size %d\r\n", lstStrings.Size());
	total = GetTickCount64();
	while (GetTickCount64() - total <= 1000)
	{
		std::string str;
		lstStrings.PopFront(str);
		tick = GetTickCount64();
	}
	printf("exit done! size %d\r\n", lstStrings.Size());
	lstStrings.Clear(); //清空队列
	std::list<std::string> lstData;
	total = GetTickCount64();
	while (GetTickCount64() - total <= 1000)
	{
		lstData.push_back("hello world");
	}
	printf("lstData push done! size %d\r\n", lstData.size());
	total = GetTickCount64();
	while (GetTickCount64() - total <= 500)
	{
		if (lstData.size() > 0)
			lstData.pop_front();
	}
	printf("exit done! size %d\r\n", lstData.size());
}

void iocp();
void udp_server();
void udp_client(bool ishost = true);
void initsock();
void clearsock();

int main(int argc, char* argv[])
{
	if (!CMyTool::Init())return 1;
	initsock();
	if (argc == 1) //主机
	{
		char wstrDir[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, wstrDir);
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		memset(&si, 0, sizeof(si));
		memset(&pi, 0, sizeof(pi));

		string strCmd = argv[0];
		strCmd += " 1";
		BOOL bRET = CreateProcessA(NULL, (LPSTR)strCmd.c_str(),NULL,NULL,FALSE, 0,NULL, wstrDir, &si,
		                           &pi);
		//创建一个新的进程
		if (bRET) //创建成功
		{
			CloseHandle(pi.hProcess); //关闭进程句柄
			CloseHandle(pi.hThread); //关闭线程句柄
			TRACE("进程id %d \r\n", pi.dwProcessId);
			TRACE("线程id %d \r\n", pi.dwThreadId);
			strCmd += " 2";
			bRET = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir,
			                      &si,
			                      &pi);
			//创建一个新的进程
			if (bRET) //创建成功
			{
				CloseHandle(pi.hProcess); //关闭进程句柄
				CloseHandle(pi.hThread); //关闭线程句柄
				TRACE("进程id %d \r\n", pi.dwProcessId);
				TRACE("线程id %d \r\n", pi.dwThreadId);
				udp_server(); //服务器代码
			}
		}
	}
	else if (argc == 2) //主客户端
	{
		udp_client();
	}
	else //从客户端
	{
		udp_client(false);
	}

	// iocp();

	clearsock();
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

// class COverlapped
// {
// public:
// 	OVERLAPPED m_Overlapped;
// 	DWORD m_operator;
// 	char m_Buffer[4096];
// 	// SOCKET m_Socket;
//
// 	COverlapped()
// 	{
// 		m_operator = 0;
// 		memset(&m_Overlapped, 0, sizeof(OVERLAPPED));
// 		memset(m_Buffer, 0, sizeof(m_Buffer));
// 		// m_Socket = INVALID_SOCKET;
// 	}
// };

void iocp()
{
	MyServer server;
	server.StartService();
	getchar();
}

void initsock()
{
	WSADATA wsaData; //初始化WSA
	WSAStartup(MAKEWORD(2, 2), &wsaData); //初始化WSA
}

void clearsock()
{
	WSACleanup();
}

void udp_server()
{
	printf("%s(%d):%s\r\n",__FILE__,__LINE__, __FUNCTION__);
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET)
	{
		printf("%s(%d):%s error!%d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
		return;
	}
	std::list<sockaddr_in> lstClients;
	sockaddr_in server, client;
	memset(&server, 0, sizeof(server));
	memset(&client, 0, sizeof(client));
	server.sin_family = PF_INET; //协议族
	server.sin_port = htons(20000); //端口
	server.sin_addr.s_addr = inet_addr("127.0.0.1"); //IP地址

	if (-1 == bind(sock, (sockaddr*)&server, sizeof(server)))
	{
		printf("%s(%d):%s error!%d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
		closesocket(sock);
		return;
	}
	std::string buf;
	buf.resize(1024 * 256);
	memset((char*)buf.c_str(), 0, buf.size());
	int len = sizeof(client);
	int ret = 0;
	while (!_kbhit())
	{
		ret = recvfrom(sock, (char*)buf.c_str(), sizeof(buf), 0, (sockaddr*)&client, &len);
		if (ret > 0)
		{
			if (lstClients.size() <= 0)
			{
				lstClients.push_back(client);
				// CMyTool::Dump((BYTE*)buf.c_str(), ret);
				printf("%s(%d):%s  ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr,
				       ntohs(client.sin_port));
				ret = sendto(sock, buf.c_str(), ret, 0, (sockaddr*)&client, len);
				printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
			}
			else
			{
				memcpy((void*)buf.c_str(), &lstClients.front(), sizeof(lstClients.front()));
				ret = sendto(sock, buf.c_str(), sizeof(lstClients.front()), 0, (sockaddr*)&client, len);
				printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
			}
		}
		else
		{
			printf("%s(%d):%s error!%d ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
		}
		// Sleep(1);
	}
	closesocket(sock);
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
}

void udp_client(bool ishost)
{
	Sleep(2000); //等待服务器启动
	sockaddr_in addr, client;
	int len = sizeof(client);
	addr.sin_family = PF_INET; //协议族
	addr.sin_port = htons(20000); //端口
	addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //IP地址
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET)
	{
		printf("%s(%d):%s error!\r\n", __FILE__, __LINE__, __FUNCTION__);
		return;
	}
	if (ishost) //主客户端
	{
		printf("%s(%d):%s\r\n",__FILE__,__LINE__, __FUNCTION__);
		std::string msg = "hello world\n";
		int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&addr, sizeof(addr));
		printf("%s(%d):%s ret= %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
		if (ret > 0)
		{
			msg.resize(1024);
			memset((char*)msg.c_str(), 0, msg.size());
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			printf("host %s(%d):%s ERROR(%d) ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			if (ret > 0)
			{
				printf("%s(%d):%s  ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr,
				       ntohs(client.sin_port));
				printf("%s(%d):%s msg= %d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.size());
			}
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			printf("host %s(%d):%s ERROR(%d) ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			if (ret > 0)
			{
				printf("%s(%d):%s  ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr,
				       ntohs(client.sin_port));
				printf("%s(%d):%s msg= %s\r\n", __FILE__, __LINE__, __FUNCTION__, msg.c_str());
			}
		}
	}
	else //从客户端
	{
		printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
		std::string msg = "hello world\n";
		int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&addr, sizeof(addr));
		printf("%s(%d):%s ret= %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
		if (ret > 0)
		{
			msg.resize(1024);
			memset((char*)msg.c_str(), 0, msg.size());
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			printf("client %s(%d):%s ERROR(%d) ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);

			if (ret > 0)
			{
				sockaddr_in addr;
				memcpy(&addr, msg.c_str(), sizeof(addr));
				sockaddr_in* paddr = (sockaddr_in*)&addr;
				printf("%s(%d):%s  ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr,
				       ntohs(client.sin_port));
				printf("%s(%d):%s msg= %d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.size());
				printf("%s(%d):%s  ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, paddr->sin_addr.s_addr,
				       ntohs(paddr->sin_port));
				msg = "hello, i am client!\r\n";
				ret = sendto(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)paddr, sizeof(sockaddr_in));
				printf("%s(%d):%s  ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, paddr->sin_addr.s_addr,
					ntohs(paddr->sin_port));
				printf("client %s(%d):%s ERROR(%d) ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(),
				       ret);
			}
		}
	}
	closesocket(sock);
}
