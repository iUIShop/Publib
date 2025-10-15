#pragma once

#include <windows.h>
#include <string>

// ����CRC32
namespace IUI
{
	enum CRC32_MSG
	{
		// wParam: �ĸ����������Ĵ���, ���ͣ�const WCHAR *; lParam: MAKELPARAM(CRC32�������, error code)
		CM_ERROR = 1,
		// lParam: �ܳ���
		CM_BEGIN,
		// wParam: ��ǰ�Ѵ�����; lParam: �ܳ���
		CM_PROGRESS,
		// lParam: ���������CRC32�ַ���ָ�룬���ͣ�const WCHAR *.
		CM_END
	};

	typedef int (*CRC32Proc)(CRC32_MSG uMsg, WPARAM wParam, LPARAM lParam, void* pUserData);

	// ͬ�����������ͨ��OnCRC32Proc���أ�������������ͨ��OnCRC32Proc���أ�Ҳ����ͨ��pstrHashResult���ء�
	// pUserData: �û��Զ����OnCRC32Proc������ͨ��OnCRC32Proc���һ���������ݡ�
	int GetDataCRC32(const BYTE* pbtData, size_t nDataLen, BOOL bUpperCase, std::wstring *pstrHashResult, CRC32Proc OnCRC32Proc, void* pUserData);

	// ͬ�����������ͨ��OnCRC32Proc���أ�������������ͨ��OnCRC32Proc���أ�Ҳ����ͨ��pstrHashResult���ء�
	// ��GetFileCRC32��Ϊstd::thread�̺߳�����ʱ��Ϊ�˷��㴫�Σ����ļ�·��������Ƴ�const wstring&���ͣ�������LPCWSTR���ͣ���ΪLPCWSTR��ǳ������
	int GetFileCRC32(const std::wstring &strFilePath, BOOL bUpperCase, std::wstring *pstrHashResult, CRC32Proc OnCRC32Proc, void* pUserData);

	// �첽��������Ⱥͽ��ͨ��OnCRC32Proc����
	int GetFileCRC32Async(const std::wstring& strFilePath, BOOL bUpperCase, CRC32Proc OnCRC32Proc, void* pUserData);
}
