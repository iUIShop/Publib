#include "WMIHelper.h"
#include <shlwapi.h>
#include <vector>

// TODO: memory leak
int WMIQueryDataFromClass(const wchar_t *pszClass,
	std::list<std::map<std::wstring, _variant_t>> *plistmapClassInfo, const wchar_t *pszNamespace/* = L"ROOT\\CIMV2"*/)
{
	if (NULL == pszClass || NULL == plistmapClassInfo)
	{
		return 1;
	}

	HRESULT hr = S_OK;
	int nRet = 0;
	IWbemServices *pSvc = NULL;
	IWbemLocator *pLoc = NULL;
	IEnumWbemClassObject *pEnumerator = NULL;
	IWbemClassObject *pclsObj = NULL;

	// Set general COM security levels --------------------------
	// Note: If you are using Windows 2000, you need to specify -
	// the default authentication credentials for a user by using
	// a SOLE_AUTHENTICATION_LIST structure in the pAuthList ----
	// parameter of CoInitializeSecurity ------------------------

	do
	{
		// Obtain the initial locator to WMI -------------------------
		hr = CoCreateInstance(
				CLSID_WbemLocator,
				0,
				CLSCTX_INPROC_SERVER,
				IID_IWbemLocator, (LPVOID *)&pLoc);
		if (FAILED(hr))
		{
			//g_log.Write("Failed to create IWbemLocator object. Err code = 0x%08x.", hr);
			nRet = -2;
			break;
		}

		// Connect to WMI through the IWbemLocator::ConnectServer method

		// Connect to the root\cimv2 namespace with
		// the current user and obtain pointer pSvc
		// to make IWbemServices calls.
		hr = pLoc->ConnectServer(
				_bstr_t(pszNamespace), // Object path of WMI namespace
				NULL,                    // User name. NULL = current user
				NULL,                    // User password. NULL = current
				0,                       // Locale. NULL indicates current
				NULL,                    // Security flags.
				0,                       // Authority (e.g. Kerberos)
				0,                       // Context object
				&pSvc                    // pointer to IWbemServices proxy
			);
		if (FAILED(hr))
		{
			//g_log.Write("Could not connect. Error code = 0x%08x.", hr);
			nRet = -3;
			break;
		}

		//g_log.Write("Connected to ROOT\\CIMV2 WMI namespace.");


		// Set security levels on the proxy -------------------------
		hr = CoSetProxyBlanket(
				pSvc,                        // Indicates the proxy to set
				RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
				RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
				NULL,                        // Server principal name
				RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
				RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
				NULL,                        // client identity
				EOAC_NONE                    // proxy capabilities
			);
		if (FAILED(hr))
		{
			//g_log.Write("Could not set proxy blanket. Error code = 0x%08x.", hr);
			nRet = -4;
			break;
		}

		// Use the IWbemServices pointer to make requests of WMI ----

		// get local group info
		std::wstring strWQL = L"SELECT * FROM ";
		strWQL += pszClass;
		hr = pSvc->ExecQuery(
				bstr_t("WQL"),
				bstr_t(strWQL.c_str()),
				WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
				NULL,
				&pEnumerator);
		if (FAILED(hr))
		{
			//g_log.Write("Query for operating system name failed. Error code = 0x08x.", hr);
			nRet = -5;
			break;
		}

		// Get the data from the query in previous step
		ULONG uReturn = 0;
		while (NULL != pEnumerator)
		{
			pclsObj = nullptr;
			HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
			if (FAILED(hr) || 0 == uReturn || nullptr == pclsObj)
			{
				break;
			}

			// 枚举出这个类信息的所有属性名
			SAFEARRAY *pNames = NULL;
			hr = pclsObj->GetNames(NULL, WBEM_FLAG_ALWAYS, NULL, &pNames);
			if (FAILED(hr))
			{
				if (NULL != pNames)
				{
					SafeArrayDestroy(pNames);
					pNames = NULL;
				}
				continue;
			}

			std::map<std::wstring, _variant_t> mapProp;

			int nNameCount = pNames->rgsabound->cElements;
			for (int nEle = 0; nEle < nNameCount; ++nEle)
			{
				WCHAR *pszName = ((wchar_t * *)pNames->pvData)[nEle];

				if (StrCmpW(pszName, L"CommandLine") == 0)
				{
					int n = 0;
				}

				VARIANT vtProp;

				// Get the value of the Name property
				hr = pclsObj->Get(pszName, 0, &vtProp, 0, 0);
				if (FAILED(hr))
				{
					continue;
				}

				mapProp[pszName] = vtProp;
				VariantClear(&vtProp);
			}
			plistmapClassInfo->push_back(mapProp);

			if (NULL != pNames)
			{
				SafeArrayDestroy(pNames);
				pNames = NULL;
			}

			if (NULL != pclsObj)
			{
				pclsObj->Release();
				pclsObj = NULL;
			}
		}
	} while (FALSE);

	// Cleanup
	// ========

	if (NULL != pSvc)
	{
		pSvc->Release();
		pSvc = NULL;
	}
	if (NULL != pLoc)
	{
		pLoc->Release();
		pLoc = NULL;
	}
	if (NULL != pEnumerator)
	{
		pEnumerator->Release();
		pEnumerator = NULL;
	}
	if (NULL != pclsObj)
	{
		pclsObj->Release();
		pclsObj = NULL;
	}

	return nRet;
}

int WMIExecuteMethodGetData(const wchar_t* pszClass, const wchar_t* pszMethod,
	__out std::map<std::wstring, CComVariant>* pmapResult,
	__in const std::map<std::wstring, CComVariant>* pmapArgs/* = nullptr*/,
	const wchar_t* pszNamespace/* = L"ROOT\\CIMV2"*/)
{
	CComPtr<IWbemClassObject> pOutParam = NULL;
	int nRet = 0;
	do
	{
		nRet = WMIExecuteMethod(pszClass, pszMethod, pmapArgs, &pOutParam, pszNamespace);
		if (0 != nRet)
		{
			break;
		}

		// 枚举结果列表
		SAFEARRAY* pNames = nullptr;
		HRESULT hr = pOutParam->GetNames(nullptr, WBEM_FLAG_ALWAYS | WBEM_FLAG_NONSYSTEM_ONLY, nullptr, &pNames);
		if (FAILED(hr) || nullptr == pNames)
		{
			nRet = -13;
			return nRet;
		}

		// 解析结果
		BYTE* pData = nullptr;
		hr = SafeArrayAccessData(pNames, reinterpret_cast<void**>(&pData));
		_ASSERT(pData == pNames->pvData);
		if (FAILED(hr))
		{
			nRet = -14;
			SafeArrayDestroy(pNames);
			break;
		}

		// 获取SAFEARRAY的维度数量，即pNames->cDims的值。
		UINT uDimensionCount = SafeArrayGetDim(pNames);
		_ASSERT(uDimensionCount == pNames->cDims);

		long lEleCount = 0;
		for (UINT nDim = 0; nDim < uDimensionCount; ++nDim)
		{
			lEleCount += pNames->rgsabound[nDim].cElements;
		}

		std::vector<BSTR> vProps;
		if (pNames->fFeatures & FADF_BSTR)
		{
			if (pNames->fFeatures & FADF_HAVEVARTYPE)
			{
				BSTR* pstrData = (BSTR*)pData;

				for (long i = 0; i < lEleCount; ++i)
				{
					BSTR pstrProp = pstrData[i];

					vProps.push_back(pstrProp);
				}
			}
		}

		// 完成对数组数据的访问，与SafeArrayAccessData对应，
		// 调用后，会使pNames->cLocks 减 1。
		SafeArrayUnaccessData(pNames);

		// 释放SAFEARRAY
		SafeArrayDestroy(pNames);

		for (auto& propName : vProps)
		{
			CComVariant vt_prop{};
			hr = pOutParam->Get(propName, 0, &vt_prop, NULL, NULL);
			if (FAILED(hr))
			{
				nRet = -6;
				break;
			}

			(*pmapResult)[propName] = vt_prop;

			// 解析结果
			//CComVariant v;
			//if (v.vt == VT_BSTR)
			//	LPCWSTR lpstr = static_cast<LPCWSTR>(v.bstrVal);
			//else if (v.vt == VT_BOOL)
			//	bool b = static_cast<bool>(vt_prop.boolVal == VARIANT_TRUE ? true : false);
			//else if (v.vt == VT_I4)
			//{
			//	long = vt_prop.lVal;
			//}
			//else if (v.vt == (VT_ARRAY | VT_I4))
			//{
			//	SAFEARRAY* psa = v.parray;
			//	std::vector<int> array_int;
			//	for (long i = 0; i < (long)psa->cbElements; i++)
			//	{
			//		int out = 0;
			//		::SafeArrayGetElement(psa, &i, &out);
			//		array_int.push_back(out);
			//	}
			//}
		}
	} while (false);

	return nRet;
}

int WMIExecuteMethodGetDataLong(const wchar_t* pszClass, const wchar_t* pszMethod, const wchar_t* pszPropName, long* plRet, const std::map<std::wstring, CComVariant>* pmapArgs, const wchar_t* pszNamespace)
{
	std::map<std::wstring, CComVariant> mapResult;
	int nRet = WMIExecuteMethodGetData(pszClass, pszMethod, &mapResult, pmapArgs, pszNamespace);

	auto it = mapResult.find(pszPropName);
	if (it != mapResult.end())
	{
		if (it->second.vt == VT_I4 || it->second.vt == VT_INT)
		{
			*plRet = it->second.lVal;
		}
		else
		{
			nRet = -101;
		}
	}
	else
	{
		nRet = -100;
	}

	return nRet;
}

int WMIExecuteMethodGetDataString(const wchar_t* pszClass, const wchar_t* pszMethod, const wchar_t* pszPropName, std::wstring* pstrRet, const std::map<std::wstring, CComVariant>* pmapArgs, const wchar_t* pszNamespace)
{
	std::map<std::wstring, CComVariant> mapResult;
	int nRet = WMIExecuteMethodGetData(pszClass, pszMethod, &mapResult, pmapArgs, pszNamespace);

	auto it = mapResult.find(pszPropName);
	if (it != mapResult.end())
	{
		if (it->second.vt == VT_BSTR)
		{
			*pstrRet = static_cast<LPCWSTR>(it->second.bstrVal);
		}
		else
		{
			nRet = -101;
		}
	}
	else
	{
		nRet = -100;
	}

	return nRet;
}

int WMIExecuteMethod(
	const wchar_t* pszClass,
	const wchar_t* pszMethod,
	const std::map<std::wstring, CComVariant>* pmapArgs,
	IWbemClassObject **ppOutParam,
	const wchar_t* pszNamespace/* = L"ROOT\\CIMV2"*/)
{
	// 按需初始化Com
	if (0)
	{
		// Step 1: 初始化COM库
		HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
		if (FAILED(hres))
		{
			return 1;
		}

		// Step 2: 设置安全模式
		hres = CoInitializeSecurity(
			NULL,
			-1,                          // Default authentication service
			NULL,                        // Default authentication level
			NULL,                        // Default principal name
			RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication level for proxies
			RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation level for proxies
			NULL,                        // Authentication information
			EOAC_NONE,                   // Additional capabilities of the client or server
			NULL                         // Reserved
		);
	}

	if (nullptr == pszClass || nullptr == pszMethod)
	{
		return -1;
	}

	HRESULT hr = S_OK;
	int nRet = 0;
	IWbemServices* pSvc = NULL;
	IWbemLocator* pLoc = NULL;
	IEnumWbemClassObject* pEnumerator = NULL;
	IWbemClassObject* pclsObj = NULL;

	// Set general COM security levels --------------------------
	// Note: If you are using Windows 2000, you need to specify -
	// the default authentication credentials for a user by using
	// a SOLE_AUTHENTICATION_LIST structure in the pAuthList ----
	// parameter of CoInitializeSecurity ------------------------

	do
	{
		// Step 1: 获取WMI服务
		hr = CoCreateInstance(
			CLSID_WbemLocator,
			0,
			CLSCTX_INPROC_SERVER,
			IID_IWbemLocator, (LPVOID*)&pLoc);
		if (FAILED(hr))
		{
			//g_log.Write("Failed to create IWbemLocator object. Err code = 0x%08x.", hr);
			nRet = -2;
			break;
		}

		// 连接到指定的namespace
		hr = pLoc->ConnectServer(
			_bstr_t(pszNamespace),   // Object path of WMI namespace
			NULL,                    // User name. NULL = current user
			NULL,                    // User password. NULL = current
			0,                       // Locale. NULL indicates current
			NULL,                    // Security flags.
			0,                       // Authority (e.g. Kerberos)
			0,                       // Context object
			&pSvc                    // pointer to IWbemServices proxy
		);
		if (FAILED(hr))
		{
			//g_log.Write("Could not connect. Error code = 0x%08x.", hr);
			nRet = -3;
			break;
		}

		// Step 2: 设置安全上下文
		hr = CoSetProxyBlanket(
			pSvc,                        // Indicates the proxy to set
			RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
			RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
			NULL,                        // Server principal name
			RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
			RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
			NULL,                        // client identity
			EOAC_NONE                    // proxy capabilities
		);
		if (FAILED(hr))
		{
			//g_log.Write("Could not set proxy blanket. Error code = 0x%08x.", hr);
			nRet = -4;
			break;
		}

		// Step 3: Get the class object path
		std::wstring strClassPath;
		{
			CComPtr<IEnumWbemClassObject> pWmiEnum = NULL;
			hr = pSvc->CreateInstanceEnum(_bstr_t(pszClass),
				WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
				nullptr,
				&pWmiEnum);
			if (FAILED(hr))
			{
				nRet = -5;
				break;
			}

			CComPtr<IWbemClassObject> pClassObj = NULL;
			DWORD u_returned;
			hr = pWmiEnum->Next(WBEM_INFINITE, 1, &pClassObj, &u_returned);
			if (FAILED(hr))
			{
				nRet = -6;
				break;
			}

			CComVariant varPath;
			hr = pClassObj->Get(L"__PATH", 0,
				&varPath,
				nullptr,
				nullptr);
			if (FAILED(hr))
			{
				nRet = -7;
				break;
			}

			try
			{
				pSvc->GetObject(_bstr_t(pszClass), 0, nullptr, &pclsObj, nullptr);
				if (nullptr == pclsObj)
				{
					nRet = -8;
					break;
				}
			}
			catch (...)
			{
				nRet = -9;
				break;
			}
			strClassPath = varPath.bstrVal;
		}

		// Step 4: 设置参数
		CComPtr<IWbemClassObject> pInClassObjectParam = nullptr;
		if (pmapArgs != nullptr && !pmapArgs->empty())
		{
			CComPtr<IWbemClassObject> pInSignature = nullptr;
			hr = pclsObj->GetMethod(
				pszMethod,
				0,
				&pInSignature,
				nullptr);
			if (FAILED(hr) || nullptr == pInSignature)
			{
				nRet = -10;
				break;
			}

			hr = pInSignature->SpawnInstance(0, &pInClassObjectParam);
			if (FAILED(hr) || nullptr == pInClassObjectParam)
			{
				nRet = -10;
				break;
			}

			// 定义参数
			for (auto& arg : *pmapArgs)
			{
				std::wstring strValue = arg.first;
				CComVariant comData = arg.second;

				// 将参数添加到方法调用中
				hr = pInClassObjectParam->Put(strValue.c_str(), 0, &comData, 0);
				if (FAILED(hr))
				{
					nRet = -11;
					break;
				}
			}
		}

		// Step 5: 执行方法
		hr = pSvc->ExecMethod(
			_bstr_t(strClassPath.c_str()), // \\LSW-I9\root\wmi:LENOVO_GAMEZONE_DATA.InstanceName="ACPI\\PNP0C14\\GMZN_0"
			_bstr_t(pszMethod),
			0,
			NULL,
			pInClassObjectParam,
			ppOutParam,
			NULL);
		if (FAILED(hr))
		{
			nRet = -12;
			break;
		}

	} while (FALSE);

	// Cleanup
	// ========

	if (NULL != pSvc)
	{
		pSvc->Release();
		pSvc = NULL;
	}
	if (NULL != pLoc)
	{
		pLoc->Release();
		pLoc = NULL;
	}
	if (NULL != pEnumerator)
	{
		pEnumerator->Release();
		pEnumerator = NULL;
	}
	if (NULL != pclsObj)
	{
		pclsObj->Release();
		pclsObj = NULL;
	}

	return nRet;
}
