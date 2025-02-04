#pragma once
#include <list>

template <class T>
class MyQueue
{
	//线程安全队列（利用IOCP实现）
public:
	MyQueue();
	~MyQueue();
	bool PushBack(const T& data);
	bool PopFront(T& data);
	size_t Size();
	void Clear();

private:
	static void threadEntry(void* arg);
	void threadMain();
	std::list<T> m_listData; //数据队列
	HANDLE m_hCompletionPort; //完成端口
	HANDLE m_hThread; //线程句柄
public:
	typedef struct IocpParam
	{
		int nOperator; //操作
		T strData; //数据
		HANDLE hEvent;//pop操作需要的事件句柄
		IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL)
		{
			nOperator = op;
			strData = sData;
		}

		IocpParam()
		{
			nOperator = -1;
			strData = "";
		}
	} PPARAM;//post parameter 用于投递信息的结构体
	enum
	{
		EQPush,
		EQtPop,
		EQSize,
		EQClear,
	};
};
