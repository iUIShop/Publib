#pragma once

#include <windows.h>


namespace IUI
{
	class CAutoCSInit
	{
	public:
		CAutoCSInit();
		~CAutoCSInit();

	public:
		CRITICAL_SECTION m_cs;
	};

	class CAutoCriticalSection
	{
	public:
		CAutoCriticalSection(CRITICAL_SECTION* pcs);
		~CAutoCriticalSection();

	private:
		CRITICAL_SECTION* m_pcs;
	};

}