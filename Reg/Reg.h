#pragma once
#include <string>
#include <vector>
#include <windows.h>

// 概念：注册表编辑器左侧树中都是键，如：HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft，其中
// HKEY_LOCAL_MACHINE叫根键，SOFTWARE\Microsoft叫子键。
// 注册表编辑器中右侧窗口中，名称一栏对应的是"值"，如"Path"，值有类型，如字符串类型、DWORD类型等，
// 而"值"的值叫"数据"，也就是“数据”一栏对应的内容，如：“C:\\Windows\\”

// 注册表重定向：
//应用程序和（COM）组件程序将它们的配置数据保存在注册表中。组件程序在安装过程中，当它们被注册的时候，通常将配置数据写到注册表中。（如果微软不做特殊处理）如果同样的组件既安装和注册了一个32位二进制文件，又安装和注册了一个64位二进制文件，那么，最后被注册的那个组件将会覆盖掉以前组件的注册，因为它们写到注册表中同样的位置上。
//为了以透明的方式解决这个问题，并且无须对32位组件进行任何代码修改，注册表被分成两个部分：原生的和Wow64的。在默认情况下，32位组件访问32位视图，64位组件访问64位视图。这为32位和64位组件提供了一个安全的执行环境，并且将32位应用程序的状态与64位应用程序（如果存在的话）的状态隔高开来。
//
//为了实现这一点，Wow64截获了所有要打开注册表键的系统调用，并且重新解释这些册表键的路径，将它们指向注册表的Wow64视图。Wow64在以下这些点上分裂注册表：
//НKLM\SOFTWARE
//НKEY_CLASSES_ROOT
//
//然而，请注意，许多子键实际上在32位和64位应用之间是共享的，也就是说，并非整个注册表键被分裂了。
//
//在以上每一个键的下面，Wow64创建了一个称为Wow6432Node的键。在该键下面保存的是32位配置信息。注册表的所有其他部分对于32位应用程序和64位应用程序都是共享的（比如 HKLM\SYSTEM )。
//
//还有一个额外的帮助，如果一个32位应用程序向注册表中写入一个以数据“%ProgramFiles%”或“%commonprogramfiles%”为开头的 REG_SZ 或者 REG_EXPAND_SZ值，那么Wow64将实际的值修改为"%ProgramFiles(x86)%"或"%commonprogramfiles(x86)%"以便符合前面介细的文件系统重定向和布局结构。
//
//32位应用程序必须正确地写这些子行串（包括大小写）-- - 任何其他的数据都被忽略，按普通的方式写入。最后，任何包含“system32“的键被替换为“syswow64”（针对所有的大小写），也不管标志和大小写是否敏感，除非便用了 KEY_WOW64_64KEY，以及该键位于“反射键”列表中（可在 MSDN 上查询到）。
//
//如果应用程序需要显式地指定一个注册表键位于某个特定的视图中，那么，在 RegOpenKeyEx, RegCreateKeyEx, RegOpenKeyTransacted、 RegCreateKeyTransacted和RegDeleteKeyEx函数中使用下述标志可以做到这一点：
//KEY_WOW64_64KEY : 从一个32位或者64位应用程序中显式地打开一个64位键，并且禁止前面介绍的 REG_SZ 或 REG_EXPAND_SZ截获转换处理。
//KEY_WOW64_32KEY : 从一个32位或者64位咸用程序中显式地打开一个32位键。

// 32位程序运行在32位系统上，无64位注册表项
// 32位程序运行在64位系统上，访问32位和64位注册表项
// 64位程序无法运行在32位系统上
// 64位程序运行在64位系统上，访问32位和64位注册表项
// 会反射的注册表项：
//HKEY_LOCAL_MACHINE\Software\Classes
//HKEY_LOCAL_MACHINE\Software\COM3
//HKEY_LOCAL_MACHINE\Software\Ole
//HKEY_LOCAL_MACHINE\Software\EventSystem
//HKEY_LOCAL_MACHINE\Software\RPC
// 当我们要访问WOW6432Node下的键值时，不论是32位程序还是64位程序运行在64位系统上，
// 只要子键路径中包含WOW6432Node，RegOpenKeyEx的时候，加不加KEY_WOW64_64KEY、RegGetValue的时候，
// 加不加RRF_SUBKEY_WOW6464KEY都可以访问，这是因为我们已经明确了访问WOW6432Node。
// https://docs.microsoft.com/en-us/windows/win32/sysinfo/32-bit-and-64-bit-application-data-in-the-registry
// https://docs.microsoft.com/en-us/windows/win32/winprog64/shared-registry-keys
// 禁用/启用注册表反射使用RegDisableReflectionKey和RegEnableReflectionKey函数
//
// 例如：	32位程序调用IUI::SetRegString(HKEY_LOCAL_MACHINE, R"(SOFTWARE\iUIShop)", "lswtest", "abc", FALSE);
// 执行成功后，会在HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\iUIShop下设置“值”lswtext的数据为abc。
// 如果最后一个参数为TRUE，则表示设置64位视图的注册表项，执行成功后，会在HKEY_LOCAL_MACHINE\SOFTWARE\iUIShop下设置“值”lswtext的数据为abc。
// 用户调用的时候，sub key路径中，尽量不要出现WOW6432Node，而应该由REG_ACCESS_FLAG参数来自动控制。

// 权限问题：普通用户无法修改HKEY_LOCAL_MACHINE下面的键值。
namespace IUI
{
	enum REG_ACCESS_FLAG
	{
		// 默认：32位程序访问32位注册表视图，64位程序访问64位注册表视图。典型示例为32位程序在64位系统上
		// 访问HKEY_LOCAL_MACHINE\SOFTWARE\iUIShop，实际上访问的是HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\iUIShop
		RAF_DEFAULT = 0,

		// 无论32位程序还是64位程序，强制访问32位注册表视图（WOW6432Node节点下的）。
		RAF_32KEY,

		// 无论32位程序还是64位程序，强制访问64位注册表视图（非WOW6432Node节点下的）。
		RAF_64KEY
	};

	enum REG_VALUE_STRING_TYPE
	{
		RVST_SZ = 0,
		RVST_EXPAND_SZ,
		RVST_MULTI_SZ
	};

	int FormatRetrunValue(LSTATUS lRet);

	// 一次可以创建出多级注册表子键
	int CreateRegKeyW(HKEY hRootKey, LPCWSTR lpszSubKey, REG_ACCESS_FLAG eRaf = RAF_DEFAULT);

	// 如果指定的子键不存在，则自动创建
	int SetRegDwordW(HKEY hRootKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue, DWORD dwData, REG_ACCESS_FLAG eRaf = RAF_DEFAULT);
	int GetRegDwordW(HKEY hRootKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue, __out DWORD *pdwData, REG_ACCESS_FLAG eRaf = RAF_DEFAULT);

	// 如果指定的子键不存在，则自动创建
	int SetRegDword(HKEY hRootKey, LPCTSTR lpszSubKey, LPCTSTR lpszValue, DWORD dwData, REG_ACCESS_FLAG eRaf = RAF_DEFAULT);
	int GetRegDword(HKEY hRootKey, LPCTSTR lpszSubKey, LPCTSTR lpszValue, __out DWORD *pdwData, REG_ACCESS_FLAG eRaf = RAF_DEFAULT);

	// 如果指定的子键不存在，则自动创建
	int SetRegBinaryW(
		HKEY hRootKey,
		LPCWSTR lpszSubKey,
		LPCWSTR lpszValue,
		const BYTE *pData,
		DWORD cbData,
		REG_ACCESS_FLAG eRaf = RAF_DEFAULT);
	int GetRegBinaryW(
		HKEY hRootKey,
		LPCWSTR lpszSubKey,
		LPCWSTR lpszValue,
		__out std::vector<BYTE> *pvData,
		REG_ACCESS_FLAG eRaf = RAF_DEFAULT);

	// 如果指定的子键不存在，则自动创建
	int SetRegStringA(
		HKEY hRootKey,
		const char *pszSubKey,
		const char *pszValue,
		const char *pszData,
		REG_VALUE_STRING_TYPE eStringType = RVST_SZ,
		REG_ACCESS_FLAG eRaf = RAF_DEFAULT);
	int GetRegStringA(
		HKEY hRootKey,
		const char *pszSubKey,
		const char *pszValue,
		std::string *pstrRet,
		REG_VALUE_STRING_TYPE eStringType = RVST_SZ,
		REG_ACCESS_FLAG eRaf = RAF_DEFAULT);
	// 如果指定的子键不存在，则自动创建
	int SetRegStringW(
		HKEY hRootKey,
		const WCHAR *pszSubKey,
		const WCHAR *pszValue,
		const WCHAR *pszData,
		REG_VALUE_STRING_TYPE eStringType = RVST_SZ,
		REG_ACCESS_FLAG eRaf = RAF_DEFAULT);
	int GetRegStringW(
		HKEY hRootKey,
		const WCHAR *pszSubKey,
		const WCHAR *pszValue,
		std::wstring *pstrRet,
		REG_VALUE_STRING_TYPE eStringType = RVST_SZ,
		REG_ACCESS_FLAG eRaf = RAF_DEFAULT);

	// 删除“Value”
	int DeleteRegValueW(HKEY hRootKey,
		LPCWSTR pszSubKey,
		LPCWSTR pszValue,
		REG_ACCESS_FLAG eRaf = RAF_DEFAULT);

	// 删除“Key”
	int DeleteRegKeyW(HKEY hRootKey, LPCWSTR pszSubKey, REG_ACCESS_FLAG eRaf = RAF_DEFAULT);

	// 查询指定的“Key”是否存在
	int QueryRegKeyW(HKEY hRootKey, LPCWSTR pszSubKey, BOOL *pbExist, REG_ACCESS_FLAG eRaf = RAF_DEFAULT);

	// 枚举一个Key下所有的Key
	typedef int (*EnumKeyCallbackW)(HKEY hRootKey, LPCWSTR lpszSubKey, LPCWSTR lpszKey, void *pCallbackData);
	int EnumKeyW(HKEY hRootKey, LPCWSTR lpszSubKey, EnumKeyCallbackW fnEnumKeyCallback, void* pCallbackData, REG_ACCESS_FLAG eRaf = RAF_DEFAULT);

	// 枚举一个Key下所有的Value
	typedef int (*EnumValueCallbackW)(HKEY hRootKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue, DWORD eValueType, const BYTE *pbtData, DWORD dwDataSize, void* pCallbackData);
	int EnumValueW(HKEY hRootKey, LPCWSTR lpszSubKey, EnumValueCallbackW fnEnumValueCallback, void* pCallbackData, REG_ACCESS_FLAG eRaf = RAF_DEFAULT);

	int GoToRegSubKey(LPCWSTR lpszSubKey);
}
