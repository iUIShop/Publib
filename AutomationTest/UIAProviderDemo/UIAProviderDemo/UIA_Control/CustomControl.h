/*************************************************************************************************
* Description: Declarations for the custom list control.
*
* See EntryPoint.cpp for a full description of this sample.
*
*
*  Copyright (C) Microsoft Corporation.  All rights reserved.
*
* This source code is intended only as a supplement to Microsoft
* Development Tools and/or on-line documentation.  See these other
* materials for detailed information regarding Microsoft code samples.
*
* THIS CODE AND INFORMATION ARE PROVIDED AS IS WITHOUT WARRANTY OF ANY
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
*************************************************************************************************/
#pragma once
#pragma warning (disable : 4244)  // Disable bogus warning for SetWindowLongPtr

#include <windows.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <stdio.h>
#include <stdlib.h>
#include <ole2.h>
#include <UIAutomation.h>
#include <vector>
#include <deque>
#include <string>
#include <atltypes.h>
#include <assert.h>

class Canvas2;



// Forward declarations.
class WLWnd;
class Canvas;
class RootProvider;
class WLWndProvider;

// WLWnd control class -- an item in the list.
//
class WLWnd
{
public:
	WLWnd(HWND hwnd);
	virtual ~WLWnd();

public:
	int Create(Canvas* pPrent, LPCWSTR lpszControlType, LPCWSTR lpszName, int x, int y, int nWidth, int nHeight, int id);

	WLWnd* GetWindow(UINT uCmd);

	WCHAR* GetName();
	int GetId();
	Canvas* GetParent();
	int GetRect(LPRECT lpRc) const;
	WLWnd *GetItemAt(int index);

	RootProvider* GetProvider();

	int OnDraw(HDC hdc);
	HWND GetHwnd();

public:
	WLWnd* m_pParent = nullptr;
	WLWnd* m_pChild = nullptr;
	WLWnd* m_pNext = nullptr;
	WLWnd* m_pPrev = nullptr;
	WLWnd* m_pFocus = nullptr;

	int m_Id = 0;
	WCHAR* m_name = nullptr;
	RootProvider* m_pProvider = nullptr;
	Canvas* m_pOwnerControl = nullptr;
	CRect m_rc;
	std::wstring m_strControlType;
	HWND   m_controlHwnd;
};

class Canvas : public WLWnd
{
public:
	Canvas(HWND hwnd);
	virtual ~Canvas();

public:
	RootProvider* GetProvider();
	bool GetIsFocused();
	void SetIsFocused(bool isFocused);

private:
	bool   m_hasFocus;
	RootProvider* m_pRootProvider = nullptr;
};
