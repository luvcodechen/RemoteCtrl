#pragma once
#include "pch.h"
#include "framework.h"

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
		char buffer[1024]; //������
		int cli_sz = sizeof(client_addr);
		m_client = accept(m_socket, (sockaddr*)&client_addr, &cli_sz);
		if (m_client == -1)
		{
			return FALSE;
		}
		return TRUE;
	}

	int DealCommand()
	{
		if (m_client == -1)
		{
			return -1;
		}
		char buffer[1024] = ""; //������
		while (1)
		{
			memset(buffer, 0, sizeof(buffer));
			int len = recv(m_client, buffer, sizeof(buffer), 0); //��������
			if (len <= 0)
			{
				return -1;
			}
			//TODO:������յ�������
		}
	}

	bool Send(const char* buffer, int len)
	{
		if (send(m_client, buffer, len, 0) == -1)
		{
			return false;
		}
		return true;
	}

private:
	SOCKET m_socket; //�׽���
	SOCKET m_client; //�ͻ����׽���
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
