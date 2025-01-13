#include "pch.h"
#include "ClientSocket.h"
// CClientSocket server;
CClientSocket* CClientSocket::m_pInstance = NULL; //
CClientSocket* pclient = CClientSocket::GetInstance(); //
CClientSocket::Chelper CClientSocket::m_helper; // = CClientSocket::Chelper();

bool CClientSocket::SendPacket(const CPacket& packet, std::list<CPacket>& lstPacks, bool isAutoClosed)
{
	if (m_socket == INVALID_SOCKET)
	{
		// if (InitSocket() == false)return false;
		_beginthread(&CClientSocket::threadEntry, 0, this);
	}
	auto pr = m_mapAck.insert({packet.hEvent, lstPacks});
	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(packet.hEvent, isAutoClosed));
	m_listSend.push_back(packet); //发送
	WaitForSingleObject(packet.hEvent, INFINITE);
	std::map<HANDLE, std::list<CPacket>&>::iterator it = m_mapAck.find(packet.hEvent); //查找事件
	if (it != m_mapAck.end())
	{
		m_mapAck.erase(it); //删除事件
		return true;
	}
	return false;
}

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
	InitSocket();
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
			std::map<HANDLE, std::list<CPacket>&>::iterator it = m_mapAck.find(head.hEvent); //查找事件
			if (it != m_mapAck.end())
			{
				// std::list<CPacket> lstRecv;
				std::map<HANDLE, bool>::iterator itAutoClosed = m_mapAutoClosed.find(head.hEvent);
				do
				{
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
							it->second.push_back(pack);
							memmove(pBuffer, pBuffer + size, index - size);
							index -= size;
							if (itAutoClosed->second == true)
							{
								SetEvent(head.hEvent);
							}
						}
					}
					else if (length <= 0 && index <= 0)
					{
						CloseSocket();
						SetEvent(head.hEvent); //等到服务器关闭再通知事件完成
						m_mapAutoClosed.erase(itAutoClosed);
						break;
					}
				}
				while (itAutoClosed->second == false);
			}

			m_listSend.pop_front(); //
			InitSocket();
		}
	}
	CloseSocket();
}
