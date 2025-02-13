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
	MyClient* m_Client; //客户端对象
	WSABUF m_wsabuffer; //WSA缓冲区
	virtual ~COverlapped()
	{
		m_Buffer.clear();
	}
};

template <Myoperator>
class AcceptOverlapped;


typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;

template <Myoperator>
class RecvOverlapped;
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;

template <Myoperator>
class SendOverlapped;
typedef SendOverlapped<ESend> SENDOVERLAPPED;

class MyClient : public ThreadFuncBase
{
public:
	MyClient();

	~MyClient()
	{
		m_Buffer.clear(); //
		closesocket(m_sock);
		m_recv.reset(); //释放接收重叠结构
		m_send.reset(); //释放发送重叠结构
		m_Overlapped.reset(); //释放接受重叠结构
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

	LPWSABUF RecvWSABuffer();


	LPWSABUF SendWSABuffer();


	DWORD& flags()
	{
		return m_flags;
	}

	sockaddr_in* GetLocalAddr()
	{
		return &m_laddr;
	}

	sockaddr_in* GetRemoteAddr()
	{
		return &m_raddr;
	}

	size_t GetBufferSize()
	{
		return m_Buffer.size();
	}

	int Recv();

	int Send(void* buffer, size_t nSize);
	int SendData(std::vector<char>& data);

private:
	SOCKET m_sock;
	DWORD m_received;
	DWORD m_flags;
	std::shared_ptr<ACCEPTOVERLAPPED> m_Overlapped;
	std::shared_ptr<RECVOVERLAPPED> m_recv; //接收重叠结构
	std::shared_ptr<SENDOVERLAPPED> m_send; //发送重叠结构
	std::vector<char> m_Buffer;
	size_t m_used; //已使用的缓冲区大小
	sockaddr_in m_laddr;
	sockaddr_in m_raddr;
	bool m_isbusy;
	SendQueue<std::vector<char>> m_vecSend; //发送数据队列
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
};


template <Myoperator>
class RecvOverlapped : public COverlapped, ThreadFuncBase
{
public:
	RecvOverlapped()
	{
		m_operator = ERecv;
		m_Worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped::RecvWorker);
		memset(&m_Overlapped, 0, sizeof(m_Overlapped));
		m_Buffer.resize(1024 * 256);
	}

	int RecvWorker()
	{
		int ret = m_Client->Recv();
		return ret;
	}
};


template <Myoperator>
class SendOverlapped : public COverlapped, ThreadFuncBase
{
public:
	SendOverlapped();

	int SendWorker()
	{
		//TODO:
		/*
		 * 1 send可能不会立即完成
		 */
		return 1;
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
		return -1;
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

	bool StartService(); //启动服务


	~MyServer();

	bool NewAccept()
	{
		PCLIENT pClient(new MyClient());
		pClient->SetOverlapped(pClient);
		m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));

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


	int threadIocp();

private:
	ThreadPool m_Pool;
	HANDLE m_hIOCP;
	SOCKET m_sock;
	sockaddr_in m_addr;
	std::map<SOCKET, std::shared_ptr<MyClient>> m_client;
};
