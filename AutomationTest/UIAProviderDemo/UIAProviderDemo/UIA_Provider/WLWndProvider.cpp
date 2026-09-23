
/*************************************************************************************************
 * Description: Implementation of the WLWndProvider class, which implements a
 * UI Automation provider for a list item in a custom control.
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
#include "UIAProviders.h"

WLWndProvider::WLWndProvider(WLWnd* pControl) : m_refCount(1)
{
	m_pBindControl = pControl;
	m_pParentControl = pControl->GetParent();
}

WLWndProvider::~WLWndProvider()
{
}


// IUnknown implementation.

IFACEMETHODIMP_(ULONG) WLWndProvider::AddRef()
{
	return InterlockedIncrement(&m_refCount);
}

IFACEMETHODIMP_(ULONG) WLWndProvider::Release()
{
	long val = InterlockedDecrement(&m_refCount);
	if (val == 0)
	{
		delete this;
	}
	return val;
}

IFACEMETHODIMP WLWndProvider::QueryInterface(REFIID riid, void** ppInterface)
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
// Implementation of IRawElementProviderSimple::GetProviderOptions.
//
IFACEMETHODIMP WLWndProvider::get_ProviderOptions(ProviderOptions* pRetVal)
{
	*pRetVal = ProviderOptions_ServerSideProvider;
	return S_OK;
}

// Implementation of IRawElementProviderSimple::GetPatternProvider.
// Gets the object that supports the specified pattern.
//
IFACEMETHODIMP WLWndProvider::GetPatternProvider(PATTERNID patternId, IUnknown** pRetVal)
{
	*pRetVal = NULL;
	return S_OK;
}
// Implementation of IRawElementProviderSimple::GetPropertyValue.
// Gets custom properties. Because list items are not directly hosted in an HWND, 
// more properties should be supported here than for the list box itself. 
//
IFACEMETHODIMP WLWndProvider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal)
{
	if (propertyId == UIA_AutomationIdPropertyId)
	{
		pRetVal->vt = VT_BSTR;
		int Id = m_pBindControl->GetId();
		// Convert int to BSTR.
		WCHAR idString[3];
		swprintf_s(idString, 3, L"%d", Id);
		pRetVal->bstrVal = SysAllocString(idString);
	}
	else if (propertyId == UIA_NamePropertyId)
	{
		pRetVal->vt = VT_BSTR;
		pRetVal->bstrVal = SysAllocString(m_pBindControl->GetName());
	}
	else if (propertyId == UIA_ControlTypePropertyId)
	{
		pRetVal->vt = VT_I4;
		pRetVal->lVal = UIA_ListItemControlTypeId;
	}
	// HasKeyboardFocus is true if the list has focus, and this item is selected.
	else if (propertyId == UIA_HasKeyboardFocusPropertyId)
	{
		pRetVal->vt = VT_BOOL;
		pRetVal->boolVal = VARIANT_TRUE;
	}
	else if (propertyId == UIA_IsControlElementPropertyId)
	{
		pRetVal->vt = VT_BOOL;
		pRetVal->boolVal = VARIANT_TRUE;
	}
	else if (propertyId == UIA_IsContentElementPropertyId)
	{
		pRetVal->vt = VT_BOOL;
		pRetVal->boolVal = VARIANT_TRUE;
	}
	else if (propertyId == UIA_IsKeyboardFocusablePropertyId)
	{
		pRetVal->vt = VT_BOOL;
		pRetVal->boolVal = VARIANT_TRUE;
	}
	else if (propertyId == UIA_ItemStatusPropertyId)
	{
		pRetVal->vt = VT_BSTR;
		pRetVal->bstrVal = SysAllocString(L"Online");
	}
	else
	{
		pRetVal->vt = VT_EMPTY;
	}
	return S_OK;
}

// Implementation of IRawElementProviderSimple::get_HostRawElementProvider.
// Gets the UI Automation provider for the host window. 
// Return NULL. because the list items are not directly hosted in a window.
//
IFACEMETHODIMP WLWndProvider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal)
{
	*pRetVal = NULL;
	return S_OK;
}

// IRawElementProviderFragment implementation.
//
// Implementation of IRawElementProviderFragment::Navigate.
// Enables UI Automation to locate the element in the tree.
//
IFACEMETHODIMP WLWndProvider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal)
{
	IRawElementProviderFragment* pFrag = NULL;
	switch (direction)
	{
	case NavigateDirection_Parent:
		pFrag = m_pParentControl->GetProvider();
		break;

	case NavigateDirection_NextSibling:
	{
		WLWnd* pNext = m_pBindControl->m_pNext;
		if (nullptr == pNext)
		{
			pFrag = NULL;
			break;
		}
		pFrag = pNext->GetProvider();
		break;
	}

	case NavigateDirection_PreviousSibling:
	{
		WLWnd* pPrev = m_pBindControl->m_pPrev;
		if (nullptr == pPrev)
		{
			pFrag = NULL;
			break;
		}
		pFrag = pPrev->GetProvider();
		break;
	}
	}
	*pRetVal = pFrag;
	if (pFrag != NULL)
	{
		pFrag->AddRef();
	}
	return S_OK;
}

// Implementation of IRawElementProviderFragment::GetRuntimeId.
// Gets the runtime identifier. This is an array consisting of UiaAppendRuntimeId, 
// which makes the ID unique among instances of the control, and the Automation Id.
//
IFACEMETHODIMP WLWndProvider::GetRuntimeId(SAFEARRAY** pRetVal)
{
	int id = m_pBindControl->GetId();
	int rId[] = { UiaAppendRuntimeId, id };

	SAFEARRAY* psa = SafeArrayCreateVector(VT_I4, 0, 2);
	for (LONG i = 0; i < 2; i++)
	{
		SafeArrayPutElement(psa, &i, &(rId[i]));
	}
	*pRetVal = psa;
	return S_OK;
}

// Implementation of IRawElementProviderFragment::get_BoundingRectangle.
// 返回屏幕坐标
IFACEMETHODIMP WLWndProvider::get_BoundingRectangle(UiaRect* pRetVal)
{
	CRect rc;
	m_pBindControl->GetRect(rc);

	IRawElementProviderFragment* pParent = m_pParentControl->GetProvider();
	UiaRect parentRect;
	HRESULT hr = pParent->get_BoundingRectangle(&parentRect);
	if (SUCCEEDED(hr))
	{
		pRetVal->left = parentRect.left + rc.left;
		pRetVal->top = parentRect.top + rc.top;
		pRetVal->width = rc.Width();
		pRetVal->height = rc.Height();
	}
	return hr;
}


// Implementation of IRawElementProviderFragment::GetEmbeddedFragmentRoots.
// Retrieves any fragment roots that may be hosted in this element.
//
IFACEMETHODIMP WLWndProvider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal)
{
	*pRetVal = NULL;
	return S_OK;
}

// Implementation of IRawElementProviderFragment::SetFocus.
// Responds to the control receiving focus through a UI Automation request.
//
IFACEMETHODIMP WLWndProvider::SetFocus()
{
	return S_OK;
}

// Implementation of IRawElementProviderFragment::get_FragmentRoot.
// Retrieves the root element of this fragment.
//
IFACEMETHODIMP WLWndProvider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal)
{
	IRawElementProviderFragmentRoot* pRoot = this->m_pParentControl->GetProvider();
	if (pRoot == NULL)
	{
		return E_FAIL;
	}
	pRoot->AddRef();
	*pRetVal = pRoot;
	return S_OK;
}

// Raises an event when an item is added to the list.
//
void WLWndProvider::NotifyItemAdded()
{
	if (UiaClientsAreListening())
	{
		UiaRaiseStructureChangedEvent(this, StructureChangeType_ChildAdded, NULL, 0);
	}
}

// Raises an event when an item is removed from the list.
//
// StructureType_ChildRemoved is unusual in that it is raised on the parent provider,
// since the child provider may not exist anymore, but it uses the child's runtime ID.
void WLWndProvider::NotifyItemRemoved()
{
	if (UiaClientsAreListening())
	{
		RootProvider* canvasProvider = m_pParentControl->GetProvider();

		// Construct the partial runtime ID for the removed child
		int id = m_pBindControl->GetId();
		int rId[] = { UiaAppendRuntimeId, id };

		UiaRaiseStructureChangedEvent(canvasProvider, StructureChangeType_ChildRemoved, rId, ARRAYSIZE(rId));
	}
}
