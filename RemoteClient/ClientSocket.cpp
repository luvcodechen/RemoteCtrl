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

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed, WPARAM wParam)
{
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);
	bool ret = PostThreadMessage(m_nThreadId,WM_SEND_PACK,
	                             (WPARAM)new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam),
	                             (LPARAM)hWnd);
	return ret;
}

//
// bool CClientSocket::SendPacket(const CPacket& packet, std::list<CPacket>& lstPacks, bool isAutoClosed)
// {
// 	if (m_socket == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE)
// 	{
// 		// if (InitSocket() == false)return false;
// 		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);
// 		TRACE("start thread\r\n");
// 	}
// 	m_lock.lock(); //加锁
// 	auto pr = m_mapAck.insert({packet.hEvent, lstPacks});
// 	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(packet.hEvent, isAutoClosed));
//
// 	m_listSend.push_back(packet); //发送
// 	m_lock.unlock(); //解锁
// 	TRACE("cmd:%d event %08X thread id %d\r\n", packet.sCmd, packet.hEvent, GetCurrentThreadId());
// 	WaitForSingleObject(packet.hEvent, INFINITE);
// 	TRACE("cmd:%d event %08X thread id %d\r\n", packet.sCmd, packet.hEvent, GetCurrentThreadId());
// 	std::map<HANDLE, std::list<CPacket>&>::iterator it = m_mapAck.find(packet.hEvent); //查找事件
// 	if (it != m_mapAck.end())
// 	{
// 		m_lock.lock();
// 		m_mapAck.erase(it); //删除事件
// 		m_lock.unlock();
// 		return true;
// 	}
// 	return false;
// }

void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	//TODO:定义一个消息的数据结构(数据和数据长度，模式)，回调函数的数据结构（HWND），
	PACKET_DATA data = *(PACKET_DATA*)wParam;
	delete (PACKET_DATA*)wParam; //释放内存
	HWND hWnd = (HWND)lParam; //获取窗口句柄
	if (InitSocket() == TRUE)
	{
		int ret = send(m_socket, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if (ret > 0)
		{
			size_t index = 0;
			std::string strBuffer;
			strBuffer.resize(BUFFER_SIZE);
			char* pBuffer = (char*)strBuffer.c_str();
			while (m_socket != INVALID_SOCKET)
			{
				int length = recv(m_socket, pBuffer + index,BUFFER_SIZE - index, 0); //接收数据
				if (length > 0 || index > 0)
				{
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBuffer, nLen);
					if (nLen > 0)
					{
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);
						if (data.nMOde & CSM_AUTOCLOSE)
						{
							CloseSocket();
							return;
						}
					}
					index -= nLen;
					memmove(pBuffer, pBuffer + index, nLen); //移动数据
				}
				else //TODO:对方关闭了连接或网络异常
				{
					CloseSocket();
					::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, 1);
				}
			}
		}
		else
		{
			CloseSocket();
			//网络终止处理
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1);
		}
	}
	else
	{
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, NULL);
	}
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

unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;
}

// void CClientSocket::threadFunc()
// {
// 	std::string strBuffer;
// 	strBuffer.resize(BUFFER_SIZE);
// 	char* pBuffer = (char*)strBuffer.c_str();
// 	int index = 0;
// 	InitSocket();
// 	while (m_socket != INVALID_SOCKET)
// 	{
// 		if (m_listSend.size() > 0)
// 		{
// 			TRACE("m_listSend.size:%d\r\n", m_listSend.size());
// 			m_lock.lock();
// 			CPacket& head = m_listSend.front();
// 			m_lock.unlock();
// 			if (Send(head) == false)
// 			{
// 				TRACE("发送失败\r\n");
//
// 				continue;
// 			}
// 			std::map<HANDLE, std::list<CPacket>&>::iterator it = m_mapAck.find(head.hEvent); //查找事件
// 			if (it != m_mapAck.end())
// 			{
// 				// std::list<CPacket> lstRecv;
// 				std::map<HANDLE, bool>::iterator itAutoClosed = m_mapAutoClosed.find(head.hEvent);
// 				do
// 				{
// 					int length = recv(m_socket, pBuffer + index, BUFFER_SIZE - index, 0);
// 					TRACE("recv %d %d \r\n", length, index);
// 					if (length > 0 || (index > 0))
// 					{
// 						index += length;
// 						size_t size = (size_t)index;
// 						CPacket pack((BYTE*)pBuffer, size);
//
// 						if (size > 0)
// 						{
// 							//TODO:通知对应的事件
// 							pack.hEvent = head.hEvent;
// 							it->second.push_back(pack);
// 							memmove(pBuffer, pBuffer + size, index - size);
// 							index -= size;
// 							if (itAutoClosed->second == true)
// 							{
// 								SetEvent(head.hEvent);
// 								break;
// 							}
// 						}
// 					}
// 					else if (length <= 0 && index <= 0)
// 					{
// 						CloseSocket();
// 						SetEvent(head.hEvent); //等到服务器关闭再通知事件完成
// 						if (itAutoClosed != m_mapAutoClosed.end())
// 						{
// 							TRACE("SetEvent %d %d \r\n", head.sCmd, itAutoClosed->second);
// 						}
// 						else
// 						{
// 							TRACE("异常得情况，么有对应的pair\r\n");
// 						}
// 						break;
// 					}
// 				}
// 				while (itAutoClosed->second == false);
// 			}
// 			m_lock.lock();
// 			m_listSend.pop_front(); //
// 			m_mapAutoClosed.erase(head.hEvent);
//
// 			m_lock.unlock();
// 			InitSocket();
// 		}
// 		Sleep(1);
// 	}
// 	CloseSocket();
// }

void CClientSocket::threadFunc2()
{
	SetEvent(m_eventInvoke);
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end())
		{
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam); //调用函数
		}
	}
}
