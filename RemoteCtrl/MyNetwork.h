#pragma once
#include "MySocket.h"
#include "MyThread.h"
/*
 * 1 核心功能到底是什么
 * 2 业务逻辑是什么
 */
class MyNetwork
{
};

typedef int (*AcceptFunc)(void* arg, MySocket& client);
typedef int (*RecvFunc)(void* arg, const MyBuffer& buffer);
typedef int (*SendFunc)(void* arg, MYSOCKET& client, int ret);
typedef int (*RecvFromFunc)(void* arg, const MyBuffer& buffer, MySockaddrIn& addr);
typedef int (*SendToFunc)(void* arg, const MySockaddrIn& addr, int ret);

class MyServerParameter
{
public:
	MyServerParameter(const std::string& ip = "0.0.0.0", short port = 9527, SocketType type = SocketType::TypeTCP,
	                  AcceptFunc accept = NULL, RecvFunc recv = NULL,
	                  SendFunc send = NULL, RecvFromFunc recvfrom = NULL, SendToFunc sendto = NULL);
	//输入
	MyServerParameter& operator<<(AcceptFunc func);
	MyServerParameter& operator<<(RecvFunc func);
	MyServerParameter& operator<<(SendFunc func);
	MyServerParameter& operator<<(SendToFunc func);
	MyServerParameter& operator<<(RecvFromFunc func);
	// MyServerParameter& operator<<(SendToFunc func);
	MyServerParameter& operator<<(std::string& ip);
	MyServerParameter& operator<<(short port);
	MyServerParameter& operator<<(SocketType type);
	//输出
	MyServerParameter& operator>>(AcceptFunc& func);
	MyServerParameter& operator>>(RecvFunc& func);
	MyServerParameter& operator>>(SendFunc& func);
	MyServerParameter& operator>>(RecvFromFunc func);
	MyServerParameter& operator>>(SendToFunc func);
	MyServerParameter& operator>>(std::string& ip);
	MyServerParameter& operator>>(short& port);
	MyServerParameter& operator>>(SocketType& type);
	//复制构造函数，等于号重载，用于同类型复制
	MyServerParameter(const MyServerParameter& param);
	MyServerParameter& operator=(const MyServerParameter& param);
	std::string m_ip;
	short m_port;
	SocketType m_type;
	AcceptFunc m_accept;
	RecvFunc m_recv;
	RecvFromFunc m_recvfrom;
	SendFunc m_send;
	SendToFunc m_sendto;
};

class MynewServer : public ThreadFuncBase
{
public:
	MynewServer(const MyServerParameter& param);
	~MynewServer();
	int Invoke(void* arg);
	int send(MYSOCKET& client, const MyBuffer& buffer);
	int Sendto(MySockaddrIn& addr, const MyBuffer& buffer);
	int Stop();

private:
	int threadFunc();
	int threadUDPFunc();
	int threadTCPFunc();

private:
	MyServerParameter m_params;
	void* m_args;
	MyThread m_thread;
	MYSOCKET m_sock;
	std::atomic<bool> m_stop;
};
