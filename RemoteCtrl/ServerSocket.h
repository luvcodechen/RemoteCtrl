#pragma once
#include "pch.h"
#include "framework.h"

class CServerSocket
{
public:
	static CServerSocket* GetInstance()
	{
		if (m_pInstance == NULL)
		{
			m_pInstance = new CServerSocket();
		}
		return m_pInstance;
	} //获取单例
	BOOL InitSocket()
	{
		if (m_socket == -1)
		{
			return FALSE;
		}

		sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		server_addr.sin_port = htons(9527);

		if (bind(m_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == -1)
		{
			return FALSE;
		}

		if (listen(m_socket, 1) == -1) //
		{
			return FALSE;
		}
		return TRUE;
	}

	BOOL AcceptSocket()
	{
		sockaddr_in client_addr;
		char buffer[1024]; //缓冲区
		int cli_sz = sizeof(client_addr);
		m_client = accept(m_socket, (sockaddr*)&client_addr, &cli_sz);
		if (m_client == -1)
		{
			return FALSE;
		}
		return TRUE;
	}

	int DealCommand()
	{
		if (m_client == -1)
		{
			return -1;
		}
		char buffer[1024] = ""; //缓冲区
		while (1)
		{
			memset(buffer, 0, sizeof(buffer));
			int len = recv(m_client, buffer, sizeof(buffer), 0); //接收数据
			if (len <= 0)
			{
				return -1;
			}
			//TODO:处理接收到的数据
		}
	}

	bool Send(const char* buffer, int len)
	{
		if (send(m_client, buffer, len, 0) == -1)
		{
			return false;
		}
		return true;
	}

private:
	SOCKET m_socket; //套接字
	SOCKET m_client; //客户端套接字
	CServerSocket& operator=(const CServerSocket&); //禁止赋值
	CServerSocket(const CServerSocket&); //禁止拷贝
	CServerSocket()
	{
		m_client = INVALID_SOCKET; //初始化套接字 -1
		if (InitSocketEnv() == FALSE)
		{
			MessageBox(NULL, L"InitSocketEnv failed", L"Error", MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_socket = socket(PF_INET, SOCK_STREAM, 0); //创建套接字
	} //构造函数

	~CServerSocket()
	{
		closesocket(m_socket); //关闭套接字
		WSACleanup(); //清理套接字
	} //析构函数

	BOOL InitSocketEnv()
	{
		WSADATA data; //套接字初始化结构体
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
		{
			return FALSE;
		}
		return TRUE;
	} //初始化套接字环境
	static CServerSocket* m_pInstance; //单例指针
	static void DestroyInstance()
	{
		if (m_pInstance != NULL)
		{
			delete m_pInstance;
			m_pInstance = NULL;
		}
	} //销毁单例

	class Chelper
	{
	public:
		Chelper()
		{
			CServerSocket::GetInstance(); //调用GetInstance
		}

		~Chelper()
		{
			CServerSocket::DestroyInstance(); //调用DestroyInstance
		}
	}; //静态变量初始化
	static Chelper m_helper; //静态变量
};

// extern CServerSocket* pserver; //全局变量
