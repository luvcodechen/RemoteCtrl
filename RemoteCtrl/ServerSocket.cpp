#include "pch.h"
#include "ServerSocket.h"

// CServerSocket server;
CServerSocket* CServerSocket::m_pInstance = NULL;//
CServerSocket* pserver = CServerSocket::GetInstance();//
CServerSocket::Chelper CServerSocket::m_helper;// = CServerSocket::Chelper();