#pragma once
#include <windows.h>

class CMSAAHelper
{
public:
	CMSAAHelper();
	virtual ~CMSAAHelper();

public:
	int Init(HWND hHost);
	int BuildUITree();

protected:
	HWND m_hWndHost = nullptr;
};
