
/*************************************************************************************************
 * Description: Implementation of the RootProvider class, which implements a
 * UI Automation provider for a custom list control.
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

#define INITGUID
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include <ole2.h>
#include "UIAProviders.h"
#include "../UIA_Control/CustomControl.h"

RootProvider::RootProvider(WLWnd* pControl) :
	m_refCount(1), m_pControl(pControl)
{
	m_controlHwnd = pControl->GetHwnd();
}

RootProvider::~RootProvider()
{
}

// IUnknown implementation.
//
IFACEMETHODIMP_(ULONG) RootProvider::AddRef()
{
	return InterlockedIncrement(&m_refCount);
}

IFACEMETHODIMP_(ULONG) RootProvider::Release()
{
	long val = InterlockedDecrement(&m_refCount);
	if (val == 0)
	{
		delete this;
	}
	return val;
}

IFACEMETHODIMP RootProvider::QueryInterface(REFIID riid, void** ppInterface)
{
	if (riid == __uuidof(IUnknown))
	{
		*ppInterface = static_cast<IRawElementProviderSimple*>(this);
	}
	else if (riid == __uuidof(IRawElementProviderSimple))
	{
		*ppInterface = static_cast<IRawElementProviderSimple*>(this);
	}
	else if (riid == __uuidof(IRawElementProviderFragment))
	{
		*ppInterface = static_cast<IRawElementProviderFragment*>(this);
	}
	else if (riid == __uuidof(IRawElementProviderFragmentRoot))
	{
		*ppInterface = static_cast<IRawElementProviderFragmentRoot*>(this);
	}
	else
	{
		*ppInterface = NULL;
		return E_NOINTERFACE;
	}
	(static_cast<IUnknown*>(*ppInterface))->AddRef();
	return S_OK;
}


// IRawElementProviderSimple implementation
//
// Implementation of IRawElementProviderSimple::get_ProviderOptions.
// Gets UI Automation provider options.
//
IFACEMETHODIMP RootProvider::get_ProviderOptions(ProviderOptions* pRetVal)
{
	*pRetVal = ProviderOptions_ServerSideProvider;
	return S_OK;
}

// Implementation of IRawElementProviderSimple::get_PatternProvider.
// Gets the object that supports ISelectionPattern.
//
IFACEMETHODIMP RootProvider::GetPatternProvider(PATTERNID patternId, IUnknown** pRetVal)
{
	*pRetVal = NULL;
	return S_OK;
}

// Implementation of IRawElementProviderSimple::get_PropertyValue.
// Gets custom properties.
//
IFACEMETHODIMP RootProvider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal)
{
	// Although it is hard-coded for the purposes of this sample, localizable 
	// text should be stored in, and loaded from, the resource file (.rc). 
	if (propertyId == UIA_LocalizedControlTypePropertyId)
	{
		pRetVal->vt = VT_BSTR;
		pRetVal->bstrVal = SysAllocString(L"contact list");
	}
	else if (propertyId == UIA_ControlTypePropertyId)
	{
		pRetVal->vt = VT_I4;
		pRetVal->lVal = UIA_ListControlTypeId;
	}
	else if (propertyId == UIA_IsKeyboardFocusablePropertyId)
	{
		pRetVal->vt = VT_BOOL;
		pRetVal->boolVal = VARIANT_TRUE;
	}
	// else pRetVal is empty, and UI Automation will attempt to get the property from
	//  the HostRawElementProvider, which is the default provider for the HWND.
	// Note that the Name property comes from the Caption property of the control window, 
	//  if it has one.
	else
	{
		pRetVal->vt = VT_EMPTY;
	}
	return S_OK;
}

// Implementation of IRawElementProviderSimple::get_HostRawElementProvider.
// Gets the default UI Automation provider for the host window. This provider 
// supplies many properties.
//
IFACEMETHODIMP RootProvider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal)
{
	if (m_controlHwnd == NULL)
	{
		return UIA_E_ELEMENTNOTAVAILABLE;
	}
	HRESULT hr = UiaHostProviderFromHwnd(m_controlHwnd, pRetVal);
	return hr;
}


// IRawElementProviderFragment implementation
//
// Implementation of IRawElementProviderFragment::Navigate.
// Enables UI Automation to locate the element in the tree.
// Navigation to the parent is handled by the host window provider.
//
IFACEMETHODIMP RootProvider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal)
{
	if (nullptr == pRetVal)
	{
		_ASSERT(FALSE);
		return S_OK;
	}

	WLWnd* pCanvas = this->m_pControl;
	WLWnd* pDest = NULL;
	IRawElementProviderFragment* pFrag = NULL;
	switch (direction)
	{
	case NavigateDirection_FirstChild:
		pDest = pCanvas->m_pChild;
		pFrag = pDest->GetProvider();
		break;
	case NavigateDirection_LastChild:
	{
		WLWnd *pLast = pCanvas->m_pChild;
		while (nullptr != pLast)
		{
			pDest = pLast;
			pLast = pLast->m_pNext;
		}
	}
		pFrag = pDest->GetProvider();
		break;
	}

	if (pFrag != NULL)
	{
		pFrag->AddRef();
	}
	*pRetVal = pFrag;
	return S_OK;
}

// Implementation of IRawElementProviderFragment::GetRuntimeId.
// UI Automation gets this value from the host window provider, so supply NULL here.
//
IFACEMETHODIMP RootProvider::GetRuntimeId(SAFEARRAY** pRetVal)
{
	*pRetVal = NULL;
	return S_OK;
}

// Implementation of IRawElementProviderFragment::get_BoundingRectangle.
//
// Retrieves the screen location and size of the control. Controls hosted in
// Win32 windows can return an empty rectangle; UI Automation will
// retrieve the rectangle from the HWND provider. However, the method is
// implemented here so that it can be used by the list items to calculate
// their own bounding rectangles.
//
// UI Spy uses the bounding rectangle to draw a red border around the element.
//
IFACEMETHODIMP RootProvider::get_BoundingRectangle(UiaRect* pRetVal)
{
	RECT rect;
	GetClientRect(m_controlHwnd, &rect);
	InflateRect(&rect, -2, -2);
	POINT upperLeft;
	upperLeft.x = rect.left;
	upperLeft.y = rect.top;
	ClientToScreen(m_controlHwnd, &upperLeft);

	pRetVal->left = upperLeft.x;
	pRetVal->top = upperLeft.y;
	pRetVal->width = rect.right - rect.left;
	pRetVal->height = rect.bottom - rect.top;
	return S_OK;
}

// Implementation of IRawElementProviderFragment::GetEmbeddedFragmentRoots.
// Retrieves other fragment roots that may be hosted in this one.
//
IFACEMETHODIMP RootProvider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal)
{
	*pRetVal = NULL;
	return S_OK;
}

// Implementation of IRawElementProviderFragment::SetFocus.
// Responds to the control receiving focus through a UI Automation request.
// For HWND-based controls, this is handled by the host window provider.
//
IFACEMETHODIMP RootProvider::SetFocus()
{
	return S_OK;
}

// Implementation of IRawElementProviderFragment::get_FragmentRoot.
// Retrieves the root element of this fragment.
//
IFACEMETHODIMP RootProvider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal)
{
	*pRetVal = static_cast<IRawElementProviderFragmentRoot*>(this);
	AddRef();
	return S_OK;
}

// IRawElementProviderFragmentRoot implementation
//
// Implementation of IRawElementProviderFragmentRoot::ElementProviderFromPoint.
// Retrieves the IRawElementProviderFragment interface for the item at the specified 
// point (in client coordinates).
// UI Spy uses this to determine what element is under the cursor when Ctrl is pressed.
//
IFACEMETHODIMP RootProvider::ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** pRetVal)
{
	if (nullptr == pRetVal)
	{
		_ASSERT(FALSE);
		return S_OK;
	}

	POINT pt;
	pt.x = (LONG)x;
	pt.y = (LONG)y;
	ScreenToClient(m_controlHwnd, &pt);

	WLWnd* pHitTest = nullptr;
	WLWnd *pChild = m_pControl->m_pChild;
	while (nullptr != pChild)
	{
		CRect rcChild;
		pChild->GetRect(rcChild);
		if (rcChild.PtInRect(pt))
		{
			pHitTest = pChild;
			break;
		}

		pChild = pChild->m_pNext;
	}

	if (nullptr != pHitTest)
	{
		*pRetVal = static_cast<IRawElementProviderFragment*>(pHitTest->GetProvider());
		pHitTest->GetProvider()->AddRef();
	}
	else
	{
		*pRetVal = NULL;
	}

	return S_OK;
}

// Implementation of IRawElementProviderFragmentRoot::GetFocus.
// Retrieves the provider for the list item that is selected when the control gets focus.
//
IFACEMETHODIMP RootProvider::GetFocus(IRawElementProviderFragment** pRetVal)
{
	*pRetVal = NULL;
	return S_OK;
}
