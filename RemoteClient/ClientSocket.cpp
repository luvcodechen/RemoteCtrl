#include "pch.h"
#include "ClientSocket.h"
// CClientSocket server;
CClientSocket* CClientSocket::m_pInstance = NULL; //
CClientSocket* pclient = CClientSocket::GetInstance(); //
CClientSocket::Chelper CClientSocket::m_helper; // = CClientSocket::Chelper();

bool CClientSocket::Send(const CPacket& pack)
{
	if (m_socket == -1)
	{
		return false;
	}
	std::string strOut;
	pack.Data(strOut);
	return send(m_socket, strOut.c_str(), strOut.size(), 0) > 0;
}

void CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc();
}

void CClientSocket::threadFunc()
{
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0;
	while (m_socket != INVALID_SOCKET)
	{
		if (m_listSend.size() > 0)
		{
			TRACE("m_listSend.size:%d\r\n", m_listSend.size());
			CPacket& head = m_listSend.front();
			if (Send(head) == false)
			{
				TRACE("发送失败\r\n");
				continue;
			}
			auto pr = m_mapAck.insert({head.hEvent, std::list<CPacket>()});
			std::list<CPacket> lstRecv;
			int length = recv(m_socket, pBuffer + index, BUFFER_SIZE - index, 0);
			if (length > 0 || index > 0)
			{
				index += length;
				size_t size = (size_t)index;
				CPacket pack((BYTE*)pBuffer, size);

				if (size > 0)
				{
					//TODO:通知对应的事件
					pack.hEvent = head.hEvent;
					pr.first->second.push_back(pack);
					SetEvent(head.hEvent); //通知发送成功
				}
			}
			else if (length <= 0 && index <= 0)
			{
				CloseSocket();
			}
			m_listSend.pop_front(); //
		}
	}
	CloseSocket();
}
