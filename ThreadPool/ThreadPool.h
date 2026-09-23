#pragma once

#include <atomic>
#include <list>
#include <thread>
#include <mutex>
#include <windows.h>

// 线程池类，特性：
// 支持设置最大线程数量
// 支持线程完成工作后释放线程，当没有任何任务时，线程数量为0
// 支持在执行任务时，停止所有运行的线程
// 【注意】：注意CThreadPool的生命周期，在线程工作过程中，要保证CThreadPool对象的有效性。
namespace IUI
{
	enum THREAD_POOL_EVENT
	{
		TPE_WORK_THREAD_START,
		TPE_WORK_THREAD_ENDING,
		TPE_MONITE_THREAD_START,
		TPE_MONITE_THREAD_ENDING,
	};

	// 任务节点
	class CThreadPoolTask
	{
	public:
		CThreadPoolTask();
		~CThreadPoolTask();

	public:
		void* m_pTask = nullptr;
		__int64 m_llExtern = 0;
	};

	// 任务队列
	class CTaskList
	{
	public:
		CTaskList();
		~CTaskList();

	public:
		int AddTask(const CThreadPoolTask* pTask);
		int TakeTask(const CThreadPoolTask** pTask);
		int DeleteTask(const CThreadPoolTask* pTask);
		size_t GetTaskCount() const;
		bool IsEmpty() const;
		int ClearAllTasks();

	protected:
		// 任务队列
		std::list<const CThreadPoolTask*> m_lstTasks;
		// 保护m_lstTasks
		mutable std::mutex m_mutexLstTasks;
	};

	// 线程池类
	// int ThreadPoolTaskCallback(std::atomic<bool>& bExit, const IUI::CThreadPoolTask* pTask, const class IUI::CThreadPool* pThis, void* pUserData) {}
	typedef int (*TaskCallback)(std::atomic<bool> &bExit, const CThreadPoolTask *pTask, const class CThreadPool *pThis, void* pUserData);
	class CThreadPool
	{
	public:
		CThreadPool();
		virtual ~CThreadPool();

	public:
		// 设置线程的最大和最小数量
		int SetMaxThreadCount(int nMax);
		int GetMaxThreadCount() const;

		int SetMinThreadCount(int nMin);
		int GetMinThreadCount() const;

		// 设置工作线程函数的回调，当工作线程运行时，将回调用户传入的函数
		int SetTaskCallback(TaskCallback fnCallback, void* pUserData);
		TaskCallback GetTaskCallback(void **ppUserData) const;

		// 添加任务，当工作线程运行时，任务将作为参数调用用户传入的回调。用户的回调函数中，就可以处理这个任务。
		int AddTask(const CThreadPoolTask* pTask);
		int DeleteTask(const CThreadPoolTask* pTask);
		size_t GetTaskCount() const;

		// 停止所有工作中的线程，并等待所有线程退出
		// 特别注意：Stop接口往往由主线程调用，所以，在工作线程的用户指定的回调函数中，千万不要同步操作主线程中的对象,
		// 在工作线程回调中调用SetWindowText、SendMessage之类的API，也会引起死锁。
		int Stop();

		bool IsTaskNeedStop() const;

	protected:
		// 任务线程
		// 初始创建工作线程的时候，先给它一个任务，这个任务执行完后，TaskThread就自己去任务队列中取任务了。
		void TaskThread(const CThreadPoolTask* pTask);

		// 线程函数。单独启动一个线程，用来监控工作线程退出。当工作线程退出后，
		// 需要把刚结束的线程从m_ThreadList中移除，并启动一个新的线程，使线程总量达到m_nMaxThreadCount或任务数（两者中较小那个）
		int MonitorTaskThreadExitThread();

		// 根据任务数和当前工作线程数决定是否启动新工作线程
		// 这个函数只负责启动工作线程，所以应该仅由AddTask和UpdateTaskThreadCountThread调用。
		int DoTask();

		virtual int OnNotify(THREAD_POOL_EVENT eEvent, WPARAM wParam, LPARAM lParam);

	private:
		int RemoveThreadHandle(HANDLE hThread);

	protected:
		// 最大最小线程数
		std::atomic<int> m_nMaxThreadCount = std::thread::hardware_concurrency();
		std::atomic<int> m_nMinThreadCount = 0;

		// 线程池中的线程回调
		TaskCallback m_fnTaskCallback = nullptr;
		void* m_pUserData = nullptr;

		// 线程池中的工作线程
		std::list<std::shared_ptr<std::thread>> m_ThreadList;
		std::mutex m_mutexThreadList;

		// 线程安全的任务列表
		// 假设现在新来一个任务A，调用DoTask创建工作线程a1来执行任务A。
		// 这时又新来一个任务B，调用DoTask创建工作线程，如果这时工作线程a1还没有取走任务A，则DoTask可能会创建两个新线程a2和b来执行任务A和B。
		// 但任务A本来是交给工作线程a1执行的，现在工作线程a2也对应任务A，这导致了两个任务启动了三个工作线程。
		// 所以，要设计一种机制，来杜绝了已创建工作线程的任务，不要重复创建
		// 我们的做法是创建工作线程的时候，先从任务队列中取走一个任务交给工作线程，这样这个任务就不会重复创建工作线程，
		// 之后，工作线程自己从任务队列中取任务。
		CTaskList m_TaskList;

		// 监控工作线程退出，当工作线程退出后，需要更新m_ThreadList，并启动新的工作线程。
		std::thread m_tMonitorTaskThread;

		std::atomic<bool> m_bExit = false;
	};
}



