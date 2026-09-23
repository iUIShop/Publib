#include "CallWPFHtml5Control.h"
#include <atlstr.h>
#include <UIAutomation.h>
#include <UIAutomationClient.h>
#pragma comment(lib, "UIAutomationCore.lib")

// UIA_ButtonControlTypeId

// <div>aaa</div>: UIA_TextControlTypeId
// <img id="IDC_IMG_EMPTY" alt="" class="empty-pic">: UIA_PaneControlTypeId
HRESULT InvokeButton(IUIAutomationElement* pButtonElement)
{
	HRESULT hr = S_OK;

	// 声明指向InvokePattern接口的指针  
	IUIAutomationInvokePattern* pInvokePattern = nullptr;

	// 检查元素是否支持InvokePattern  
	hr = pButtonElement->GetCurrentPattern(UIA_InvokePatternId, reinterpret_cast<IUnknown**>(&pInvokePattern));
	if (FAILED(hr) || !pInvokePattern)
	{
		// 错误处理：元素不支持InvokePattern  
		if (pInvokePattern)
		{
			pInvokePattern->Release(); // 如果GetCurrentPattern成功但返回了空指针，则不需要释放  
		}
		return hr;
	}

	// 调用Invoke方法来模拟点击  
	hr = pInvokePattern->Invoke();
	if (FAILED(hr))
	{
		// 错误处理：Invoke失败  
	}

	// 释放资源  
	pInvokePattern->Release();

	return hr;
}

// 通过控件类型，从WPF中得到控件,
// 注：运行时，似乎需要WPF窗口可见
int FindControlByControlType(
	IUIAutomation* pClientUIA,
	IUIAutomationElement* pRootElement,
	const long controlType,
	OUT IUIAutomationElement** ppFoundElement)
{
	int nRet = 0;
	HRESULT hr;

	do
	{
		IUIAutomationCondition* pCondition = nullptr;
		CComVariant vProp(controlType); // VT_I4(3)
		hr = pClientUIA->CreatePropertyCondition(UIA_ControlTypePropertyId, vProp, &pCondition);
		if (FAILED(hr))
		{
			nRet = UIAE_CREATE_PROP_COND;
			break;
		}

		CComPtr<IUIAutomationElementArray> pElementFound;
		hr = pRootElement->FindAll(TreeScope_Subtree, pCondition, &pElementFound);
		if (FAILED(hr) || nullptr == pElementFound)
		{
			nRet = UIAE_FIND_CONTROL;
			break;
		}

		int nEleCount = 0;
		pElementFound->get_Length(&nEleCount);
		for (int i = 0; i < nEleCount; i++)
		{
			IUIAutomationElement* pElement = nullptr;
			hr = pElementFound->GetElement(i, &pElement);
			if (FAILED(hr))
			{
				nRet = UIAE_GET_CONTROL;
				break;
			}

			BSTR bstrName;
			hr = pElement->get_CurrentName(&bstrName);
			if (FAILED(hr))
			{
				nRet = UIAE_GET_NAME;
				break;
			}

			hr = pElement->get_CurrentClassName(&bstrName);
			if (FAILED(hr))
			{
				nRet = UIAE_GET_CLASS_NAME;
				break;
			}

			*ppFoundElement = pElement;
		}
	} while (false);

	return nRet;
}

int FindControlsByControlType(
	IUIAutomation* pClientUIA,
	IUIAutomationElement* pParentElement,
	const long controlType,
	OUT std::vector<IUIAutomationElement*>* pFoundElements)
{
	int nRet = 0;
	HRESULT hr;

	do
	{
		if (nullptr == pFoundElements)
		{
			nRet = -1;
			break;
		}

		pFoundElements->clear();

		IUIAutomationCondition* pCondition = nullptr;
		CComVariant vProp(controlType); // VT_I4(3)
		hr = pClientUIA->CreatePropertyCondition(UIA_ControlTypePropertyId, vProp, &pCondition);
		if (FAILED(hr))
		{
			nRet = UIAE_CREATE_PROP_COND;
			break;
		}

		CComPtr<IUIAutomationElementArray> pElementFound;
		hr = pParentElement->FindAll(TreeScope_Subtree, pCondition, &pElementFound);
		if (FAILED(hr) || nullptr == pElementFound)
		{
			nRet = UIAE_FIND_CONTROL;
			break;
		}

		int nEleCount = 0;
		pElementFound->get_Length(&nEleCount);
		for (int i = 0; i < nEleCount; i++)
		{
			IUIAutomationElement* pElement = nullptr;
			hr = pElementFound->GetElement(i, &pElement);
			if (FAILED(hr))
			{
				nRet = UIAE_GET_CONTROL;
				break;
			}

			BSTR bstrName;
			hr = pElement->get_CurrentName(&bstrName);
			if (FAILED(hr))
			{
				nRet = UIAE_GET_NAME;
				break;
			}

			hr = pElement->get_CurrentClassName(&bstrName);
			if (FAILED(hr))
			{
				nRet = UIAE_GET_CLASS_NAME;
				break;
			}

			pFoundElements->push_back(pElement);
		}
	} while (false);

	return nRet;
}


// 从WPF中得到指定了AutomationProperties.Name属性的控件,
// 注：运行时，似乎需要WPF窗口可见
int FindControlByNameProp(
	IUIAutomation *pClientUIA,
	IUIAutomationElement* pRootElement,
	LPCWSTR lpszAutomationPropertiesName,
	OUT IUIAutomationElement** ppFoundElement)
{
	int nRet = 0;
	HRESULT hr = S_OK;

	do
	{
		// 通过控件的属性名，可以得到控件
		// 在WPF中，需为控件指定如下属性：AutomationProperties.Name="Download Button"
		// 则通过L"Download Button"，就可以找到控件
		IUIAutomationCondition* pConditionName = nullptr;
		CComVariant vPropName(lpszAutomationPropertiesName); // VT_BSTR(8)
		hr = pClientUIA->CreatePropertyCondition(UIA_NamePropertyId, vPropName, &pConditionName);
		if (FAILED(hr) || nullptr == pConditionName)
		{
			nRet = UIAE_CREATE_PROP_COND;
			break;
		}

		IUIAutomationCondition* pCondition = nullptr;
		VARIANT varID;
		varID.vt = VT_BSTR;
		varID.bstrVal = SysAllocString(L"IDC_BTN_DOWNLOAD"); // 注意：这通常是AutomationId，而不是HTML ID
		hr = pClientUIA->CreatePropertyCondition(UIA_AutomationIdPropertyId, varID, &pCondition);
		SysFreeString(varID.bstrVal);

		IUIAutomationElement* pElement = nullptr;
		hr = pRootElement->FindFirst(TreeScope_Subtree, pCondition, &pElement);
		if (FAILED(hr) || nullptr == pElement)
		{
			nRet = UIAE_FIND_CONTROL;
			break;
		}

		BSTR bstrName;
		hr = pElement->get_CurrentName(&bstrName);
		if (FAILED(hr))
		{
			nRet = UIAE_GET_NAME;
			break;
		}

		hr = pElement->get_CurrentClassName(&bstrName);
		if (FAILED(hr))
		{
			nRet = UIAE_GET_CLASS_NAME;
			break;
		}

		*ppFoundElement = pElement;
	} while (false);

	return nRet;
}

// 从HTML5中得到指定了id属性的控件,
int FindControlByHtml5IdProp(
	IUIAutomation* pClientUIA,
	IUIAutomationElement* pRootElement,
	LPCWSTR lpszAutomationPropertiesControlID,
	OUT IUIAutomationElement** ppFoundElement)
{
	int nRet = 0;
	HRESULT hr = S_OK;

	do
	{
		IUIAutomationCondition* pConditionName = nullptr;
		CComVariant vPropName(lpszAutomationPropertiesControlID); // VT_BSTR(8)
		hr = pClientUIA->CreatePropertyCondition(UIA_AutomationIdPropertyId, vPropName, &pConditionName);
		if (FAILED(hr) || nullptr == pConditionName)
		{
			nRet = UIAE_CREATE_PROP_COND;
			break;
		}

		IUIAutomationElement* pElement = nullptr;
		hr = pRootElement->FindFirst(TreeScope_Subtree, pConditionName, &pElement);
		if (FAILED(hr) || nullptr == pElement)
		{
			nRet = UIAE_FIND_CONTROL;
			break;
		}

		// Get title prop
		BSTR bstrName;
		hr = pElement->get_CurrentName(&bstrName);
		if (FAILED(hr))
		{
			nRet = UIAE_GET_NAME;
			break;
		}

		hr = pElement->get_CurrentClassName(&bstrName);
		if (FAILED(hr))
		{
			nRet = UIAE_GET_CLASS_NAME;
			break;
		}

		*ppFoundElement = pElement;
	} while (false);

	return nRet;
}

int GetElement(HWND hWnd, LPCWSTR lpszElementID, BUTTON_PROP_TYPE eElementPropType, IUIAutomationElement **ppElement)
{
	int nRet = 0;
	HRESULT hr = S_OK;
	CComPtr<IUIAutomation> pClientUIA;
	CComPtr<IUIAutomationElement> pRootElement;

	do
	{
		if (nullptr == lpszElementID || nullptr == ppElement)
		{
			nRet = UIAE_INVALID_PARAM;
			break;
		}

		if (!IsWindow(hWnd))
		{
			nRet = UIAE_INVALID_HWND;
			break;
		}

		hr = CoCreateInstance(CLSID_CUIAutomation, NULL, CLSCTX_INPROC_SERVER, IID_IUIAutomation,
			reinterpret_cast<void**>(&pClientUIA));
		if (FAILED(hr))
		{
			nRet = UIAE_IUIAutomation;
			break;
		}

		hr = pClientUIA->ElementFromHandle(hWnd, &pRootElement);
		if (FAILED(hr))
		{
			nRet = UIAE_GET_ROOT_ELEMENT;
			break;
		}

		BSTR bstrName;
		hr = pRootElement->get_CurrentName(&bstrName);

		IUIAutomationElement* pElement = nullptr;

		if (eElementPropType == BUTTON_PROP_TYPE_WPF_NAME)
		{
			//nRet = FindControlByControlType(pClientUIA, pRootElement, UIA_ButtonControlTypeId, &pElement);
			nRet = FindControlByNameProp(pClientUIA, pRootElement, lpszElementID, &pElement);
		}
		else if (eElementPropType == BUTTON_PROP_TYPE_HTML_CONTROL_ID)
		{
			nRet = FindControlByHtml5IdProp(pClientUIA, pRootElement, lpszElementID, &pElement);
		}
		if (nRet != 0)
		{
			break;
		}

		*ppElement = pElement;

	} while (false);

	return nRet;
}

int CallButton(HWND hWnd, LPCWSTR lpszButton, BUTTON_PROP_TYPE eButtonPropType)
{
	int nRet = 0;
	HRESULT hr = S_OK;
	CComPtr<IUIAutomation> pClientUIA;
	CComPtr<IUIAutomationElement> pRootElement;

	do
	{
		IUIAutomationElement* pElement = nullptr;

		GetElement(hWnd, lpszButton, eButtonPropType, &pElement);

		// 模拟点击按钮
		InvokeButton(pElement);

	} while (false);

	return nRet;
}

CHtml5Automation::CHtml5Automation()
{
}

CHtml5Automation::~CHtml5Automation()
{
}

int CHtml5Automation::Init(HWND hWndBrowser)
{
	m_hBrowserWnd = hWndBrowser;

	int nRet = 0;
	HRESULT hr = S_OK;

	do
	{
		if (!IsWindow(m_hBrowserWnd))
		{
			nRet = UIAE_INVALID_HWND;
			break;
		}

		hr = CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER, __uuidof(IUIAutomation),
			reinterpret_cast<void**>(&m_pClientUIA));
		if (FAILED(hr))
		{
			nRet = UIAE_IUIAutomation;
			break;
		}

		// GetRootElement是得到桌面
		hr = m_pClientUIA->ElementFromHandle(m_hBrowserWnd, &m_pRootElement);
		if (FAILED(hr))
		{
			nRet = UIAE_GET_ROOT_ELEMENT;
			break;
		}

#ifdef _DEBUG
		BSTR bstrName;
		hr = pRootElement->get_CurrentName(&bstrName);
		int n = 0;
#endif
	} while (false);

	return nRet;
}

HWND CHtml5Automation::GetHwnd()
{
	return m_hBrowserWnd;
}

IUIAutomation* CHtml5Automation::GetAutomation()
{
	return m_pClientUIA;
}

int CHtml5Automation::BuildControlTree()
{
	// 获取树遍历器
	IUIAutomationTreeWalker* pTreeWalker = nullptr;
	m_pClientUIA->get_ControlViewWalker(&pTreeWalker);

	// 从根元素开始遍历
	IUIAutomationElement* pCurrentElement = m_pRootElement;

	while (pCurrentElement != NULL)
	{
		IUIAutomationElement *pChildElement = nullptr;

		// 移动到下一个元素
		pTreeWalker->GetFirstChildElement(pCurrentElement, &pChildElement);
		if (pChildElement == nullptr)
		{
			int n = 9;
		}
		GetElementProp(pChildElement, nullptr);

		pCurrentElement = pChildElement;
	}

	return 0;
}

int CHtml5Automation::BuildContentTree()
{
	return 0;
}

int CHtml5Automation::BuildRawTree()
{
	HRESULT hr = S_OK;
	int nRet = 0;

	do
	{
		CComPtr<IUIAutomationCondition> spConditionRawView;
		hr = m_pClientUIA->get_RawViewCondition(&spConditionRawView);
		if (FAILED(hr))
		{
			nRet = -2;
			break;
		}

		CComPtr<IUIAutomationElementArray> pElementFound;
		hr = m_pRootElement->FindAll(TreeScope_Subtree, spConditionRawView, &pElementFound);
		if (FAILED(hr) || nullptr == pElementFound)
		{
			nRet = UIAE_FIND_CONTROL;
			break;
		}

		int nEleCount = 0;
		pElementFound->get_Length(&nEleCount);
		for (int i = 0; i < nEleCount; i++)
		{
			IUIAutomationElement* pElement = nullptr;
			hr = pElementFound->GetElement(i, &pElement);
			if (FAILED(hr))
			{
				nRet = UIAE_GET_CONTROL;
				break;
			}

			GetElementProp(pElement, nullptr);
		}
	} while (false);

	return 0;
}

int CHtml5Automation::BuildTrueTreeRecursive(IUIAutomationElement* pParentElement)
{
	HRESULT hr = S_OK;
	int nRet = 0;

	IUIAutomationTreeWalker* pWalker = nullptr;
	m_pClientUIA->get_RawViewWalker(&pWalker);

	IUIAutomationElement* pChild = nullptr;
	pWalker->GetFirstChildElement(pParentElement, &pChild);
	CElementProp eleProp;
	GetElementProp(pChild, &eleProp);

	BuildTrueTreeRecursive(pChild);

	return 0;
}
int CHtml5Automation::BuildTrueTree()
{
	HRESULT hr = S_OK;
	int nRet = 0;

	do
	{


		nRet = BuildTrueTreeRecursive(m_pRootElement);

		// 此条件，可以遍历到窗口、文档、按钮、图像、文本等元素，
		// 但无法遍历到“组”这种元素，即使组元素有AutomationId.
		// 通过AutomationId条件，也拿不到组元素。
		IUIAutomationCondition* pCondition = nullptr;
		hr = m_pClientUIA->CreateTrueCondition(&pCondition);
		if (FAILED(hr))
		{
			nRet = -2;
			break;
		}

		CComPtr<IUIAutomationElementArray> pElementFound;
		hr = m_pRootElement->FindAll(TreeScope_Subtree, pCondition, &pElementFound);
		if (FAILED(hr) || nullptr == pElementFound)
		{
			nRet = UIAE_FIND_CONTROL;
			break;
		}

		int nEleCount = 0;
		pElementFound->get_Length(&nEleCount);
		for (int i = 0; i < nEleCount; i++)
		{
			IUIAutomationElement* pElement = nullptr;
			hr = pElementFound->GetElement(i, &pElement);
			if (FAILED(hr))
			{
				nRet = UIAE_GET_CONTROL;
				break;
			}

			GetElementProp(pElement, nullptr);
		}
	} while (false);

	return 0;
}

int CHtml5Automation::ElementFromPoint(POINT pt, IUIAutomationElement** ppElement)
{
	HRESULT hr = m_pClientUIA->ElementFromPoint(pt, ppElement);
	return FAILED(hr) ? -2 : 0;
}

int CHtml5Automation::GetElement(LPCWSTR lpszElementID, IUIAutomationElement** ppElement)
{
	HRESULT hr = S_OK;
	int nRet = 0;

	do
	{
		IUIAutomationCondition* pCondition = nullptr;
		CComVariant vProp(lpszElementID); // VT_I4(3)
		hr = m_pClientUIA->CreatePropertyCondition(UIA_AutomationIdPropertyId, vProp, &pCondition);
		if (FAILED(hr))
		{
			nRet = UIAE_CREATE_PROP_COND;
			break;
		}

		CComPtr<IUIAutomationElementArray> pElementFound;
		hr = m_pRootElement->FindAll(TreeScope_Subtree, pCondition, &pElementFound);
		if (FAILED(hr) || nullptr == pElementFound)
		{
			nRet = UIAE_FIND_CONTROL;
			break;
		}

		int nEleCount = 0;
		pElementFound->get_Length(&nEleCount);
		for (int i = 0; i < nEleCount; i++)
		{
			IUIAutomationElement* pElement = nullptr;
			hr = pElementFound->GetElement(i, &pElement);
			if (FAILED(hr))
			{
				nRet = UIAE_GET_CONTROL;
				break;
			}

			// title
			BSTR bstrName;
			hr = pElement->get_CurrentName(&bstrName);
			if (FAILED(hr))
			{
				nRet = UIAE_GET_NAME;
				break;
			}

			*ppElement = pElement;
		}
	} while (false);


	//// 获取子元素的数量  
	//int childCount;
	//m_pRootElement->FindAll( GetCurrentChildCount(&childCount);

	//// 遍历子元素  
	//for (int i = 0; i < childCount; ++i)
	//{
	//	IUIAutomationElement* pChild = nullptr;
	//	pStartElement->GetChildElement(i, &pChild);

	//	// 递归遍历或处理pChild  
	//	TraverseUIAutomationTree(pChild);

	//	// 释放子元素  
	//	pChild->Release();
	//}  

	//// 注意：这里没有直接处理<div>，因为<div>在UI Automation中可能不是以这种方式表示的  
	//	// 你可能需要基于元素的属性（如Name, ControlType, AutomationId等）来识别它  

	//	// 示例：检查ControlType（这通常不会直接对应于HTML标签）  
	//UIA_ControlType controlType;
	//pStartElement->GetCurrentControlType(&controlType);
	//if (controlType == UIA_PaneControlTypeId) {
	//	// 这里只是一个示例，实际上Pane可能并不对应于<div>  
	//	// 你可能需要根据实际情况添加更多的逻辑来识别元素  
	//}

	return 0;
}

int CHtml5Automation::GetElementByControlType(long lControlType, LPCWSTR lpszText, BOOL bEqual, IUIAutomationElement** ppElement)
{
	HRESULT hr = S_OK;
	int nRet = 0;

	do
	{
		IUIAutomationCondition* pCondition = nullptr;
		CComVariant vProp(lControlType); // VT_I4(3)
		hr = m_pClientUIA->CreatePropertyCondition(UIA_ControlTypePropertyId, vProp, &pCondition);
		if (FAILED(hr))
		{
			nRet = UIAE_CREATE_PROP_COND;
			break;
		}

		CComPtr<IUIAutomationElementArray> pElementFound;
		hr = m_pRootElement->FindAll(TreeScope_Subtree, pCondition, &pElementFound);
		if (FAILED(hr) || nullptr == pElementFound)
		{
			nRet = UIAE_FIND_CONTROL;
			break;
		}

		int nEleCount = 0;
		pElementFound->get_Length(&nEleCount);
		for (int i = 0; i < nEleCount; i++)
		{
			IUIAutomationElement* pElement = nullptr;
			hr = pElementFound->GetElement(i, &pElement);
			if (FAILED(hr))
			{
				nRet = UIAE_GET_CONTROL;
				break;
			}

			// title
			BSTR bstrName;
			hr = pElement->get_CurrentName(&bstrName);
			if (FAILED(hr))
			{
				nRet = UIAE_GET_NAME;
				break;
			}

			CStringW strTitle = bstrName;
			if (bEqual)
			{
				if (strTitle != lpszText)
				{
					nRet = -10;
					continue;
				}
			}
			else
			{
				if (strTitle.Find(lpszText) < 0)
				{
					nRet = -10;
					continue;
				}
			}

			GetElementProp(pElement, nullptr);

			*ppElement = pElement;
			break;
		}
	} while (false);


	//// 获取子元素的数量  
	//int childCount;
	//m_pRootElement->FindAll( GetCurrentChildCount(&childCount);

	//// 遍历子元素  
	//for (int i = 0; i < childCount; ++i)
	//{
	//	IUIAutomationElement* pChild = nullptr;
	//	pStartElement->GetChildElement(i, &pChild);

	//	// 递归遍历或处理pChild  
	//	TraverseUIAutomationTree(pChild);

	//	// 释放子元素  
	//	pChild->Release();
	//}  

	//// 注意：这里没有直接处理<div>，因为<div>在UI Automation中可能不是以这种方式表示的  
	//	// 你可能需要基于元素的属性（如Name, ControlType, AutomationId等）来识别它  

	//	// 示例：检查ControlType（这通常不会直接对应于HTML标签）  
	//UIA_ControlType controlType;
	//pStartElement->GetCurrentControlType(&controlType);
	//if (controlType == UIA_PaneControlTypeId) {
	//	// 这里只是一个示例，实际上Pane可能并不对应于<div>  
	//	// 你可能需要根据实际情况添加更多的逻辑来识别元素  
	//}

	return 0;
}

int CHtml5Automation::GetElementsByControlType(long lControlType, LPCWSTR lpszText, BOOL bEqual, std::vector<IUIAutomationElement*>* pElements)
{
	HRESULT hr = S_OK;
	int nRet = 0;

	do
	{
		if (nullptr == pElements)
		{
			nRet = -1;
			break;
		}

		pElements->clear();

		IUIAutomationCondition* pCondition = nullptr;
		CComVariant vProp(lControlType); // VT_I4(3)
		hr = m_pClientUIA->CreatePropertyCondition(UIA_ControlTypePropertyId, vProp, &pCondition);
		if (FAILED(hr))
		{
			nRet = UIAE_CREATE_PROP_COND;
			break;
		}

		CComPtr<IUIAutomationElementArray> pElementFound;
		hr = m_pRootElement->FindAll(TreeScope_Subtree, pCondition, &pElementFound);
		if (FAILED(hr) || nullptr == pElementFound)
		{
			nRet = UIAE_FIND_CONTROL;
			break;
		}

		int nEleCount = 0;
		pElementFound->get_Length(&nEleCount);
		for (int i = 0; i < nEleCount; i++)
		{
			IUIAutomationElement* pElement = nullptr;
			hr = pElementFound->GetElement(i, &pElement);
			if (FAILED(hr))
			{
				nRet = UIAE_GET_CONTROL;
				break;
			}

			// title
			BSTR bstrName;
			hr = pElement->get_CurrentName(&bstrName);
			if (FAILED(hr))
			{
				nRet = UIAE_GET_NAME;
				break;
			}

			CStringW strTitle = bstrName;
			if (bEqual)
			{
				if (strTitle != lpszText)
				{
					nRet = -10;
					continue;
				}
			}
			else
			{
				if (strTitle.Find(lpszText) < 0)
				{
					nRet = -10;
					continue;
				}
			}

			GetElementProp(pElement, nullptr);

			pElements->push_back(pElement);
		}
	} while (false);

	return nRet;
}

int CHtml5Automation::GetElementByPoint(IUIAutomationElement** ppElement)
{
	POINT pt;
	GetCursorPos(&pt);
	m_pClientUIA->ElementFromPoint(pt, ppElement);

	return 0;
}

int CHtml5Automation::GetElementProp(IUIAutomationElement* pElement, CElementProp *pEleProp)
{
	HRESULT hr;
	int nRet = 0;

	do
	{
		if (nullptr == pElement)
		{
			nRet = -1;
			break;
		}
		// title
		BSTR bstrName;
		hr = pElement->get_CurrentName(&bstrName);
		if (FAILED(hr))
		{
			nRet = UIAE_GET_NAME;
			break;
		}

		CONTROLTYPEID eType;
		pElement->get_CurrentControlType(&eType);
		if (eType == UIA_GroupControlTypeId)
		{
			int n = 0;
		}

		BSTR bstrLocalizedControlType;
		hr = pElement->get_CurrentLocalizedControlType(&bstrLocalizedControlType);
		if (FAILED(hr))
		{
			nRet = UIAE_GET_NAME;
			break;
		}

		BSTR bstrAutomationId;
		hr = pElement->get_CurrentAutomationId(&bstrAutomationId);
		if (FAILED(hr))
		{
			nRet = UIAE_GET_NAME;
			break;
		}

		BSTR bstrItemType;
		hr = pElement->get_CurrentItemType(&bstrItemType);
		if (FAILED(hr))
		{
			nRet = UIAE_GET_NAME;
			break;
		}

		BSTR bstrFrameworkId;
		hr = pElement->get_CurrentFrameworkId(&bstrFrameworkId);
		if (FAILED(hr))
		{
			nRet = UIAE_GET_NAME;
			break;
		}

		BSTR bstrItemStatus;
		hr = pElement->get_CurrentItemStatus(&bstrItemStatus);
		if (FAILED(hr))
		{
			nRet = UIAE_GET_NAME;
			break;
		}

		BSTR bstrAriaProperties;
		hr = pElement->get_CurrentAriaProperties(&bstrAriaProperties);
		if (FAILED(hr))
		{
			nRet = UIAE_GET_NAME;
			break;
		}

		BSTR bstrProviderDescription;
		hr = pElement->get_CurrentProviderDescription(&bstrProviderDescription);
		if (FAILED(hr))
		{
			nRet = UIAE_GET_NAME;
			break;
		}

		BSTR bstrClassName;
		hr = pElement->get_CurrentClassName(&bstrClassName);
		if (FAILED(hr))
		{
			nRet = UIAE_GET_CLASS_NAME;
			break;
		}

		BSTR bstrHelpText;
		hr = pElement->get_CurrentHelpText(&bstrHelpText);
		if (FAILED(hr))
		{
			nRet = UIAE_GET_CLASS_NAME;
			break;
		}

		CElementProp eleProp;
		eleProp.m_strName = bstrName;
		eleProp.m_strAutomationId = bstrAutomationId;
		eleProp.m_ControlType = eType;
		eleProp.m_strLocalizedControlType = bstrLocalizedControlType;
		eleProp.m_strFrameworkId = bstrFrameworkId;
		eleProp.m_strItemType = bstrItemType;
		eleProp.m_strItemStatus = bstrItemStatus;
		eleProp.m_strClassName = bstrClassName;
		eleProp.m_strHelpText = bstrHelpText;

		if (nullptr != pEleProp)
		{
			*pEleProp = eleProp;
		}

		OnGetElementProp(&eleProp);

	} while (false);

	return nRet;
}

void CHtml5Automation::SetOnGetElementPropFunc(OnGetElementPropFunc funcCallback, void *pArg)
{
	m_pOnGetElementPropFunc = funcCallback;
	m_pOnGetElementPropArg = pArg;

}

int CHtml5Automation::OnGetElementProp(const CElementProp* pEleProp)
{
	if (nullptr != m_pOnGetElementPropFunc)
	{
		m_pOnGetElementPropFunc(pEleProp, m_pOnGetElementPropArg);
	}
	return 0;
}
