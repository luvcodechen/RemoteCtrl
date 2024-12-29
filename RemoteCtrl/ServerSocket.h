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
		nLength = nSize + 4; //����
		sCmd = nCmd; //����
		strData.resize(nSize); //����
		memcpy((void*)strData.c_str(), pDData, nSize); //����
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
				i += 2; //������ͷ
				break;
			}
		} //�Ұ�ͷ
		if (i + 4 + 2 + 2 > nSize) //��ͷ+����+����+У���
		{
			nSize = 0; //û���ҵ���ͷ
			return;
		} //û���ҵ���ͷ���߰����ݲ�ȫ

		nLength = *(DWORD*)(pData + i); //����
		i += 4; //��������

		if (nLength + i > nSize) //��δ��ȫ���յ����ͷ��أ��ȴ��´ν���
		{
			nSize = 0;
			return;
		} //��δ����


		sCmd = *(WORD*)(pData + i); //����
		i += 2; //��������

		if (nLength > 4)
		{
			strData.resize(nLength - 2 - 2); //����-����-У���
			memcpy((void*)strData.c_str(), pData + i, nLength - 2 - 2); //������
		}

		sSUM = *(WORD*)(pData + i); //У���
		i += 2; //����У���
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) //У���
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSUM)
		{
			nSize = i; //head2 �����ĳ���4  ������������ݣ�У�飩
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

	int Size() //����С
	{
		return nLength + 6;
	}

	const char* Data() //������
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
	WORD sHead; //��ͷ �̶�ΪFE FF
	DWORD nLength; //����(�����У���)
	WORD sCmd; //����
	std::string strData; //������
	WORD sSUM; //У���
	std::string strOut; //������������
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
	} //��ȡ����
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
		// char buffer[1024] = ""; //������
		char* buffer = new char[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (1)
		{
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0); //��������
			if (len <= 0)
			{
				return -1;
			}
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len); //��������
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
	SOCKET m_socket; //�׽���
	SOCKET m_client; //�ͻ����׽���
	CPacket m_packet; //���ݰ�
	CServerSocket& operator=(const CServerSocket&); //��ֹ��ֵ
	CServerSocket(const CServerSocket&); //��ֹ����
	CServerSocket()
	{
		m_client = INVALID_SOCKET; //��ʼ���׽��� -1
		if (InitSocketEnv() == FALSE)
		{
			MessageBox(NULL, L"InitSocketEnv failed", L"Error", MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_socket = socket(PF_INET, SOCK_STREAM, 0); //�����׽���
	} //���캯��

	~CServerSocket()
	{
		closesocket(m_socket); //�ر��׽���
		WSACleanup(); //�����׽���
	} //��������

	BOOL InitSocketEnv()
	{
		WSADATA data; //�׽��ֳ�ʼ���ṹ��
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
		{
			return FALSE;
		}
		return TRUE;
	} //��ʼ���׽��ֻ���
	static CServerSocket* m_pInstance; //����ָ��
	static void DestroyInstance()
	{
		if (m_pInstance != NULL)
		{
			delete m_pInstance;
			m_pInstance = NULL;
		}
	} //���ٵ���

	class Chelper
	{
	public:
		Chelper()
		{
			CServerSocket::GetInstance(); //����GetInstance
		}

		~Chelper()
		{
			CServerSocket::DestroyInstance(); //����DestroyInstance
		}
	}; //��̬������ʼ��
	static Chelper m_helper; //��̬����
};

// extern CServerSocket* pserver; //ȫ�ֱ���
