#include "pch.h"
#include "MyNetwork.h"

#include <conio.h>
#include <list>

MynewServer::MynewServer(const MyServerParameter& param): m_stop(false), m_args(NULL)
{
	m_params = param;
	m_thread.UpdateWorker(ThreadWorker(this, (FUNCTYPE)&MynewServer::threadFunc));
}

MynewServer::~MynewServer()
{
	Stop();
}

int MynewServer::Invoke(void* arg)
{
	m_sock.reset(new MySocket(m_params.m_type));
	if (*m_sock == INVALID_SOCKET)
	{
		printf("%s(%d):%s error!%d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
		return -1;
	}
	if (m_params.m_type == SocketType::TypeTCP)
	{
		if (m_sock->listen() == -1)
		{
			return -1;
		}
	}
	MySockaddrIn client;
	if (-1 == m_sock->bind(m_params.m_ip, m_params.m_port))
	{
		printf("%s(%d):%s error!%d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
		return -3;
	}

	if (m_thread.Start() == false)
	{
		return -4;
	}
	m_args = arg;
	return 0;
}

int MynewServer::send(MYSOCKET& client, const MyBuffer& buffer)
{
	int ret = m_sock->send(buffer); //TODO:待优化，发送虽然成功，但是不完整
	if (m_params.m_send)
		m_params.m_send(m_args,client,ret);
	return ret;
}

int MynewServer::Sendto(MySockaddrIn& addr, const MyBuffer& buffer)
{
	int ret = m_sock->sendto(buffer, addr); //TODO:待优化，发送虽然成功，但是不完整
	if (m_params.m_sendto)
		m_params.m_sendto(m_args,addr,ret);
	return ret;
}

int MynewServer::Stop()
{
	if (m_stop == false)
	{
		m_sock->close();
		m_stop = true;
		m_thread.Stop();
	}

	return 0;
}

int MynewServer::threadFunc()
{
	if (m_params.m_type == SocketType::TypeTCP)
	{
		return threadTCPFunc();
	}
	else
	{
		return threadUDPFunc();
	}
}

int MynewServer::threadUDPFunc()
{
	MyBuffer buf(1024 * 256);
	MySockaddrIn client;
	int ret = 0;
	while (!m_stop)
	{
		ret = m_sock->recvfrom(buf, client);
		if (ret > 0)
		{
			client.update();
			if (m_params.m_recvfrom)
				m_params.m_recvfrom(m_args, buf, client);
		}
		else
		{
			printf("%s(%d):%s error!%d ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			break;
		}
		// Sleep(1);
	}
	if (m_stop == false)m_stop = true;
	m_sock->close();
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	return 0;
}

int MynewServer::threadTCPFunc()
{
	return 0;
}

MyServerParameter::MyServerParameter(const std::string& ip, short port, SocketType type, AcceptFunc accept,
                                     RecvFunc recv, SendFunc send, RecvFromFunc recvfrom, SendToFunc sendto)
{
	m_ip = ip;
	m_port = port;
	m_type = type;
	m_accept = accept;
	m_recv = recv;
	m_send = send;
	m_recvfrom = recvfrom;
	m_sendto = sendto;
}

MyServerParameter& MyServerParameter::operator<<(AcceptFunc func)
{
	// TODO: 在此处插入 return 语句
	m_accept = func;
	return *this;
}

MyServerParameter& MyServerParameter::operator<<(RecvFunc func)
{
	// TODO: 在此处插入 return 语句
	m_recv = func;
	return *this;
}

MyServerParameter& MyServerParameter::operator<<(SendFunc func)
{
	// TODO: 在此处插入 return 语句
	m_send = func;
	return *this;
}

MyServerParameter& MyServerParameter::operator<<(RecvFromFunc func)
{
	// TODO: 在此处插入 return 语句
	m_recvfrom = func;
	return *this;
}

MyServerParameter& MyServerParameter::operator<<(std::string& ip)
{
	// TODO: 在此处插入 return 语句
	m_ip = ip;
	return *this;
}

MyServerParameter& MyServerParameter::operator<<(short port)
{
	// TODO: 在此处插入 return 语句
	m_port = port;
	return *this;
}

MyServerParameter& MyServerParameter::operator<<(SocketType type)
{
	// TODO: 在此处插入 return 语句
	m_type = type;
	return *this;
}

MyServerParameter& MyServerParameter::operator>>(AcceptFunc& func)
{
	// TODO: 在此处插入 return 语句
	func = m_accept;
	return *this;
}

MyServerParameter& MyServerParameter::operator>>(RecvFunc& func)
{
	// TODO: 在此处插入 return 语句
	func = m_recv;
	return *this;
}

MyServerParameter& MyServerParameter::operator>>(SendFunc& func)
{
	// TODO: 在此处插入 return 语句
	func = m_send;
	return *this;
}

MyServerParameter& MyServerParameter::operator>>(RecvFromFunc func)
{
	func = m_recvfrom;
	return *this;
}

MyServerParameter& MyServerParameter::operator>>(SendToFunc func)
{
	func = m_sendto;
	return *this;
}

MyServerParameter& MyServerParameter::operator>>(std::string& ip)
{
	ip = m_ip;
	return *this;
}

MyServerParameter& MyServerParameter::operator>>(short& port)
{
	port = m_port;
	return *this;
}

MyServerParameter& MyServerParameter::operator>>(SocketType& type)
{
	type = m_type;
	return *this;
}

MyServerParameter::MyServerParameter(const MyServerParameter& param)
{
	m_ip = param.m_ip;
	m_port = param.m_port;
	m_type = param.m_type;
	m_accept = param.m_accept;
	m_recv = param.m_recv;
	m_send = param.m_send;
	m_recvfrom = param.m_recvfrom;
	m_sendto = param.m_sendto;
}

MyServerParameter& MyServerParameter::operator=(const MyServerParameter& param)
{
	if (this != &param)
	{
		m_ip = param.m_ip;
		m_port = param.m_port;
		m_type = param.m_type;
		m_accept = param.m_accept;
		m_recv = param.m_recv;
		m_send = param.m_send;
		m_recvfrom = param.m_recvfrom;
		m_sendto = param.m_sendto;
	}
	return *this;
}

MyServerParameter& MyServerParameter::operator<<(SendToFunc func)
{
	m_sendto = func;
	return *this;
}
