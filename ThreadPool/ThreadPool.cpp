#include "ThreadPool.h"
#include <vector>
#include <windows.h>
#include <minwindef.h>

HANDLE g_hFindThread = nullptr;


IUI::CThreadPoolTask::CThreadPoolTask()
{

}

IUI::CThreadPoolTask::~CThreadPoolTask()
{
	// 销毁前应该先调用Stop
}

//////////////////////////////////////////////////////

IUI::CTaskList::CTaskList()
{
}

IUI::CTaskList::~CTaskList()
{
}

int IUI::CTaskList::AddTask(const CThreadPoolTask* pTask)
{
	std::lock_guard<std::mutex> mutex(m_mutexLstTasks);

	m_lstTasks.push_back(pTask);

	return 0;
}

int IUI::CTaskList::TakeTask(const CThreadPoolTask** ppTask)
{
	if (nullptr == ppTask)
	{
		return -1;
	}

	std::lock_guard<std::mutex> mutex(m_mutexLstTasks);

	if (m_lstTasks.empty())
	{
		return -2;
	}

	const CThreadPoolTask*p= m_lstTasks.front();
	*ppTask = p;

	m_lstTasks.erase(m_lstTasks.begin());

	return 0;
}

const IUI::CThreadPoolTask* g_pFindTask = nullptr;
bool FindTaskCallback(const IUI::CThreadPoolTask*ptr)
{
	return (ptr == g_pFindTask);
}
int IUI::CTaskList::DeleteTask(const CThreadPoolTask* pTask)
{
	std::lock_guard<std::mutex> mutex(m_mutexLstTasks);

	g_pFindTask = pTask;
	auto it = std::find_if(m_lstTasks.begin(), m_lstTasks.end(), FindTaskCallback);
	if (it != m_lstTasks.end())
	{
		m_lstTasks.erase(it);
	}

	g_pFindTask = nullptr;

	return 0;
}

size_t IUI::CTaskList::GetTaskCount() const
{
	std::lock_guard<std::mutex> mutex(m_mutexLstTasks);

	return m_lstTasks.size();
}

bool IUI::CTaskList::IsEmpty() const
{
	std::lock_guard<std::mutex> mutex(m_mutexLstTasks);

	return m_lstTasks.empty();
}

int IUI::CTaskList::ClearAllTasks()
{
	std::lock_guard<std::mutex> mutex(m_mutexLstTasks);

	m_lstTasks = std::list<const CThreadPoolTask*>();

	return 0;
}

//////////////////////////////////////////////////////

IUI::CThreadPool::CThreadPool()
{
}
	
IUI::CThreadPool::~CThreadPool()
{
}

int IUI::CThreadPool::SetMaxThreadCount(int nMax)
{
	m_nMaxThreadCount = nMax;

	return 0;
}

int IUI::CThreadPool::GetMaxThreadCount() const
{
	return m_nMinThreadCount;
}

int IUI::CThreadPool::SetMinThreadCount(int nMin)
{
	m_nMinThreadCount = nMin;

	return 0;
}

int IUI::CThreadPool::GetMinThreadCount() const
{
	return m_nMinThreadCount;
}

int IUI::CThreadPool::SetTaskCallback(TaskCallback fnCallback, void* pUserData)
{
	m_fnTaskCallback = fnCallback;
	m_pUserData = pUserData;

	return 0;
}

IUI::TaskCallback IUI::CThreadPool::GetTaskCallback(void** ppUserData) const
{
	if (nullptr != ppUserData)
	{
		*ppUserData = m_pUserData;
	}

	return m_fnTaskCallback;
}

int IUI::CThreadPool::AddTask(const CThreadPoolTask* pTask)
{
	// 没有任务函数，把任务加进去没有意义
	if (nullptr == m_fnTaskCallback)
	{
		return -2;
	}

	m_bExit = false;

	// new一个新的task，我们通过task指针的值来区分task.
	CThreadPoolTask* pNewTask = new CThreadPoolTask();
	*pNewTask = *pTask;

	int nRet = m_TaskList.AddTask(pNewTask);
	if (0 != nRet)
	{
		return nRet;
	}

	DoTask();

	return 0;
}

int IUI::CThreadPool::DeleteTask(const CThreadPoolTask* pTask)
{
	return m_TaskList.DeleteTask(pTask);
}

size_t IUI::CThreadPool::GetTaskCount() const
{
	return m_TaskList.GetTaskCount();
}

int IUI::CThreadPool::Stop()
{
	m_bExit = true;

	if (m_tMonitorTaskThread.joinable())
	{
		m_tMonitorTaskThread.join();
	}

	m_TaskList.ClearAllTasks();

	return 0;
}

bool IUI::CThreadPool::IsTaskNeedStop() const
{
	return m_bExit;
}

// 线程自已去队列中读取任务，当读不到任务后，线程自己退出
void IUI::CThreadPool::TaskThread(const CThreadPoolTask* pTask)
{
	OnNotify(TPE_WORK_THREAD_START, GetCurrentThreadId(), 0);

	do
	{
		if (m_bExit)
		{
			break;
		}

		if (nullptr == pTask)
		{
			int nRet = m_TaskList.TakeTask(&pTask);
			if (0 != nRet)
			{
				break;
			}
		}

		//g_log.WriteA("[%d]工作线程处理任务: %p.", GetCurrentThreadId(), (DWORD_PTR)pTask);

		if (nullptr != m_fnTaskCallback)
		{
			m_fnTaskCallback(m_bExit, pTask, this, m_pUserData);
		}

		if (nullptr != pTask)
		{
			delete pTask;
			pTask = nullptr;
		}
	} while (true);

	OnNotify(TPE_WORK_THREAD_ENDING, GetCurrentThreadId(), 0);
}

// 这是一个单独的线程函数
int IUI::CThreadPool::MonitorTaskThreadExitThread()
{
	OnNotify(TPE_MONITE_THREAD_START, GetCurrentThreadId(), 0);

	//
	// 同时等待多个线程的退出，退出一个，就从列表中删除一个
	//
	while (true)
	{
		std::vector<HANDLE> vThreads;

		{
			std::lock_guard<std::mutex> locker(m_mutexThreadList);
			for (auto& h : m_ThreadList)
			{
				vThreads.push_back(h.get()->native_handle());
			}
		}

		if (vThreads.empty())
		{
			// 任务结束
			break;
		}

		// WaitForMultipleObjects一次最多只能等待MAXIMUM_WAIT_OBJECTS（64）个对象
		// 所以下面的代码有个弊端，就是如果这64个线程后面的线程先退出了，我们是无法及时知道的。
		// TODO: 可以考虑使用WMI监控线程退出，但WMI没有那么及时。
		size_t nWaitCount = vThreads.size();
		if (nWaitCount > MAXIMUM_WAIT_OBJECTS)
		{
			nWaitCount = MAXIMUM_WAIT_OBJECTS;
		}

		DWORD dwObj = ::WaitForMultipleObjects((DWORD)nWaitCount, &vThreads[0], FALSE, INFINITE);
		if (dwObj >= WAIT_OBJECT_0 && dwObj <= (WAIT_OBJECT_0 + nWaitCount - 1))
		{
			RemoveThreadHandle(vThreads[dwObj]);
		}

		// 启动新的线程
		if (!m_bExit)
		{
			DoTask();
		}
	}

	OnNotify(TPE_MONITE_THREAD_ENDING, GetCurrentThreadId(), 0);

	return 0;
}

// 负责创建线程，但不负责销毁线程，因为线程在读不到任务后会自我销毁
// DoTask可以设计成一直负责监视任务队列，所以，它是一个单独的线程
// 也可以设计成在任务队列增加后调用，这样，它就不需要是一个线程了。
int IUI::CThreadPool::DoTask()
{
	//
	// 启动工作线程
	//
	{
		std::lock_guard<std::mutex> locker(m_mutexThreadList);

		size_t nCurThreadCount = m_ThreadList.size();
		if (nCurThreadCount >= m_nMaxThreadCount)
		{
			return 1;
		}

		size_t nTaskCount = m_TaskList.GetTaskCount();
		if (nTaskCount == 0)
		{
			return 1;
		}

		LONGLONG nNewThreadCount = min(LONGLONG(nTaskCount), LONGLONG(m_nMaxThreadCount - nCurThreadCount));
		for (LONGLONG i = 0; i < nNewThreadCount; i++)
		{
			// 从任务列表中拿走任务，防止这个任务重复创建工作线程。
			const CThreadPoolTask* pTask = nullptr;
			int nRet = m_TaskList.TakeTask(&pTask);
			if (0 != nRet)
			{
				break;
			}
			m_ThreadList.push_back(std::make_shared<std::thread>(&IUI::CThreadPool::TaskThread, this, pTask));
		}
	}

	//
	// 启动监控工作线程退出的线程
	//
	{
		DWORD dwObj = WaitForSingleObject(m_tMonitorTaskThread.native_handle(), 0);
		if (WAIT_TIMEOUT != dwObj)
		{
			if (dwObj == WAIT_OBJECT_0)
			{
				// 如果监控线程正常退出，需要调用join，否则std::thread在析构的时候，会崩溃。
				// 因为WaitForSingleObject早于线程结束，所以，一般是等到第二批任务开始的时候，才会执行到这里。
				// 这里的join是join上次退出的线程。
				if (m_tMonitorTaskThread.joinable())
				{
					m_tMonitorTaskThread.join();
				}
			}

			// 监控线程没有运行，需要创建
			m_tMonitorTaskThread = std::thread(&CThreadPool::MonitorTaskThreadExitThread, this);
		}
	}

	return 0;
}

int IUI::CThreadPool::OnNotify(THREAD_POOL_EVENT eEvent, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

bool FindThreadCallback(const std::shared_ptr<std::thread>& ptr)
{
	return (ptr->native_handle() == g_hFindThread);
}

int IUI::CThreadPool::RemoveThreadHandle(HANDLE hThread)
{
	std::lock_guard<std::mutex> locker(m_mutexThreadList);

	g_hFindThread = hThread;

	auto it = std::find_if(m_ThreadList.begin(), m_ThreadList.end(), FindThreadCallback);
	if (it != m_ThreadList.end())
	{
		it->get()->join();
		m_ThreadList.erase(it);
	}

	g_hFindThread = nullptr;

	return 0;
}
