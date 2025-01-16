#include "pch.h"
#include "ClientSocket.h"
// CClientSocket server;
CClientSocket* CClientSocket::m_pInstance = NULL; //
CClientSocket* pclient = CClientSocket::GetInstance(); //
CClientSocket::Chelper CClientSocket::m_helper; // = CClientSocket::Chelper();

BOOL CClientSocket::InitSocket()
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
	TRACE("addr %08X nIP %08X\r\n", inet_addr("127.0.0.1"), m_nIP);
	server_addr.sin_addr.s_addr = htonl(m_nIP);
	server_addr.sin_port = htons(m_nPort);

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

bool CClientSocket::SendPacket(const CPacket& packet, std::list<CPacket>& lstPacks, bool isAutoClosed)
{
	if (m_socket == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE)
	{
		// if (InitSocket() == false)return false;
		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);
		TRACE("start thread\r\n");
	}
	m_lock.lock(); //加锁

	auto pr = m_mapAck.insert({packet.hEvent, lstPacks});
	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(packet.hEvent, isAutoClosed));

	m_listSend.push_back(packet); //发送
	m_lock.unlock(); //解锁
	TRACE("cmd:%d event %08X thread id %d\r\n", packet.sCmd, packet.hEvent, GetCurrentThreadId());
	WaitForSingleObject(packet.hEvent, INFINITE);
	TRACE("cmd:%d event %08X thread id %d\r\n", packet.sCmd, packet.hEvent, GetCurrentThreadId());
	std::map<HANDLE, std::list<CPacket>&>::iterator it = m_mapAck.find(packet.hEvent); //查找事件
	if (it != m_mapAck.end())
	{
		m_lock.lock();
		m_mapAck.erase(it); //删除事件
		m_lock.unlock();
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
			m_lock.lock();
			CPacket& head = m_listSend.front();
			m_lock.unlock();
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
					TRACE("recv %d %d \r\n", length, index);
					if (length > 0 || (index > 0))
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
								break; 
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
			m_lock.lock();
			m_listSend.pop_front(); //
			m_lock.unlock();
			InitSocket();
		}
		Sleep(1);
	}
	CloseSocket();
}
