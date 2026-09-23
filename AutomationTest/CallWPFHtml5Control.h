#pragma once

#include <windows.h>
#include <UIAutomationClient.h>
#include <atlcomcli.h>
#include <vector>
#include <string>
#include <atltypes.h>


enum UIAUTOMATION_ERROR
{
	UIAE_INVALID_PARAM = -1,
	UIAE_INVALID_HWND = -2,
	UIAE_COM_INIT = -3,
	UIAE_IUIAutomation = -4,
	UIAE_GET_ROOT_ELEMENT = -5,
	UIAE_FIND_CONTROL_BY_PROP_NAME = -6,
	UIAE_CREATE_PROP_COND = -7,
	UIAE_FIND_CONTROL = -8,
	UIAE_GET_CONTROL = -9,
	UIAE_GET_NAME = -10,
	UIAE_GET_CLASS_NAME = -11,
};

enum BUTTON_PROP_TYPE
{
	BUTTON_PROP_TYPE_WPF_NAME = 1,
	BUTTON_PROP_TYPE_HTML_CONTROL_ID = 2
};

class CElementProp
{
public:
	std::wstring m_strAcceleratorKey;
	std::wstring m_strAccessKey;
	std::wstring m_strAutomationId;
	CRect m_rcBoundingRectangle;
	std::wstring m_strClassName;
	UINT m_ControlType = 0;
	std::wstring m_strFrameworkId;
	BOOL m_bHasKeyboardFocus = FALSE;
	std::wstring m_strHelpText;
	BOOL m_bContentElement = FALSE;
	BOOL m_bControlElement = FALSE;
	BOOL m_bEnabled = FALSE;
	BOOL m_bKeyboardFocusable = FALSE;
	BOOL m_bOffscreen = FALSE;
	BOOL m_bPassword = FALSE;
	BOOL m_bRequiredForForm = FALSE;
	std::wstring m_strItemStatus;
	std::wstring m_strItemType;
	UINT m_LabeledBy = 0;
	std::wstring m_strLocalizedControlType;
	std::wstring m_strName;
	HWND m_hWnd = nullptr;
	UINT m_Orientation;
	UINT m_uPid = 0;
};

typedef int (*OnGetElementPropFunc)(const CElementProp* pEleProp, void *pArg);

class CHtml5Automation
{
public:
	CHtml5Automation();
	virtual ~CHtml5Automation();

public:
	int Init(HWND hWndBrowser);
	HWND GetHwnd();
	IUIAutomation* GetAutomation();

	int BuildControlTree();
	int BuildContentTree();
	int BuildRawTree();
	int BuildTrueTree();
	int ElementFromPoint(POINT pt, IUIAutomationElement** ppElement);

	int GetElement(LPCWSTR lpszElementID, IUIAutomationElement** ppElement);
	// lControlType: UIAutomationClient.h line:1318
	int GetElementByControlType(long lControlType, LPCWSTR lpszText, BOOL bEqual, IUIAutomationElement** ppElement);
	int GetElementsByControlType(long lControlType, LPCWSTR lpszText, BOOL bEqual, std::vector<IUIAutomationElement*> *pElements);

	int GetElementByPoint(IUIAutomationElement** ppElement);

	int CallButton(LPCWSTR lpszButton, BUTTON_PROP_TYPE eButtonPropType);

	int GetElementProp(IUIAutomationElement* pElement, CElementProp* pEleProp);
	void SetOnGetElementPropFunc(OnGetElementPropFunc funcCallback, void *pArg);

protected:
	int OnGetElementProp(const CElementProp *pEleProp);
	int BuildTrueTreeRecursive(IUIAutomationElement* pParentElement);

protected:
	HWND m_hBrowserWnd = nullptr;
	CComPtr<IUIAutomation> m_pClientUIA;
	CComPtr<IUIAutomationElement> m_pRootElement;
	OnGetElementPropFunc m_pOnGetElementPropFunc = nullptr;
	void* m_pOnGetElementPropArg = nullptr;
};

int GetElement(HWND hWnd, LPCWSTR lpszElementID, BUTTON_PROP_TYPE eElementPropType, IUIAutomationElement** ppElement);

int CallButton(HWND hWnd, LPCWSTR lpszButton, BUTTON_PROP_TYPE eButtonPropType);
HRESULT InvokeButton(IUIAutomationElement* pButtonElement);
int FindControlsByControlType(
	IUIAutomation* pClientUIA,
	IUIAutomationElement* pParentElement,
	const long controlType,
	OUT std::vector<IUIAutomationElement*>* pFoundElements);
