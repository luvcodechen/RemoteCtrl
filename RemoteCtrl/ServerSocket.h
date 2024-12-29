#pragma once
#include "pch.h"
#include "framework.h"

#pragma pack(push)
#pragma pack(1)
class CPacket
{
public:
	CPacket(): sHead(0), nLength(0), sCmd(0), sSUM(0)
	{
	}

	CPacket(WORD nCmd, const BYTE* pDData, size_t nSize)
	{
		sHead = 0xFEFF;
		nLength = nSize + 4; //包长
		sCmd = nCmd; //命令
		strData.resize(nSize); //数据
		memcpy((void*)strData.c_str(), pDData, nSize); //数据
		sSUM = 0;
		for (size_t i = 0; i < strData.size(); i++)
		{
			sSUM += BYTE(strData[i]) & 0xFF;
		}
	}

	CPacket(const CPacket& packet)
	{
		sHead = packet.sHead;
		nLength = packet.nLength;
		sCmd = packet.sCmd;
		strData = packet.strData;
		sSUM = packet.sSUM;
	}

	CPacket(const BYTE* pData, size_t& nSize)
	{
		size_t i = 0;
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF)
			{
				sHead = *(WORD*)(pData + i);
				i += 2; //跳过包头
				break;
			}
		} //找包头
		if (i + 4 + 2 + 2 > nSize) //包头+包长+命令+校验和
		{
			nSize = 0; //没有找到包头
			return;
		} //没有找到包头或者包数据不全

		nLength = *(DWORD*)(pData + i); //包长
		i += 4; //跳过包长

		if (nLength + i > nSize) //包未完全接收到，就返回，等待下次接收
		{
			nSize = 0;
			return;
		} //包未完整


		sCmd = *(WORD*)(pData + i); //命令
		i += 2; //跳过命令

		if (nLength > 4)
		{
			strData.resize(nLength - 2 - 2); //包长-命令-校验和
			memcpy((void*)strData.c_str(), pData + i, nLength - 2 - 2); //包数据
		}

		sSUM = *(WORD*)(pData + i); //校验和
		i += 2; //跳过校验和
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) //校验和
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSUM)
		{
			nSize = i; //head2 包长的长度4  包长（命令，数据，校验）
			return;
		}
		nSize = 0;
	}

	~CPacket()
	{
	}

	CPacket& operator=(const CPacket& packet)
	{
		if (this == &packet)
		{
			return *this;
		}
		sHead = packet.sHead;
		nLength = packet.nLength;
		sCmd = packet.sCmd;
		strData = packet.strData;
		sSUM = packet.sSUM;
		return *this;
	}

	int Size() //包大小
	{
		return nLength + 6;
	}

	const char* Data() //包数据
	{
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead;
		pData += 2;
		*(DWORD*)pData = nLength;
		pData += 4;
		*(WORD*)pData = sCmd;
		pData += 2;
		memcpy(pData, strData.c_str(), strData.size());
		pData += strData.size();
		*(WORD*)pData = sSUM;
		return strOut.c_str();
	}

public:
	WORD sHead; //包头 固定为FE FF
	DWORD nLength; //包长(从命令到校验和)
	WORD sCmd; //命令
	std::string strData; //包数据
	WORD sSUM; //校验和
	std::string strOut; //整个包的数据
};
#pragma pack(pop)

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
		int cli_sz = sizeof(client_addr);
		m_client = accept(m_socket, (sockaddr*)&client_addr, &cli_sz);
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
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (1)
		{
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0); //接收数据
			if (len <= 0)
			{
				return -1;
			}
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len); //解析数据
			if (len > 0)
			{
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
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
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}

private:
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
