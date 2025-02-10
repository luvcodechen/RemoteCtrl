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


		if (!m_Server->NewAccept()) //接受连接
		{
			return -2;
		}
	}
	return -1;
}

MyClient::MyClient() : m_isbusy(false), m_Overlapped(new ACCEPTOVERLAPPED())
{
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_Buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(m_laddr));
	memset(&m_raddr, 0, sizeof(m_raddr));
}

void MyClient::SetOverlapped(PCLIENT& ptr)
{
	m_Overlapped->m_Client = ptr; //设置重叠结构
}

MyClient::operator LPOVERLAPPED()
{
	return &m_Overlapped->m_Overlapped; //返回重叠结构首地址
}

template <Myoperator op>
SendOverlapped<op>::SendOverlapped()
{
	m_operator = ESend;
	m_Worker = ThreadWorker(this, &SendOverlapped::SendWorker);
	memset(&m_Overlapped, 0, sizeof(m_Overlapped));
	m_Buffer.resize(1024 * 256);
}
