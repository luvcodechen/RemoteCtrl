#pragma once
#include <map>
#include <MSWSock.h>

#include "MyQueue.h"
#include "MyThread.h"
#pragma warning(disable:4407)

enum Myoperator
{
	ENone,
	EAccept,
	ERecv,
	ESend,
	EError
};

class MyServer;
class MyClient;
typedef std::shared_ptr<MyClient> PCLIENT;

class COverlapped
{
public:
	OVERLAPPED m_Overlapped;
	DWORD m_operator; //操作类型
	std::vector<char> m_Buffer; //缓冲区
	ThreadWorker m_Worker; //处理函数
	MyServer* m_Server; //服务器对象
};

template <Myoperator>
class AcceptOverlapped;


typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;

class MyClient
{
public:
	MyClient();

	~MyClient()
	{
		closesocket(m_sock);
	}

	void SetOverlapped(PCLIENT& ptr);


	operator SOCKET() //类型转换
	{
		return m_sock;
	}

	operator PVOID()
	{
		return &m_Buffer[0]; //返回缓冲区首地址
	}

	operator LPOVERLAPPED();


	operator LPDWORD()
	{
		return &m_received; //返回接收到的字节数首地址
	}

	sockaddr_in* GetLocalAddr()
	{
		return &m_laddr;
	}

	sockaddr_in* GetRemoteAddr()
	{
		return &m_raddr;
	}

private:
	SOCKET m_sock;
	DWORD m_received;
	std::shared_ptr<ACCEPTOVERLAPPED> m_Overlapped;
	std::vector<char> m_Buffer;
	sockaddr_in m_laddr;
	sockaddr_in m_raddr;
	bool m_isbusy;
};


template <Myoperator>
class AcceptOverlapped : public COverlapped, ThreadFuncBase
{
public:
	AcceptOverlapped()
	{
		m_operator = EAccept;
		m_Worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped::AcceptWorker);
		memset(&m_Overlapped, 0, sizeof(m_Overlapped));
		m_Buffer.resize(1024);
		m_Server = NULL;
	}

	int AcceptWorker();


	PCLIENT m_Client;
};


template <Myoperator>
class RecvOverlapped : public COverlapped, ThreadFuncBase
{
public:
	RecvOverlapped()
	{
		m_operator = ERecv;
		m_Worker = ThreadWorker(this, &RecvOverlapped::RecvWorker);
		memset(&m_Overlapped, 0, sizeof(m_Overlapped));
		m_Buffer.resize(1024 * 256);
	}

	int RecvWorker()
	{
		//TODO:
	}
};

typedef RecvOverlapped<EAccept> RECVOVERLAPPED;

template <Myoperator>
class SendOverlapped : public COverlapped, ThreadFuncBase
{
public:
	SendOverlapped();

	int SendWorker()
	{
		//TODO:
	}
};

typedef SendOverlapped<ESend> SENDOVERLAPPED;

template <Myoperator>
class ErrorOverlapped : public COverlapped, ThreadFuncBase
{
public:
	ErrorOverlapped()
	{
		m_operator = EError;
		m_Worker = ThreadWorker(this, &ErrorOverlapped::ErrorWorker);
		memset(&m_Overlapped, 0, sizeof(m_Overlapped));
		m_Buffer.resize(1024);
	}

	int ErrorWorker()
	{
		//TODO:
	}
};

typedef ErrorOverlapped<EError> ERROROVERLAPPED;


class MyServer :
	public ThreadFuncBase
{
public:
	MyServer(const std::string& ip = "0.0.0.0", short port = 9527): m_Pool(10)
	{
		m_hIOCP = INVALID_HANDLE_VALUE;
		m_sock = INVALID_SOCKET;
		m_addr.sin_family = AF_INET;
		m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
		m_addr.sin_port = htons(port);
	}

	bool StartService()
	{
		CreateSocket();
		if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1)
		{
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return false;
		}
		if (listen(m_sock, 3) == -1)
		{
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return false;
		}
		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4); //创建IOCP
		if (m_hIOCP == NULL)
		{
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
		CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0); //绑定IOCP
		m_Pool.Invoke(); //启动线程池
		m_Pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&MyServer::threadIocp)); //启动线程
		if (!NewAccept())
		{
			return false;
		}
		return true;
	}

	~MyServer()
	{
	}

	bool NewAccept()
	{
		PCLIENT pClient(new MyClient());
		pClient->SetOverlapped(pClient);
		// m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));

		if (FALSE == AcceptEx(m_sock, *pClient, *pClient, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		                      *pClient, *pClient)) //接受连接
		{
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
		return true;
	}

private:
	void CreateSocket()
	{
		m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED); //创建套接字TCP
		int opt = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)); //设置端口复用
	}


	int threadIocp()
	{
		DWORD transferred = 0;
		ULONG_PTR Completionkey = 0;
		OVERLAPPED* lpOverlapped = NULL;
		if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &Completionkey, &lpOverlapped, INFINITE))
		{
			if (transferred > 0 && Completionkey != 0)
			{
				COverlapped* p_overlapped = CONTAINING_RECORD(lpOverlapped, COverlapped, m_Overlapped);
				switch (p_overlapped->m_operator)
				{
				case EAccept:
					{
						ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)(p_overlapped);
						m_Pool.DispatchWorker(pOver->m_Worker); //启动线程
					}
					break;
				case ERecv:
					{
						RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)(p_overlapped);
						m_Pool.DispatchWorker(pOver->m_Worker); //启动线程
					}
					break;
				case ESend:
					{
						SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)(p_overlapped);
						m_Pool.DispatchWorker(pOver->m_Worker); //启动线程
					}
					break;
				case EError:
					{
						ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)(p_overlapped);
						m_Pool.DispatchWorker(pOver->m_Worker); //启动线程
					}
					break;
				}
			}
			else
			{
				return -1;
			}
		}
		return 0;
	}

private:
	ThreadPool m_Pool;
	HANDLE m_hIOCP;
	SOCKET m_sock;
	sockaddr_in m_addr;
	std::map<SOCKET, std::shared_ptr<MyClient*>> m_client;
};
