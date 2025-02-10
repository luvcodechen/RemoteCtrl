#include "pch.h"
#include "MyServer.h"
#pragma warning(disable:4407)
template <Myoperator op>
int AcceptOverlapped<op>::AcceptWorker()
{
	INT lLength = 0, rLength = 0;
	if (*(LPDWORD)*m_Client.get() > 0)
	{
		GetAcceptExSockaddrs(*m_Client, 0,
		                     sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		                     (sockaddr**)m_Client->GetLocalAddr(), &lLength,
		                     (sockaddr**)m_Client->GetRemoteAddr(), &rLength);

		int ret = WSARecv((SOCKET)*m_Client, m_Client->RecvWSABuffer(), 1, *m_Client, &m_Client->flags(),
		                  *m_Client,NULL);
		if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			//TODO:报错
		}
		if (!m_Server->NewAccept()) //接受连接
		{
			return -2;
		}
	}
	return -1;
}

MyClient::MyClient()
	: m_isbusy(false)
	  , m_Overlapped(new ACCEPTOVERLAPPED()), m_flags(0)
	  , m_recv(new RECVOVERLAPPED())
	  , m_send(new SENDOVERLAPPED())
{
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_Buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(m_laddr));
	memset(&m_raddr, 0, sizeof(m_raddr));
}

void MyClient::SetOverlapped(PCLIENT& ptr)
{
	m_Overlapped->m_Client = ptr; //设置重叠结构
	m_recv->m_Client = ptr; //设置重叠结构
	m_send->m_Client = ptr; //设置重叠结构
}

MyClient::operator LPOVERLAPPED()
{
	return &m_Overlapped->m_Overlapped; //返回重叠结构首地址
}

LPWSABUF MyClient::RecvWSABuffer()
{
	return &m_recv->m_wsabuffer;
}

LPWSABUF MyClient::SendWSABuffer()
{
	return &m_send->m_wsabuffer;
}

template <Myoperator op>
SendOverlapped<op>::SendOverlapped()
{
	m_operator = ESend;
	m_Worker = ThreadWorker(this, (FUNCTYPE)&SendOverlapped::SendWorker);
	memset(&m_Overlapped, 0, sizeof(m_Overlapped));
	m_Buffer.resize(1024 * 256);
}

bool MyServer::StartService()
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

int MyServer::threadIocp()
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
