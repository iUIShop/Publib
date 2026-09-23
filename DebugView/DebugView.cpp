#include "DebugView.h"


IUI::DebugView::DebugView()
{
}

IUI::DebugView::~DebugView()
{
	Uninit();
}

int IUI::DebugView::Init(OnNotify notify) // 当收到OutputDebugString后，调用notify
{
	int nRet = 0;

	do
	{
		if (m_t.joinable())
		{
			nRet = -2;
			break;
		}
		m_notify = notify;

		// "DBWIN_BUFFER_READY"是一个特殊的名字，必须叫这个名字
		HANDLE hTest = ::OpenEvent(SYNCHRONIZE, FALSE, L"DBWIN_BUFFER_READY");
		if (nullptr != hTest)
		{
			// dbgview.exe可能已启动
			::CloseHandle(hTest);
			nRet = -3;
			break;
		}

		m_hEvtBufReady = ::CreateEvent(nullptr, FALSE, TRUE, L"DBWIN_BUFFER_READY");
		m_hEvtDataReady = ::CreateEvent(nullptr, FALSE, FALSE, L"DBWIN_DATA_READY");
		m_hEvtExit = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);

		m_hFile = ::CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, buffer_size, L"DBWIN_BUFFER");
		if (nullptr != m_hFile)
		{
			m_pViewOfFile = (DbgViewBuffer*)::MapViewOfFile(m_hFile, FILE_MAP_READ, 0, 0, buffer_size);
		}

		if (nullptr == m_hEvtBufReady
			|| nullptr == m_hEvtDataReady
			|| nullptr == m_hEvtExit
			|| nullptr == m_hFile
			|| nullptr == m_pViewOfFile)
		{
			nRet = -4;
			break;
		}

		m_t = std::thread(&IUI::DebugView::ThreadProc, this);
		if (!m_t.joinable())
		{
			nRet = -5;
			break;
		}
	} while (false);

	if (nRet != 0)
	{
		Uninit();
	}

	return nRet;
}

void IUI::DebugView::Uninit()
{
	auto close = [](HANDLE& h)
		{
			if (h != nullptr && h != INVALID_HANDLE_VALUE) {
				::CloseHandle(h);
				h = nullptr;
			}
		};

	if (m_t.joinable())
	{
		::SetEvent(m_hEvtExit);
		// This will cause deadlock while going to notify
		// WaitForSingleObject(_hThread, INFINITE);
		m_t.join();
	}

	if (m_pViewOfFile)
	{
		::UnmapViewOfFile(m_pViewOfFile);
		m_pViewOfFile = nullptr;
	}

	close(m_hFile);

	close(m_hEvtExit);
	close(m_hEvtBufReady);
	close(m_hEvtDataReady);
}

unsigned int IUI::DebugView::ThreadProc()
{
	HANDLE handles[2] = { m_hEvtExit, m_hEvtDataReady };

	for (bool loop = true; loop;)
	{
		DWORD dwWait = ::WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE);
		switch (dwWait)
		{
		case WAIT_OBJECT_0 + 0:
			::ResetEvent(m_hEvtExit);
			loop = false;
			break;

		case WAIT_OBJECT_0 + 1:
			if (m_notify)
				m_notify(m_pViewOfFile->pid, m_pViewOfFile->str);

			::SetEvent(m_hEvtBufReady);
			loop = true;
			break;

		default:
			loop = false;
			break;
		}
	}

	return 0;
}
