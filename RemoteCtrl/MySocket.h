#pragma once
#include <WinSock2.h>

enum SocketType
{
	TypeTCP = 1,
	TypeUDP
};

class MySockaddrIn
{
public:
	MySockaddrIn()
	{
		memset(&m_addr, 0, sizeof(m_addr));
		m_port = -1;
	}

	MySockaddrIn(sockaddr_in addr)
	{
		memcpy(&m_addr, &addr, sizeof(addr));
		m_addr = addr;
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}

	MySockaddrIn(UINT nIP, short nPort)
	{
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.s_addr = htonl(nIP);
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = nPort;
	}

	MySockaddrIn(const std::string& strIP, short nPort)
	{
		m_ip = strIP;
		m_port = nPort;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.s_addr = inet_addr(strIP.c_str());
	}

	MySockaddrIn(const MySockaddrIn& addr)
	{
		memcpy(&m_addr, &addr.m_addr, sizeof(addr.m_addr));
		m_ip = addr.m_ip;
		m_port = addr.m_port;
	}

	MySockaddrIn& operator=(const MySockaddrIn& addr)
	{
		if (this != &addr)
		{
			memcpy(&m_addr, &addr.m_addr, sizeof(addr.m_addr));
			m_ip = addr.m_ip;
			m_port = addr.m_port;
		}
		return *this;
	}

	operator sockaddr*() const
	{
		return (sockaddr*)&m_addr;
	}

	operator void*() const
	{
		return (void*)&m_addr;
	}

	void update()
	{
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}

	std::string GetIP() const
	{
		return m_ip;
	}

	short GetPort() const
	{
		return m_port;
	}

	inline int size() const
	{
		return sizeof(sockaddr_in);
	}

private:
	sockaddr_in m_addr;
	std::string m_ip;
	short m_port;
};

class MyBuffer : public std::string
{
public:
	MyBuffer(const char* str)
	{
		resize(strlen(str));
		memcpy((void*)c_str(), str, size());
	}

	MyBuffer(size_t size = 0) : std::string()
	{
		if (size > 0)
		{
			resize(size);
			memset(*this, 0, size);
		}
	}

	MyBuffer(void* buffer, size_t size): std::string()
	{
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}

	~MyBuffer()
	{
		std::string::~basic_string();
	}

	operator char*() const
	{
		return (char*)c_str();
	}

	operator const char*() const
	{
		return c_str();
	}

	operator BYTE*() const
	{
		return (BYTE*)c_str();
	}

	operator void*() const
	{
		return (void*)c_str();
	}

	void Update(void* buffer, size_t size)
	{
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}
};

class MySocket
{
public:
	MySocket(SocketType nType = TypeTCP, int nProtocol = 0)
	{
		m_socket = socket(AF_INET, nType, nProtocol);
		m_type = nType;
		m_protocol = nProtocol;
	}

	MySocket(const MySocket& s)
	{
		m_socket = socket(AF_INET, (int)s.m_type, s.m_protocol);
		m_type = s.m_type;
		m_protocol = s.m_protocol;
		m_addr = s.m_addr;
	}

	~MySocket()
	{
		close();
	}

	operator SOCKET() const
	{
		return m_socket;
	}

	operator SOCKET()
	{
		return m_socket;
	}

	MySocket& operator=(const MySocket& s)
	{
		if (this != &s)
		{
			m_socket = socket(AF_INET, (int)s.m_type, s.m_protocol);
			m_type = s.m_type;
			m_protocol = s.m_protocol;
			m_addr = s.m_addr;
		}
		return *this;
	}

	bool operator==(SOCKET sock) const
	{
		return m_socket == sock;
	}

	int listen(int backlog = 5)
	{
		if (m_type != TypeTCP)
			return -1;
		return ::listen(m_socket, backlog);
	}

	int bind(const std::string& strIP, short nPort)
	{
		m_addr = MySockaddrIn(strIP, nPort);
		return ::bind(m_socket, m_addr, m_addr.size());
	}

	int accept()
	{
	}

	int connect(const std::string& ip, short port)
	{
	}

	int send(const MyBuffer& buffer)
	{
		return ::send(m_socket, buffer, buffer.size(), 0);
	}

	int recv(MyBuffer& buffer)
	{
		return ::recv(m_socket, buffer, buffer.size(), 0);
	}

	int sendto(const MyBuffer& buffer, const MySockaddrIn& to)
	{
		return ::sendto(m_socket, buffer, buffer.size(), 0, to, to.size());
	}

	int recvfrom(MyBuffer& buffer, MySockaddrIn& from)
	{
		int len = from.size();
		int ret = ::recvfrom(m_socket, buffer, buffer.size(), 0, from, &len);
		if (ret > 0)
			from.update();
		return ret;
	}

	void close()
	{
		if (m_socket != INVALID_SOCKET)
		{
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}
	}

private:
	SOCKET m_socket;
	SocketType m_type;
	int m_protocol;
	MySockaddrIn m_addr;
};

typedef std::shared_ptr<MySocket> MYSOCKET;
