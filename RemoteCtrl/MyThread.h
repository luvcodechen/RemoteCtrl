#pragma once
#include <atomic>
#include <vector>
#include <mutex>

class ThreadFuncBase
{
};

typedef int (ThreadFuncBase::*FUNCTYPE)();

class ThreadWorker
{
public:
	ThreadWorker(): thiz(NULL), func(NULL)
	{
	}

	ThreadWorker(ThreadFuncBase* pthiz, FUNCTYPE f)
	{
		thiz = pthiz;
		func = f;
	}

	ThreadWorker(const ThreadWorker& other)
	{
		thiz = other.thiz;
		func = other.func;
	}

	ThreadWorker& operator=(const ThreadWorker& other)
	{
		if (this != &other) //防止自赋值
		{
			thiz = other.thiz;
			func = other.func;
		}
		return *this;
	}

	int operator()()
	{
		if (IsValid())
			return (thiz->*func)();
		return -1;
	}

	bool IsValid() const
	{
		return (thiz != NULL) && (func != NULL);
	}

private:
	ThreadFuncBase* thiz; //指向对象的指针
	FUNCTYPE func; //函数指针
};


class MyThread
{
public:
	MyThread()
	{
		m_hThread = NULL;
		m_bStatus = false;
	}

	~MyThread()
	{
		Stop();
	}

	//true表示成功启动线程，false表示启动线程失败
	bool Start()
	{
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&MyThread::threadEntry, 0, this);
		if (!IsValid())
		{
			m_bStatus = false;
		}
		return m_bStatus;
	}

	bool IsValid() //true表示有效，false表示无效 
	{
		if (m_hThread == NULL || m_hThread == INVALID_HANDLE_VALUE)
			return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT; //等待线程结束
	}

	bool Stop()
	{
		if (m_bStatus == false)
			return true;
		m_bStatus = false;
		DWORD ret = WaitForSingleObject(m_hThread, 1000); //等待线程结束
		if (ret == WAIT_TIMEOUT)
		{
			TerminateThread(m_hThread, -1); //强制结束线程	
		}
		UpdateWorker();
		return ret == WAIT_OBJECT_0;
	}

	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker())
	{
		if (m_worker.load() != NULL && m_worker.load() != &worker)
		{
			::ThreadWorker* pWorker = m_worker.load();
			m_worker.store(NULL);
			delete pWorker;
		}
		if (m_worker.load() == &worker)
			return;
		if (!worker.IsValid())
		{
			m_worker.store(NULL);
			return;
		}
		m_worker.store(new ::ThreadWorker(worker)); //设置工作函数
	}

	bool IsIdle() //true表示空闲，false表示正在工作
	{
		if (m_worker.load() == NULL)
			return true;
		return !m_worker.load()->IsValid();
	}

private:
	virtual void ThreadWorker()
	{
		while (m_bStatus)
		{
			if (m_worker.load() == NULL)
			{
				Sleep(1);
				continue;
			}
			::ThreadWorker worker = *m_worker.load();
			if (worker.IsValid())
			{
				int ret = worker();
				if (ret != 0)
				{
					CStringA str;
					str.Format("thread found warning code %d\r\n", ret); //输出警告信息
					OutputDebugStringA(str); //输出警告信息
				}
				if (ret < 0)
				{
					m_worker.store(NULL);
				}
			}
			else
			{
				Sleep(1);
			}
		}
	}

	static void threadEntry(void* arg)
	{
		MyThread* pThis = (MyThread*)arg;
		if (pThis)
			pThis->ThreadWorker();
		_endthread();
	}

private:
	HANDLE m_hThread;
	bool m_bStatus; //false 表示线程将要关闭，true 表示线程正在运行
	std::atomic<::ThreadWorker*> m_worker;
};

class ThreadPool
{
public:
	ThreadPool(size_t size)
	{
		m_Threads.resize(size);
		for (size_t i = 0; i < size; i++)
		{
			m_Threads[i] = new MyThread();
		}
	}

	ThreadPool()
	{
	}

	~ThreadPool()
	{
		Stop();
		for (size_t i = 0; i < m_Threads.size(); i++)
		{
			delete m_Threads[i];
			m_Threads[i] = NULL;
		}
		m_Threads.clear();
	}


	bool Invoke()
	{
		bool ret = true;
		for (size_t i = 0; i < m_Threads.size(); i++)
		{
			if (!m_Threads[i]->Start())
			{
				ret = false;
				break;
			}
		}
		if (ret == false)
		{
			for (size_t i = 0; i < m_Threads.size(); i++)
			{
				m_Threads[i]->Stop();
			}
		}
		return ret;
	}

	void Stop()
	{
		for (size_t i = 0; i < m_Threads.size(); i++)
		{
			m_Threads[i]->Stop();
		}
	}

	int DispatchWorker(const ThreadWorker& woker)
	{
		int index = -1;
		m_lock.lock();
		for (size_t i = 0; i < m_Threads.size(); i++)
		{
			if (m_Threads[i]->IsIdle())
			{
				m_Threads[i]->UpdateWorker(woker);
				index = i;
				break;
			}
		}
		m_lock.unlock();
		return index;
	}

	bool CheckThreadValid(size_t index)
	{
		if (index < m_Threads.size())
			return m_Threads[index]->IsValid();
		return false;
	}

private:
	std::vector<MyThread*> m_Threads;
	std::mutex m_lock;
};
