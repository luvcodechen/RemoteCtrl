#pragma once
#include "pch.h"
#include <string>
#include <vector>

#include "framework.h"

#pragma pack(push)
#pragma pack(1)
class CPacket
{
public:
	CPacket() : sHead(0), nLength(0), sCmd(0), sSUM(0)
	{
	}

	CPacket(WORD nCmd, const BYTE* pDData, size_t nSize)
	{
		sHead = 0xFEFF;
		nLength = nSize + 4; //包长
		sCmd = nCmd; //命令
		if (nSize > 0)
		{
			strData.resize(nSize); //数据
			memcpy((void*)strData.c_str(), pDData, nSize); //数据
		}
		else
		{
			strData.clear();
		}
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


typedef struct MouseEvent
{
	MouseEvent()
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}

	WORD nAction; //点击，移动，双击
	WORD nButton; //左键，右键、中键
	POINT ptXY; //鼠标坐标
} MOUSEEV, *PMOUSEEV;


inline std::string GetErrInfo(int wsaErrCode) //由于该函数所在头文件被多个文件包含，所以设置内联或者将定义放在cpp文件中
{
	std::string strError;
	LPVOID lpMsgBuf = NULL; //接收错误信息
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
	              NULL,
	              wsaErrCode,
	              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	              (LPTSTR)&lpMsgBuf, 0,NULL);
	//格式化错误信息
	strError = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf); //释放内存
	return strError;
}

class CClientSocket
{
public:
	static CClientSocket* GetInstance()
	{
		if (m_pInstance == NULL)
		{
			m_pInstance = new CClientSocket();
		}
		return m_pInstance;
	} //获取单例
	BOOL InitSocket(const std::string& strIPAddress)
	{
		if (m_socket != INVALID_SOCKET)
			CloseSocket();

		m_socket = socket(PF_INET, SOCK_STREAM, 0); //创建套接字
		if (m_socket == -1)
		{
			return FALSE;
		}

		sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = inet_addr(strIPAddress.c_str());
		server_addr.sin_port = htons(9527);

		if (server_addr.sin_addr.s_addr == INADDR_NONE)
		{
			AfxMessageBox("指定的ip地址不存在"); //ip地址不存在
			return FALSE;
		}
		int ret = connect(m_socket, (sockaddr*)&server_addr, sizeof(server_addr));
		if (ret == -1)
		{
			AfxMessageBox("连接失败"); //连接失败
			TRACE("连接失败 %d %s\n", WSAGetLastError(),
			      GetErrInfo(WSAGetLastError()).c_str()); //输出错误信息
			return FALSE;
		}
		return TRUE;
	}

#define BUFFER_SIZE 4096

	int DealCommand()
	{
		if (m_socket == -1)
		{
			return -1;
		}
		// char buffer[1024] = ""; //缓冲区
		char* buffer = m_buffer.data();
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (1)
		{
			size_t len = recv(m_socket, buffer + index, BUFFER_SIZE - index, 0); //接收数据
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
		if (send(m_socket, buffer, len, 0) == -1)
		{
			return false;
		}
		return true;
	}

	bool Send(CPacket& pack)
	{
		if (m_socket == -1)
		{
			return false;
		}
		return send(m_socket, pack.Data(), pack.Size(), 0) > 0;
	}

	bool GetFilePath(std::string& strPath) const
	{
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4))
		{
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}

	bool GetMouseEvent(MOUSEEV& mouse)
	{
		if (m_packet.sCmd == 5)
		{
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV)); //获取鼠标事件
			return true;
		}
		return false;
	}

	CPacket& GetPacket()
	{
		return m_packet;
	}

	void CloseSocket()
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}

private:
	std::vector<char> m_buffer; //缓冲区
	SOCKET m_socket; //套接字

	CPacket m_packet; //数据包
	CClientSocket& operator=(const CClientSocket&); //禁止赋值
	CClientSocket(const CClientSocket&); //禁止拷贝
	CClientSocket()
	{
		if (InitSocketEnv() == FALSE)
		{
			MessageBox(NULL, _T("InitSocketEnv failed"), _T("Error"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
	} //构造函数

	~CClientSocket()
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
	static CClientSocket* m_pInstance; //单例指针
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
			CClientSocket::GetInstance(); //调用GetInstance
		}

		~Chelper()
		{
			CClientSocket::DestroyInstance(); //调用DestroyInstance
		}
	}; //静态变量初始化
	static Chelper m_helper; //静态变量
};
