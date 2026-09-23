/*************************************************************************************************
* Description: Implementation of the custom list control.
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
#include "CustomControl.h"
#include "../UIA_Provider/UIAProviders.h"
#pragma comment(lib, "Uiautomationcore.lib")


// Canvas class.
//
Canvas::Canvas(HWND hwnd) :
	WLWnd(hwnd)
{
	// Initialize the list items.
	WLWnd* pItem = new WLWnd(hwnd);
	pItem->Create(this, L"abc", L"def", 0, 0, 100, 100, 7);

	WLWnd* pItem2 = new WLWnd(hwnd);
	pItem2->Create(this, L"123", L"456", 0, 150, 100, 200, 8);
}

// Destructor.
//
Canvas::~Canvas()
{
	if (m_pRootProvider != NULL)
	{
		m_pRootProvider->Release();
	}
}

// Gets the UI Automation provider for the list; creates it if necessary.
//
RootProvider* Canvas::GetProvider()
{
	if (m_pRootProvider == NULL)
	{
		m_pRootProvider = new (std::nothrow) RootProvider(this);
	}
	return m_pRootProvider;
}

// Gets the focused state.
//
bool Canvas::GetIsFocused()
{
	return m_hasFocus;
}

// Sets the focused state.
//
void Canvas::SetIsFocused(bool isFocused)
{
	m_hasFocus = isFocused;
}

// Gets the HWND of the control.
//
HWND WLWnd::GetHwnd()
{
	return m_controlHwnd;
}


// WLWnd class 
//
// Constructor.
WLWnd::WLWnd(HWND hwnd)
	: m_controlHwnd(hwnd)
{
	int n = 0;
}

// Destructor.
WLWnd::~WLWnd()
{
	free(m_name);
	if (m_pProvider != NULL)
	{
		m_pProvider->Release();
	}
}

int WLWnd::OnDraw(HDC hdc)
{
	HBRUSH hbrBlack = (HBRUSH)::GetStockObject(BLACK_BRUSH);
	::FrameRect(hdc, m_rc, hbrBlack);
	::DeleteObject(hbrBlack);

	// Draw the text.
	TextOutW(hdc, m_rc.left, m_rc.top, GetName(), static_cast<int>(wcslen(GetName())));

	return 0;
}

// Gets the UI Automation provider for the list item; creates it if necessary.
//
RootProvider* WLWnd::GetProvider()
{
	if (m_pProvider == NULL)
	{
		m_pProvider = new (std::nothrow) RootProvider(this);
	}
	return m_pProvider;
}

// Gets the custom list control that holds this item.
//
Canvas* WLWnd::GetParent()
{
	return m_pOwnerControl;
}

int WLWnd::GetRect(LPRECT lpRc) const
{
	if (nullptr == lpRc)
	{
		return -1;
	}

	*lpRc = m_rc;

	return 0;
}

WLWnd* WLWnd::GetItemAt(int index)
{
	int i = 0;
	WLWnd* pRet = nullptr;
	WLWnd* pChild = m_pChild;
	while (nullptr != pChild)
	{
		if (i == index)
		{
			pRet = pChild;
			break;
		}

		pChild = pChild->m_pNext;
		i++;
	}

	return pRet;
}

BOOL AddHWLWND(WLWnd* pWnd, WLWnd* pParent)
{
	if (pParent == NULL || pWnd == NULL)
	{
		return FALSE;
	}

	// 先看一下hParent中，是否已包含hWnd
	BOOL bExist = FALSE;
	WLWnd* pChild = NULL;
	pChild = pParent->GetWindow(GW_CHILD);

	WLWnd* pLast = NULL;
	for (; pChild != NULL; pChild = pChild->GetWindow(GW_HWNDNEXT))
	{
		if (pChild == pWnd)
		{
			bExist = TRUE;
			break;
		}

		pLast = pChild;
	}

	if (bExist)
	{
		return TRUE;
	}

	pWnd->m_pParent = pParent;

	if (pLast == NULL)
	{
		pParent->m_pChild = pWnd;
	}
	else
	{
		pLast->m_pNext = pWnd;
		pWnd->m_pPrev = pLast;
	}

	return TRUE;
}

int WLWnd::Create(Canvas* pParent, LPCWSTR lpszControlType, LPCWSTR lpszName, int x, int y, int nWidth, int nHeight, int id)
{
	m_pOwnerControl = pParent;
	m_strControlType = lpszControlType;
	m_Id = id;
	m_name = _wcsdup(lpszName);
	m_pProvider = NULL;
	m_rc.left = x;
	m_rc.top = y;
	m_rc.right = m_rc.left + nWidth;
	m_rc.bottom = m_rc.top + nHeight;

	if (nullptr != pParent)
	{
		AddHWLWND(this, pParent);
	}
	RootProvider* itemProvider = GetProvider();
	//itemProvider->NotifyItemAdded();

	return 0;
}

WLWnd* WLWnd::GetWindow(UINT uCmd)
{
	WLWnd* pwndT = NULL;

	switch (uCmd)
	{
	case GW_HWNDNEXT:
		pwndT = m_pNext;
		break;

	case GW_HWNDFIRST:
		if (m_pParent != NULL)
		{
			pwndT = m_pParent->m_pChild;
		}
		break;

	case GW_HWNDLAST:
		pwndT = this;
		while (NULL != pwndT && NULL != pwndT->m_pNext)
		{
			pwndT = pwndT->m_pNext;
		}
		break;

	case GW_HWNDPREV:
		pwndT = m_pPrev;
		break;

	case GW_OWNER:
		pwndT = GetParent();
		break;

	case GW_CHILD:
		pwndT = m_pChild;
		break;

	case GW_ENABLEDPOPUP:
		pwndT = NULL;
		break;

	default:
		return NULL;
	}

	return pwndT;
}

// Gets the name of the contact.
//
WCHAR* WLWnd::GetName()
{
	return m_name;
}

// Gets the Id of the contact.
//
int WLWnd::GetId()
{
	return m_Id;
}
