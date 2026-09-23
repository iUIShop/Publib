/*************************************************************************************************
* Description: Declarations for the sample UI Autoamtion provider implementations.
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

#include <ole2.h>
#include <UIAutomation.h>

#include <assert.h>
#include "../UIA_Control/CustomControl.h"

class WLWndProvider;

// “Fragment” 的含义
// 在 UIA 中，一个复杂控件，例如listbox，对应FragmentRoot. 而listbox的item对应Fragment.
// 每个 Fragment 必须有一个 IRawElementProviderFragmentRoot 作为根。
// IRawElementProviderFragmentRoot 是复杂控件（例如listbox）在 UIA 树中的根节点，用于管理子元素的导航和结构。
// 对于普通控件，例如按钮，也需要实现IRawElementProviderFragment，因为它提供了导航功能和返回控件坐标的功能。
class RootProvider : public IRawElementProviderSimple,
	public IRawElementProviderFragment,
	public IRawElementProviderFragmentRoot
{
public:

	// Constructor/destructor.
	RootProvider(WLWnd* pControl);

	// IUnknown methods
	IFACEMETHODIMP_(ULONG) AddRef();
	IFACEMETHODIMP_(ULONG) Release();
	IFACEMETHODIMP QueryInterface(REFIID riid, void** ppInterface);

	// IRawElementProviderSimple methods
	IFACEMETHODIMP get_ProviderOptions(ProviderOptions* pRetVal);
	IFACEMETHODIMP GetPatternProvider(PATTERNID iid, IUnknown** pRetVal);
	IFACEMETHODIMP GetPropertyValue(PROPERTYID idProp, VARIANT* pRetVal);
	IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple** pRetVal);

	// IRawElementProviderFragment methods
	IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal);
	IFACEMETHODIMP GetRuntimeId(SAFEARRAY** pRetVal);
	IFACEMETHODIMP get_BoundingRectangle(UiaRect* pRetVal);
	IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal);
	IFACEMETHODIMP SetFocus();
	IFACEMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal);

	// IRawElementProviderFragmenRoot methods
	IFACEMETHODIMP ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** pRetVal);
	IFACEMETHODIMP GetFocus(IRawElementProviderFragment** pRetVal);

private:
	virtual ~RootProvider();

	// Ref counter for this COM object.
	ULONG m_refCount = 0;

	// Parent control.
	HWND m_controlHwnd = nullptr;
	WLWnd* m_pControl = nullptr;
};

class WLWndProvider : public IRawElementProviderSimple,
	public IRawElementProviderFragment
{
public:

	// Constructor / destructor
	WLWndProvider(WLWnd* pControl);

	// IUnknown methods
	IFACEMETHODIMP_(ULONG) AddRef();
	IFACEMETHODIMP_(ULONG) Release();
	IFACEMETHODIMP QueryInterface(REFIID riid, void** ppInterface);

	// IRawElementProviderSimple methods
	IFACEMETHODIMP get_ProviderOptions(ProviderOptions* pRetVal);
	IFACEMETHODIMP GetPatternProvider(PATTERNID iid, IUnknown** pRetVal);
	IFACEMETHODIMP GetPropertyValue(PROPERTYID idProp, VARIANT* pRetVal);
	IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple** pRetVal);

	// IRawElementProviderFragment methods
	IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal);
	IFACEMETHODIMP GetRuntimeId(SAFEARRAY** pRetVal);
	IFACEMETHODIMP get_BoundingRectangle(UiaRect* pRetVal);
	IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal);
	IFACEMETHODIMP SetFocus();
	IFACEMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal);

	// Various methods
	void NotifyItemAdded();
	void NotifyItemRemoved();

private:
	virtual ~WLWndProvider();

	// Ref Counter for this COM object
	ULONG m_refCount = 0;

	// Pointers to the owning item control and list control.
	WLWnd* m_pBindControl = nullptr;
	Canvas* m_pParentControl = nullptr;
};
