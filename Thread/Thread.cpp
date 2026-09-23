#include "Thread.h"


IUI::CAutoCSInit::CAutoCSInit()
{
	InitializeCriticalSection(&m_cs);
}

IUI::CAutoCSInit::~CAutoCSInit()
{
	DeleteCriticalSection(&m_cs);
}

IUI::CAutoCriticalSection::CAutoCriticalSection(CRITICAL_SECTION* pcs)
	: m_pcs(pcs)
{
	EnterCriticalSection(m_pcs);
}

IUI::CAutoCriticalSection::~CAutoCriticalSection()
{
	LeaveCriticalSection(m_pcs);
}
