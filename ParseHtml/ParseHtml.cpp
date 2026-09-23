#include "ParseHtml.h"
#include <atlbase.h>
#include <MsHTML.h>
#include <exdisp.h> // IWebBrowser2
#include <comutil.h>
#include <atlsafe.h>
#include <iostream>
#include <vector>
#include "../String/String.h"
#include "../FileOperation/FileReadWrite.h"
#include <atlstr.h>

// 空白标签：即由空格和tab组成的标签。
// VML标签：https://learn.microsoft.com/en-us/windows/win32/vml/msdn-online-vml-introduction?redirectedfrom=MSDN
//          这是一种从word中拖出一段混排的图文混排的html中的标签。从IE9开始被废弃，并不被现代浏览器支持。如<v:imagedata>、<o:lock>。
//          使用mshtml库的时候，只需要使用imagedata、lock当成标签名就行，前缀不需要。
//			在Word中，选中一段混排的图文，切记从文本上拖出，拖入到一个接受程序，接受程序处理"HTML Format"格式的数据，并保存到文件中
//			大概率就得到一个包含VML标签的html文档，但有些混排的图文不包含VML标签。这样的接收程序，见《Drag & Drop》中的DropTarget的Demo。
// 下面这几个方法，都不包含空白标签，但包含VML标签。
// IHTMLDocument2.get_all: 可拿到所有子元素和嵌套子元素，包括集合始终包括html标记、head标记和body标记的一个元素对象，而不管这些标记是否存在于文档中。
// IHTMLElement.get_all: 可以拿到它所有子元素和嵌套子元素。
// IHTMLElement.get_children: 可以拿到它直接孩子，不包含嵌套孩子。
// IHTMLDocument3.getElementsByTagName: 可以拿到所有指定名字的子元素和嵌套子元素。
// 	 注：如果IHTMLDocument2是通过write接口写入的Html流，则通过
//		IPersistFile接口的Save是无法保存修改后的Html dom。目前测试下来，通过
//		把修改后的Html Dom再转为String，是可以的。而通过IWebBrowser2的Naviate接口加载的本地html文件，
//		可以通过IPersistFile的Save来保存修改后的Dom.


int ParseIHTMLElementCollection(IHTMLElementCollection* pHTMLElementCollection, LPCWSTR lpszTagName, std::vector<IHTMLElement*>* pvHtmlElements)
{
	int nRet = 0;

	do
	{
		if (nullptr == pHTMLElementCollection || nullptr == pvHtmlElements)
		{
			nRet = -1;
			break;
		}

		long lCount = 0;
		HRESULT hr = pHTMLElementCollection->get_length(&lCount);
		if (FAILED(hr))
		{
			nRet = -2;
			break;
		}

		for (long i = 0; i < lCount; ++i)
		{
			// 这里必须先拿到IDispatch类型指针，然后再通过IDispatch拿到IHTMLElement类型指针，
			// 才可以使用IHTMLElement，不能直接通过item强转拿IHTMLElement指针。
			CComPtr<IDispatch> pElemDisp;
			pHTMLElementCollection->item(CComVariant(i), CComVariant(i), (IDispatch**)&pElemDisp);

			// 这里不能使用CComPtr，否则在函数返回后，就被Release了，导致
			// 存入vector中的IHTMLElement是无效指针。
			IHTMLElement* pHtmlElement = nullptr;
			hr = pElemDisp->QueryInterface(IID_IHTMLElement, (void**)&pHtmlElement);

			// 拿到标签名
			CComBSTR tagName;
			pHtmlElement->get_tagName(&tagName);

			CStringW strTagName = lpszTagName;
			if (strTagName.CompareNoCase(tagName) == 0)
			{
				pvHtmlElements->push_back(pHtmlElement);
			}
		}
	} while (false);

	return nRet;
}

int ParseIHTMLElement(IHTMLElement* pHTMLElement, LPCWSTR lpszTagName, std::vector<IHTMLElement*>* pvHtmlElements)
{
	int nRet = 0;
	HRESULT hr = S_OK;

	do
	{
		if (nullptr == pHTMLElement || nullptr == pvHtmlElements)
		{
			nRet = -1;
			break;
		}

		CComPtr<IDispatch> pDispChildren;
		hr = pHTMLElement->get_children(&pDispChildren);
		if (FAILED(hr) || pDispChildren == nullptr)
		{
			nRet = -2;
			break;
		}

		CComPtr<IHTMLElementCollection> spChildren;
		hr = pDispChildren->QueryInterface(IID_IHTMLElementCollection, (void**)&spChildren);
		if (FAILED(hr) || spChildren == nullptr)
		{
			nRet = -3;
			break;
		}

		nRet = ParseIHTMLElementCollection(spChildren, lpszTagName, pvHtmlElements);
	} while (false);

	return nRet;
}

//for (auto& imagedataElement : vHtmlElements)
//{
//	BSTR bstrInnerHtml = nullptr;
//	hr = imagedataElement->get_outerHTML(&bstrInnerHtml);
//
//	CStringW s = bstrInnerHtml;
//	if (s.Find(L"hao123") > 0)
//	{
//		s.Replace(L"hao123", L"hao321");
//		BSTR strImg = SysAllocString(s);
//		hr = imagedataElement->put_outerHTML(bstrInnerHtml);
//	}
//
//	IHTMLStyle* pStyle = nullptr;
//	hr = imagedataElement->get_style(&pStyle);
//
//	// 设置成功但不生效
//	CComVariant strColor = L"green";
//	hr = pStyle->put_color(strColor);
//	CComVariant vC;
//	pStyle->get_color(&vC);
//}

// 使用mshtml库解析html，并把其中的a标签删除。
// html文件内容如下，文件由utf-8编码：
//<!DOCTYPE html>
//<html lang="en">
//<head>
//	<meta charset="UTF-8">
//	<meta name="viewport" content="width=device-width, initial-scale=1.0">
//	<title>Document</title>
//</head>
//<body>
//	<!-- 树控件 -->
//	<a>ttt</a>
//	<ul>
//		<li>111
//			<ol>
//				<li>111-1</li>
//				<li>111-2</li>
//				<li>111-3</li>
//			</ol>
//		</li>
//		<li>222
//			<ol>
//				<li>222-1</li>
//				<li>222-2</li>
//				<li>222-3</li>
//			</ol>
//		</li>
//		<li>333
//			<ol>
//				<li>333-1</li>
//				<li>333-2</li>
//				<li>333-3</li>
//			</ol>
//		</li>
//	</ul>
//</body>
//</html>
int DeleteLocalHtmlNode(LPCWSTR lpszHtmlFile)
{
	int nRet = 0;
	SAFEARRAY* psaStrings = nullptr;
	BSTR bstrHtml = nullptr;
	CComPtr<IHTMLDocument2> spHTMLDoc2;

	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
	{
		return -1;
	}

	do
	{
		if (1)
		{
			std::vector<BYTE> vFile;
			ReadFromFile(lpszHtmlFile, &vFile);

			// 原始字符串为UTF8 without BOM。转为ANSI后为乱码。
			std::string strUtf8;
			strUtf8.assign((const char*)&vFile[0], vFile.size());
			std::wstring strUnicodeHtml = IUI::UTF8ToUnicode(strUtf8.c_str());

			//
			// Create IHTMLDocument2 object.
			//
			hr = CoCreateInstance(CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER, IID_IHTMLDocument2, (void**)&spHTMLDoc2);
			if (FAILED(hr))
			{
				nRet = -2;
				break;
			}

			//
			// Load html string
			//

			// 将 HTML 字符串转换为 BSTR，并封装到 SAFEARRAY 中
			bstrHtml = IUI::WCHARToBSTR(strUnicodeHtml.c_str());
			psaStrings = SafeArrayCreateVector(VT_VARIANT, 0, 1);
			VARIANT* pvar;
			SafeArrayAccessData(psaStrings, (void**)&pvar);
			pvar->vt = VT_BSTR;
			pvar->bstrVal = bstrHtml;
			SafeArrayUnaccessData(psaStrings);

			// Load html string
			// 注：如果直接(SAFEARRAY*)lpszUtf8Html传给write，返回值是正确的
			// 但下面通过IHTMLDocument3.getElementsByTagName是得不到元素的。
			hr = spHTMLDoc2->write(psaStrings);
			if (FAILED(hr))
			{
				nRet = -3;
				break;
			}

			hr = spHTMLDoc2->close();
			if (FAILED(hr))
			{
				nRet = -4;
				break;
			}
		}

		//
		// 成功拿到所有的子元素和嵌套元素
		// 集合始终包括html标记、head标记和body标记的一个元素对象，而不管这些标记是否存在于文档中。
		// 由于是解析所有元素，故包含VML标签<v:imagedata>（如果有的话）。
		//
		CComPtr<IHTMLElementCollection> pChildren;
		hr = spHTMLDoc2->get_all((IHTMLElementCollection**)&pChildren);
		if (FAILED(hr))
		{
			nRet = -4;
			break;
		}

		//
		// 拿到指定类型的元素
		//
		std::vector<IHTMLElement*> vHtmlElements;
		ParseIHTMLElementCollection(pChildren, L"a", &vHtmlElements);
		//_ASSERT(vHtmlElements.size() == 1);


		// 删除元素
		if (!vHtmlElements.empty())
		{
			IHTMLElement* pDeleteEle = vHtmlElements[0];

			// 获取元素的父节点
			CComPtr<IHTMLDOMNode> pDeleteNode;
			HRESULT hr = pDeleteEle->QueryInterface(IID_PPV_ARGS(&pDeleteNode));
			if (SUCCEEDED(hr))
			{
				// removeNode是删除本节点
				// removeChild是删除直接孩子节点。
				// 假设a标签为<a>ttt</a>
				VARIANT_BOOL vb = TRUE; // TRUE: 删除整个a标签及它的孩子；FALSE:只会删除a标签，保留里面的ttt内容。
				CComPtr< IHTMLDOMNode> spRemove;
				hr = pDeleteNode->removeNode(vb, &spRemove);
				if (SUCCEEDED(hr))
				{
				}
				int n = 0;
			}
		}

		vHtmlElements.clear();
		ParseIHTMLElementCollection(pChildren, L"HTML", &vHtmlElements);

		// 转字符串，成功
		{
			// 获取根元素的 outerHTML，即整个文档的 HTML 字符串  
			// 通过IHTMLDocuments.write加载的Html，转成字符时，会改变HTML dom结构。
			BSTR bstrHtml2 = nullptr;
			hr = vHtmlElements[0]->get_outerHTML(&bstrHtml2);
			if (FAILED(hr))
			{
				// 错误处理  
			}

			int n = 0;
		}
	} while (false);

	SysFreeString(bstrHtml);

	CoUninitialize();

	return nRet;
}

int DeleteLocalHtmlNodeByWebBrowser(LPCWSTR lpszHtmlFile)
{
	int nRet = 0;
	SAFEARRAY* psaStrings = nullptr;
	BSTR bstrHtml = nullptr;
	CComPtr<IHTMLDocument2> spHTMLDoc2;
	CComPtr<IWebBrowser2> spBrowser;

	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
	{
		return -1;
	}

	do
	{
		if (1)
		{
			// 创建IWebBrowser2实例
			HRESULT hr = CoCreateInstance(CLSID_InternetExplorer, NULL, CLSCTX_LOCAL_SERVER, IID_IWebBrowser2, (void**)&spBrowser);
			if (FAILED(hr))
			{
				nRet = -5;
				break;
			}

			// 加载HTML内容（例如一个URL）
			CComBSTR Url = SysAllocString(lpszHtmlFile);

			// 如果不想显示IE窗口，就注释掉这行
			spBrowser->put_Visible(VARIANT_TRUE);

			hr = spBrowser->Navigate(Url, NULL, NULL, NULL, NULL);
			if (FAILED(hr))
			{
				nRet = -6;
				break;
			}

			// 获取 IHTMLDocument2 接口，可能需要管理员权限
			CComPtr<IDispatch> spDocDispatch;
			hr = spBrowser->get_Document(&spDocDispatch);
			if (FAILED(hr))
			{
				nRet = -7;
				break;
			}

			hr = spDocDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&spHTMLDoc2);
			if (FAILED(hr))
			{
				nRet = -7;
				break;
			}
		}

		//
		// 成功拿到所有的子元素和嵌套元素
		// 集合始终包括html标记、head标记和body标记的一个元素对象，而不管这些标记是否存在于文档中。
		// 由于是解析所有元素，故包含VML标签<v:imagedata>（如果有的话）。
		//
		CComPtr<IHTMLElementCollection> pChildren;
		hr = spHTMLDoc2->get_all((IHTMLElementCollection**)&pChildren);
		if (FAILED(hr))
		{
			nRet = -4;
			break;
		}

		//
		// 拿到指定类型的元素
		//
		std::vector<IHTMLElement*> vHtmlElements;
		ParseIHTMLElementCollection(pChildren, L"a", &vHtmlElements);
		//_ASSERT(vHtmlElements.size() == 1);


		// 删除元素
		if (!vHtmlElements.empty())
		{
			IHTMLElement* pDeleteEle = vHtmlElements[0];


			// 获取元素的父节点
			CComPtr<IHTMLDOMNode> pDeleteNode;
			HRESULT hr = pDeleteEle->QueryInterface(IID_PPV_ARGS(&pDeleteNode));
			if (SUCCEEDED(hr))
			{
				// removeNode是删除本节点
				// 这里调用父节点的removeChild删除自己，会失败。
				// 假设a标签为<a>ttt</a>
				VARIANT_BOOL vb = TRUE; // TRUE: 删除整个a标签及它的孩子；FALSE:只会删除a标签，保留里面的ttt内容。
				CComPtr< IHTMLDOMNode> spRemove;
				hr = pDeleteNode->removeNode(vb, &spRemove);
				if (SUCCEEDED(hr))
				{
				}
				int n = 0;
			}
		}

		vHtmlElements.clear();
		ParseIHTMLElementCollection(pChildren, L"HTML", &vHtmlElements);
		//_ASSERT(vHtmlElements.empty());

		// 转字符串，成功
		{
			// 获取根元素的 outerHTML，即整个文档的 HTML 字符串
			// 通过IWebBrowser2.Navigate加载的Html，转成字符时，不改变HTML dom结构。
			BSTR bstrHtml2 = nullptr;
			hr = vHtmlElements[0]->get_outerHTML(&bstrHtml2);
			if (FAILED(hr))
			{
				// 错误处理  
			}
			int n = 0;
		}

		if (0)
		{
			// 获取 IPersistFile 接口
			// 注：如果IHTMLDocument2是通过write接口写入的Html流，则通过
			// IPersistFile接口的Save是无法保存修改后的Html dom。目前测试下来，通过
			// 把修改后的Html Dom再转为String，是可以的。
			CComPtr<IPersistFile> spPersistFile;
			hr = spHTMLDoc2->QueryInterface(IID_IPersistFile, (void**)&spPersistFile);
			if (FAILED(hr))
			{
				nRet = -5;
				break;
			}

			// 保存文档到文件中，对于本地文档，在IE中元素已修改，但保存成本地文件后，还是旧的状态。
			// 查看目标文件，字符串被截断了。
			hr = spPersistFile->Save(L"D:\\local2.html", TRUE);
			if (FAILED(hr))
			{
				nRet = -6;
				break;
			}
		}

	} while (false);

	SysFreeString(bstrHtml);

	CoUninitialize();

	return nRet;
}

int HtmlSaveAsMhtml(LPCWSTR lpszHtmlFile)
{
	int nRet = 0;
	SAFEARRAY* psaStrings = nullptr;
	BSTR bstrHtml = nullptr;
	CComPtr<IHTMLDocument2> spHTMLDoc2;
	CComPtr<IWebBrowser2> spBrowser;

	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
	{
		return -1;
	}

	do
	{
		if (1)
		{
			// 创建IWebBrowser2实例
			HRESULT hr = CoCreateInstance(CLSID_InternetExplorer, NULL, CLSCTX_LOCAL_SERVER, IID_IWebBrowser2, (void**)&spBrowser);
			if (FAILED(hr))
			{
				nRet = -5;
				break;
			}

			// 加载HTML内容（例如一个URL）
			CComBSTR Url = SysAllocString(lpszHtmlFile);

			// 如果不想显示IE窗口，就注释掉这行
			spBrowser->put_Visible(VARIANT_TRUE);

			hr = spBrowser->Navigate(Url, NULL, NULL, NULL, NULL);
			if (FAILED(hr))
			{
				nRet = -6;
				break;
			}

			// 获取 IHTMLDocument2 接口，可能需要管理员权限
			CComPtr<IDispatch> spDocDispatch;
			hr = spBrowser->get_Document(&spDocDispatch);
			if (FAILED(hr))
			{
				nRet = -7;
				break;
			}

			hr = spDocDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&spHTMLDoc2);
			if (FAILED(hr))
			{
				nRet = -7;
				break;
			}
		}

		//
		// 成功拿到所有的子元素和嵌套元素
		// 集合始终包括html标记、head标记和body标记的一个元素对象，而不管这些标记是否存在于文档中。
		// 由于是解析所有元素，故包含VML标签<v:imagedata>（如果有的话）。
		//
		CComPtr<IHTMLElementCollection> pChildren;
		hr = spHTMLDoc2->get_all((IHTMLElementCollection**)&pChildren);
		if (FAILED(hr))
		{
			nRet = -4;
			break;
		}

		//
		// 拿到指定类型的元素
		//
		std::vector<IHTMLElement*> vHtmlElements;
		ParseIHTMLElementCollection(pChildren, L"a", &vHtmlElements);
		//_ASSERT(vHtmlElements.size() == 1);

		// 删除元素
		if (!vHtmlElements.empty())
		{
			IHTMLElement* pDeleteEle = vHtmlElements[0];


			// 获取元素的父节点
			CComPtr<IHTMLDOMNode> pDeleteNode;
			HRESULT hr = pDeleteEle->QueryInterface(IID_PPV_ARGS(&pDeleteNode));
			if (SUCCEEDED(hr))
			{
				// removeNode是删除本节点
				// 这里调用父节点的removeChild删除自己，会失败。
				// 假设a标签为<a>ttt</a>
				VARIANT_BOOL vb = TRUE; // TRUE: 删除整个a标签及它的孩子；FALSE:只会删除a标签，保留里面的ttt内容。
				CComPtr< IHTMLDOMNode> spRemove;
				hr = pDeleteNode->removeNode(vb, &spRemove);
				if (SUCCEEDED(hr))
				{
				}
				int n = 0;
			}
		}

		vHtmlElements.clear();
		ParseIHTMLElementCollection(pChildren, L"HTML", &vHtmlElements);
		//_ASSERT(vHtmlElements.empty());

		if (1)
		{
			// 调用"另存为"功能
			VARIANT vEmpty;
			VariantInit(&vEmpty);
			hr = spBrowser->ExecWB(OLECMDID_SAVEAS, OLECMDEXECOPT_DODEFAULT, &vEmpty, &vEmpty);
			if (FAILED(hr))
			{
				std::cerr << "另存为操作失败" << std::endl;
			}
			else
			{
				std::cout << "另存为操作成功" << std::endl;
			}

			// 清理
			VariantClear(&vEmpty);
		}

	} while (false);

	SysFreeString(bstrHtml);

	CoUninitialize();

	return nRet;
}


int Word2(LPCWSTR lpszHtmlFile)
{
	int nRet = 0;

	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
	{
		return -1;
	}

	do
	{
		// 创建 Word 应用程序对象指针
		IDispatch* pWordApp = NULL;
		CLSID clsWordApp;
		// 获取 Word 应用程序的 CLSID
		hr = CLSIDFromProgID(L"Word.Application", &clsWordApp);
		if (FAILED(hr))
		{
			std::cerr << "Failed CLSIDFromProgID" << std::endl;
			CoUninitialize();
			return 1;
		}

		// 创建 Word 应用程序实例
		hr = CoCreateInstance(clsWordApp, NULL, CLSCTX_LOCAL_SERVER, IID_IDispatch, (void**)&pWordApp);
		if (FAILED(hr))
		{
			std::cerr << "Failed to create Word application instance" << std::endl;
			CoUninitialize();
			return 1;
		}
		{
			// 使用_Application::Documents->Open打开文档
			IDispatch* pDocs = NULL;
			DISPID dispidDocs;
			CComBSTR methodName = L"Documents";
			hr = pWordApp->GetIDsOfNames(IID_NULL, &methodName, 1, LOCALE_USER_DEFAULT, &dispidDocs);
			if (FAILED(hr))
			{
				return -1;
			}

			// 调用Documents属性  
			DISPPARAMS dispparamsNoArgs = { NULL, NULL, 0, 0 };
			VARIANT varResult;
			VariantInit(&varResult);

			hr = pWordApp->Invoke(
				dispidDocs,               // 属性或方法的DISPID  
				IID_NULL,             // 通常是IID_NULL  
				LOCALE_USER_DEFAULT,  // 地域设置  
				DISPATCH_PROPERTYGET, // 属性获取  
				&dispparamsNoArgs,    // 没有参数  
				&varResult,           // 结果存储在这里  
				NULL,                 // 异常信息（可以为NULL）  
				NULL                  // 异步状态的标志（通常为NULL）  
			);

			pDocs = varResult.pdispVal;

			{
				// 构建DISPPARAMS结构体  
								// Open的参数
				VARIANT vtFileName;
				VariantInit(&vtFileName);
				vtFileName.vt = VT_BSTR;
				vtFileName.bstrVal = SysAllocString(L"d:\\1.docx"); // 替换为你的HTML文件路径

				VARIANT vtFalse;
				VariantInit(&vtFalse);
				vtFalse.vt = VT_BOOL;
				vtFalse.boolVal = VARIANT_FALSE;

				DISPPARAMS dispparamsOpen;
				dispparamsOpen.cArgs = 3;
				dispparamsOpen.cNamedArgs = 3;
				dispparamsOpen.rgvarg = new VARIANT[3];
				dispparamsOpen.rgvarg[0] = vtFileName;
				dispparamsOpen.rgvarg[1] = vtFalse; // ConfirmConversions
				dispparamsOpen.rgvarg[2] = vtFalse; // ReadOnly

				// 调用Open方法  
				DISPID dispidOpen;
				CComBSTR methodNameOpen = L"Open";
				hr = pDocs->GetIDsOfNames(IID_NULL, &methodNameOpen, 1, LOCALE_USER_DEFAULT, &dispidOpen);
				if (FAILED(hr))
				{
					return -1;
				}
				VARIANT varResult;
				VariantInit(&varResult);
				HRESULT hr = pDocs->Invoke(dispidOpen, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparamsOpen, &varResult, NULL, NULL);
			}
		}

		// 获取文档集合对象
		IDispatch* pDocuments = NULL;
		{
			DISPPARAMS params = { NULL, NULL, 0, 0 };
			VARIANT result;
			VariantInit(&result);
			DISPID dispid;
			CComBSTR methodName = L"Documents";
			// 获取 Documents 属性的 DISPID
			hr = pWordApp->GetIDsOfNames(IID_NULL, &methodName, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
			if (SUCCEEDED(hr))
			{
				// 调用 Documents 属性获取文档集合对象
				hr = pWordApp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &params, &result, NULL, NULL);
				if (SUCCEEDED(hr)) {
					pDocuments = V_DISPATCH(&result);
				}
			}
		}

		// 获取文档数量
		long docCount = 0;
		if (pDocuments != NULL) {
			{
				DISPID dispid;
				CComBSTR szMember = L"Count";
				hr = pDocuments->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_USER_DEFAULT, &dispid);
				if (SUCCEEDED(hr)) {
					VARIANT result;
					VariantInit(&result);
					DISPPARAMS dispParams = { NULL, NULL, 0, 0 };
					hr = pDocuments->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &dispParams, &result, NULL, NULL);
					if (SUCCEEDED(hr)) {
						docCount = V_I4(&result);
					}
				}
			}
		}

		// 打印文档数量
		std::cout << "Number of open Word documents: " << docCount << std::endl;


	} while (false);

	CoUninitialize();

	return nRet;
}

int ModifyOnlineHtml(LPCWSTR lpszUrl)
{
	// 初始化COM库
	CoInitialize(NULL);
	int nRet = 0;
	HRESULT hr = S_OK;

	do
	{
		// 创建IWebBrowser2实例
		CComPtr<IWebBrowser2> spBrowser;
		HRESULT hr = CoCreateInstance(CLSID_InternetExplorer, NULL, CLSCTX_LOCAL_SERVER, IID_IWebBrowser2, (void**)&spBrowser);
		if (SUCCEEDED(hr))
		{
			// 加载HTML内容（例如一个URL）
			CComVariant varUrl = lpszUrl;
			spBrowser->put_Visible(VARIANT_TRUE);

			hr = spBrowser->Navigate2(&varUrl, NULL, NULL, NULL, NULL);
			if (FAILED(hr))
			{
				// 处理错误
			}
		}
		else
		{
			// 处理创建失败的情况
		}

		// 等待页面加载完成
		READYSTATE readyState;
		do
		{
			spBrowser->get_ReadyState(&readyState);
			Sleep(100);
		} while (readyState != READYSTATE_COMPLETE);

		// 获取 IHTMLDocument2 接口
		CComPtr<IDispatch> spDocDispatch;
		hr = spBrowser->get_Document(&spDocDispatch);
		if (FAILED(hr))
		{
			nRet = -4;
			break;
		}

		CComPtr<IHTMLDocument2> spHTMLDoc2;
		hr = spDocDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&spHTMLDoc2);
		if (FAILED(hr))
		{
			nRet = -4;
			break;
		}

		//
		// 成功拿到所有的子元素和嵌套元素
		// 集合始终包括html标记、head标记和body标记的一个元素对象，而不管这些标记是否存在于文档中。
		// 由于是解析所有元素，故包含VML标签<v:imagedata>（如果有的话）。
		//
		CComPtr<IHTMLElementCollection> pChildren;
		hr = spHTMLDoc2->get_all((IHTMLElementCollection**)&pChildren);
		if (FAILED(hr))
		{
			nRet = -4;
			break;
		}

		//
		// 拿到指定类型的元素
		//
		std::vector<IHTMLElement*> vHtmlElements;
		ParseIHTMLElementCollection(pChildren, L"a", &vHtmlElements);
		//_ASSERT(vHtmlElements.size() == 1);

		for (auto& imagedataElement : vHtmlElements)
		{
			BSTR bstrInnerHtml = nullptr;
			hr = imagedataElement->get_outerHTML(&bstrInnerHtml);

			CStringW s = bstrInnerHtml;
			if (s.Find(L"hao123") > 0)
			{
				s.Replace(L"hao123", L"hao321");
				BSTR strImg = SysAllocString(s);
				hr = imagedataElement->put_outerHTML(bstrInnerHtml);
			}

			IHTMLStyle* pStyle = nullptr;
			hr = imagedataElement->get_style(&pStyle);

			// 实时生效，如果你通过浏览器查看本html，浏览器中的元素在调用put_color后，实时生效。
			// 如果在浏览器中打开F12,可以看到DOM结构中的元素实时增加相应的属性。
			CComVariant strColor = L"red";
			hr = pStyle->put_color(strColor);
			CComVariant vC;
			pStyle->get_color(&vC);
		}

		// 获取 IPersistStreamInit 接口
		CComPtr<IPersistFile> spPersistFile;
		hr = spHTMLDoc2->QueryInterface(IID_IPersistFile, (void**)&spPersistFile);
		if (FAILED(hr))
		{
			nRet = -5;
			break;
		}

		// 保存文档到文件中，下载baidu.com，需要管理员权限。
		hr = spPersistFile->Save(L"D:\\online.html", TRUE);
		if (FAILED(hr))
		{
			nRet = -6;
			break;
		}
	} while (false);

	// 在程序结束时，释放COM库
	CoUninitialize(); // 在适当的位置调用，例如在窗口销毁时

	return 0;
}
