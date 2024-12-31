#include "pch.h"
#include "ClientSocket.h"
// CClientSocket server;
CClientSocket* CClientSocket::m_pInstance = NULL;//
CClientSocket* pclient = CClientSocket::GetInstance();//
CClientSocket::Chelper CClientSocket::m_helper;// = CClientSocket::Chelper();