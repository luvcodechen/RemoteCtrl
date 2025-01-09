#pragma once
#include "pch.h"

#include <list>

#include "framework.h"

#include "Packet.h"


typedef void (*SOCKET_CALLBACK)(void* arg, int status, std::list<CPacket>&, CPacket&);

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


	int Run(SOCKET_CALLBACK callback, void* arg, short port = 9527)
	{
		// socket、bind,listen,accept,read,write,close

		//套接字初始化
		m_callback = callback;
		m_arg = arg;
		bool ret = InitSocket(port);
		if (ret == false)
		{
			return -1;
		}
		std::list<CPacket> lstPacket;
		int count = 0;
		while (true)
		{
			if (AcceptSocket() == false)
			{
				if (count >= 3)
				{
					return -2;
				}
				count++;
			}
			int cmd = DealCommand();
			if (cmd > 0)
			{
				m_callback(m_arg, cmd, lstPacket, m_packet);
				while (lstPacket.size() > 0)
				{
					Send(lstPacket.front()); //发送数据
					lstPacket.pop_front(); //删除数据
				}
			}
			CloseSocket();
		}
	}

protected:
	BOOL InitSocket(short port)
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
		TRACE("Enter AcceptSocket\r\n");
		sockaddr_in client_addr;
		int cli_sz = sizeof(client_addr);
		m_client = accept(m_socket, (sockaddr*)&client_addr, &cli_sz);
		TRACE("m_client =%d\r\n", m_client);
		if (m_client == -1)
			return FALSE;


		return TRUE;
	}

#define BUFFER_SIZE 4096

	int DealCommand()
	{
		if (m_client == -1)
		{
			return -1;
		}
		// char buffer[1024] = ""; //缓冲区
		char* buffer = new char[BUFFER_SIZE];
		if (buffer == NULL)
		{
			TRACE("内存不足\r\n");
			return -2;
		}
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (1)
		{
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0); //接收数据
			if (len <= 0)
			{
				delete[]buffer;
				return -1;
			}
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len); //解析数据
			if (len > 0)
			{
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				delete[]buffer;
				return m_packet.sCmd;
			}
		}
		delete[]buffer;
		return -1;
	}

	bool Send(const char* buffer, int len)
	{
		if (send(m_client, buffer, len, 0) == -1)
		{
			return false;
		}
		return true;
	}

	bool Send(CPacket& pack)
	{
		if (m_client == -1)
		{
			return false;
		}
		Sleep(10);
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}


	void CloseSocket()
	{
		if (m_client != INVALID_SOCKET)
		{
			closesocket(m_client);
			m_client = INVALID_SOCKET;
		}
	}

private:
	SOCKET_CALLBACK m_callback; //回调函数
	void* m_arg; //回调函数参数
	SOCKET m_socket; //套接字
	SOCKET m_client; //客户端套接字
	CPacket m_packet; //数据包
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
