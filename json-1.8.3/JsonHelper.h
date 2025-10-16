// jsoncpp助手，支持linux/win
#pragma once
#include <fstream>
#include <string>
#include "json/json.h"

#ifdef WIN32
#include <shlwapi.h>
#include <atlconv.h>
#include <atlstr.h>
#pragma comment(lib, "shlwapi.lib")
#endif

class CJsonHelper
{
public:
	static bool LoadJsonByFile(const char *lpFile, Json::Value &jvRoot)
	{
		m_strError.Empty();

		bool bLoadSucc = false;

#ifdef WIN32
		if (!PathFileExistsA(lpFile))
#else
		if (access(lpFile, F_OK) != 0)
#endif
		{
			return bLoadSucc;
		}

		try
		{
			Json::Reader _reader;

			jvRoot = Json::Value(Json::nullValue);

			std::ifstream _config;

			_config.open(lpFile, std::ios::in);
			bLoadSucc = _reader.parse(_config, jvRoot);
		}
		catch (...)
		{
			return false;
		}

		return bLoadSucc;
	}

	static bool LoadJsonByStream(const std::string &strDoc, Json::Value &jvRoot)
	{
		m_strError.Empty();

		bool bLoadSucc = false;

		if (strDoc.empty())
		{
			return bLoadSucc;
		}

		try
		{
			Json::Reader _reader;

			jvRoot = Json::Value(Json::nullValue);

			bLoadSucc = _reader.parse(strDoc, jvRoot);
			if (!bLoadSucc)
			{
				m_strError = _reader.getFormatedErrorMessages().c_str();
			}
		}
		catch (...)
		{
			return false;
		}

		return bLoadSucc;
	}

	// 把json对象格式化后写入文件
	static bool WriteJsonToFile(const char *lpFile, const Json::Value &jvRoot)
	{
		if (lpFile == NULL)
		{
			return false;
		}

		try
		{
			Json::StyledWriter writer;
			std::string strBuff = writer.write(jvRoot);

			std::ofstream _config;
			_config.open(lpFile, std::ios::out);
			_config.write(strBuff.c_str(), strBuff.size());
			_config.flush();
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	// 把json对象转成格式化好的字符串
	static bool StyledWriter(const Json::Value &jvRoot, std::string &strJsonA)
	{
		try
		{
			Json::StyledWriter writer;

			strJsonA = writer.write(jvRoot);
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	// 把json对象转成单行字符串
	static bool FastWriteJson(const Json::Value &jvRoot, std::string &strJsonA)
	{
		try
		{
			Json::FastWriter writer;

			strJsonA = writer.write(jvRoot);
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	static int GetJsonValueINT(const char *lpKey, const Json::Value &jvRoot, int dwDefault = 0)
	{
		try
		{
			if (jvRoot.isNull() || !jvRoot.isObject())
			{
				return dwDefault;
			}

			if (lpKey == NULL || (!jvRoot.isMember(std::string(lpKey))))
			{
				return dwDefault;
			}

			Json::Value _value = jvRoot[std::string(lpKey)];
			if (!_value.isInt() && !_value.isUInt())
			{
				return dwDefault;
			}

			return _value.asInt();
		}
		catch (...)
		{
			return dwDefault;
		}
	}

	static Json::UInt64 GetJsonValueINT64(const char *lpKey, const Json::Value &jvRoot, Json::UInt64 dwDefault = 0)
	{
		try
		{
			if (jvRoot.isNull() || !jvRoot.isObject())
			{
				return dwDefault;
			}

			if (lpKey == NULL || (!jvRoot.isMember(std::string(lpKey))))
			{
				return dwDefault;
			}

			Json::Value _value = jvRoot[std::string(lpKey)];
			if (!_value.isInt() && !_value.isUInt())
			{
				return dwDefault;
			}

			return _value.asInt64();
		}
		catch (...)
		{
			return dwDefault;
		}
	}

	static double GetJsonValueDouble(const char *lpKey, const Json::Value &jvRoot, double dDefault = 0)
	{
		try
		{
			if (jvRoot.isNull() || !jvRoot.isObject())
			{
				return dDefault;
			}

			if (lpKey == NULL || (!jvRoot.isMember(lpKey)))
			{
				return dDefault;
			}

			Json::Value _value = jvRoot[lpKey];
			if (!_value.isNumeric())
			{
				return dDefault;
			}

			return _value.asDouble();
		}
		catch (...)
		{
			return dDefault;
		}
	}

	static int GetJsonValueString(const char *lpKey, const Json::Value &jvRoot, std::string *pstrRet, std::string strDefault = std::string())
	{
		int nRet = 0;
		do
		{
			if (NULL == pstrRet)
			{
				nRet = -1;
				break;
			}

			*pstrRet = strDefault;

			if (jvRoot.isNull() || !jvRoot.isObject())
			{
				nRet = -2;
				break;
			}

			if (lpKey == NULL || (!jvRoot.isMember(std::string(lpKey))))
			{
				nRet = -3;
				break;
			}

			Json::Value _value = jvRoot[std::string(lpKey)];
			if (_value.isNull() || !_value.isString())
			{
				nRet = -4;
				break;
			}

			*pstrRet = _value.asCString();
		} while (false);

		return nRet;
	}

	static bool GetJsonValueArray(const char *lpKey, const Json::Value &jvRoot, Json::Value &jvArray)
	{
		try
		{
			if (jvRoot.isNull() || !jvRoot.isObject())
			{
				return false;
			}

			if (lpKey == NULL || (!jvRoot.isMember(std::string(lpKey))))
			{
				return false;
			}

			jvArray = jvRoot[lpKey];
			if (jvArray.isNull() || !jvArray.isArray())
			{
				return false;
			}

			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	static bool WriteJsonValueInt(const char *lpKey, Json::Value &jvRoot, int nValue)
	{
		try
		{
			if (lpKey == NULL)
			{
				return false;
			}

			Json::Value val(nValue);
			jvRoot[std::string(lpKey)].swap(val);

			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	static bool WriteJsonValueString(const char *lpKey, Json::Value &jvRoot, const char *lpValue)
	{
		try
		{
			if (lpKey == NULL)
			{
				return false;
			}

			Json::Value val(lpValue);
			jvRoot[std::string(lpKey)].swap(val);

			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	static bool GetJsonObject(const Json::Value &jsParent, const char *lpKey, Json::Value &jsObject)
	{
		try
		{
			if (jsParent.isNull() || !jsParent.isObject() || lpKey == NULL)
			{
				return false;
			}

			jsObject = Json::nullValue;

			if (jsParent.isMember(std::string(lpKey)))
			{
				jsObject = jsParent[lpKey];
			}

			return !jsObject.isNull();
		}
		catch (...)
		{
			return false;
		}
	}

	static CStringA m_strError;
};
