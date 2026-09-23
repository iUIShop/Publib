#pragma once

#include <list>
#include <map>
#include <string>
#include <comutil.h>
#include <atlbase.h>
#include <WbemCli.h>
#pragma comment (lib, "Wbemuuid.lib")

int WMIQueryDataFromClass(const wchar_t *pszClass,
	std::list<std::map<std::wstring, _variant_t>> *plistmapClassInfo,
	const wchar_t *pszNamespace = L"ROOT\\CIMV2");

int WMIExecuteMethodGetData(const wchar_t *pszClass, const wchar_t *pszMethod,
	__out std::map<std::wstring, CComVariant>* pmapResult,
	__in const std::map<std::wstring, CComVariant>* pmapArgs = nullptr,
	const wchar_t *pszNamespace = L"ROOT\\CIMV2");

int WMIExecuteMethodGetDataLong(const wchar_t* pszClass, const wchar_t* pszMethod,
	const wchar_t *pszPropName,
	__out long *plRet,
	__in const std::map<std::wstring, CComVariant>* pmapArgs = nullptr,
	const wchar_t* pszNamespace = L"ROOT\\CIMV2");

int WMIExecuteMethodGetDataString(const wchar_t* pszClass, const wchar_t* pszMethod,
	const wchar_t* pszPropName,
	__out std::wstring *pstrRet,
	__in const std::map<std::wstring, CComVariant>* pmapArgs = nullptr,
	const wchar_t* pszNamespace = L"ROOT\\CIMV2");

// protected
int WMIExecuteMethod(const wchar_t* pszClass, const wchar_t* pszMethod,
	__in const std::map<std::wstring, CComVariant>* pmapArgs,
	__out IWbemClassObject** ppOutParam,
	const wchar_t* pszNamespace = L"ROOT\\CIMV2");
