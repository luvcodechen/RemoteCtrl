#pragma once
#include <list>
#include "pch.h"

template <class T>
class MyQueue
{
	//线程安全队列（利用IOCP实现）
public:
	enum
	{
		EQNone,
		EQPush,
		EQPop,
		EQSize,
		EQClear,
	};

	typedef struct IocpParam
	{
		size_t nOperator; //操作
		T Data; //数据
		HANDLE hEvent; //pop操作需要的事件句柄
		IocpParam(int op, const T& data, HANDLE hEve = NULL)
		{
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}

		IocpParam()
		{
			nOperator = EQNone;
		}
	} PPARAM; //post parameter 用于投递信息的结构体

	MyQueue()
	{
		m_lock = false;
		m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0); //创建完成端口
		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompletionPort != NULL)
		{
			m_hThread = (HANDLE)_beginthread(&MyQueue<T>::threadEntry, 0, m_hCompletionPort); //创建线程
		}
	}

	~MyQueue()
	{
		m_lock = true; //锁定队列
		HANDLE hTemp = m_hCompletionPort;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
		//投递一个空的消息
		WaitForSingleObject(m_hThread, INFINITE); //等待线程结束
		m_hCompletionPort = NULL;
		CloseHandle(hTemp); //关闭完成端口
	}

	bool PushBack(const T& data)
	{
		IocpParam* pParam = new IocpParam(EQPush, data);
		if (m_lock)
		{
			delete pParam;
			return false;
		}
		BOOL ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(pParam), (ULONG_PTR)pParam, NULL); //投递消息
		if (ret == false)
		{
			delete pParam;
			pParam = NULL;
		}
		return ret;
	}

	bool PopFront(T& data)
	{
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); //创建事件
		IocpParam pParam(EQPop, data, hEvent);
		if (m_lock == true)
		{
			if (hEvent)CloseHandle(hEvent);
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(pParam), (ULONG_PTR)&pParam, NULL); //投递消息
		if (ret == false)
		{
			CloseHandle(hEvent);
			return false;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0; //等待事件
		if (ret)
		{
			data = pParam.Data;
		}
		return ret;
	}

	size_t Size()
	{
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); //创建事件
		IocpParam pParam(EQSize, T(), hEvent);
		if (m_lock == true)
		{
			if (hEvent)CloseHandle(hEvent);
			return -1;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(pParam), (ULONG_PTR)&pParam, NULL); //投递消息
		if (ret == false)
		{
			CloseHandle(hEvent);
			return -1;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0; //等待事件
		if (ret)
		{
			return pParam.nOperator;
		}
		return -1;
	}

	bool Clear()
	{
		if (m_lock)return false;
		IocpParam* pParam = new IocpParam(EQClear, T());
		BOOL ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(pParam), (ULONG_PTR)pParam, NULL); //投递消息
		if (ret == false)
		{
			delete pParam;
			pParam = NULL;
		}
		return ret;
	}

private
:
	static void threadEntry(void* arg)
	{
		MyQueue<T>* pThis = (MyQueue<T>*)arg;
		pThis->threadMain();
		_endthread();
	}

	void DealParam(PPARAM* pParam)
	{
		switch (pParam->nOperator)
		{
		case EQPush:
			m_listData.push_back(pParam->Data);
			delete pParam;
			break;
		case EQPop:
			if (m_listData.size() > 0)
			{
				pParam->Data = m_listData.front();
				m_listData.pop_front();
			}
			if (pParam->hEvent != NULL)
				SetEvent(pParam->hEvent);
			break;
		case EQSize:
			pParam->nOperator = m_listData.size();
			if (pParam->hEvent != NULL)
				SetEvent(pParam->hEvent);
			break;
		case EQClear:
			m_listData.clear();
			delete pParam;
			break;
		default:
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
	}

	void threadMain()
	{
		DWORD dwTransferred = 0;
		PPARAM* pParam = NULL;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* pOverlapped = NULL;
		while (GetQueuedCompletionStatus(m_hCompletionPort, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE))
		{
			if (CompletionKey == 0 && dwTransferred == NULL)
			{
				printf("exit threadQueueEntry\r\n");
				break;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		while (GetQueuedCompletionStatus(m_hCompletionPort, &dwTransferred, &CompletionKey, &pOverlapped, 0))
		{
			if (CompletionKey == 0 && dwTransferred == NULL)
			{
				printf("exit threadQueueEntry\r\n");
				continue;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		CloseHandle(m_hCompletionPort); //关闭完成端口
	}

	std::list<T> m_listData; //数据队列
	HANDLE m_hCompletionPort; //完成端口
	HANDLE m_hThread; //线程句柄
	std::atomic<bool> m_lock; //队列正在析构
};
