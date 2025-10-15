#pragma once

#include <windows.h>
#include <string>

// ����Hash256
namespace IUI
{
	enum HASH256_MSG
	{
		// wParam: �ĸ����������Ĵ���, ���ͣ�const WCHAR *; lParam: MAKELPARAM(Hash256�������, error code)
		HM_ERROR = 1,
		// lParam: �ܳ���
		HM_BEGIN,
		// wParam: ��ǰ�Ѵ�����; lParam: �ܳ��ȡ���λsize_t
		HM_PROGRESS,
		// lParam: ���������Hash256�ַ���ָ�룬���ͣ�const WCHAR *.
		HM_END
	};

	typedef int (*Hash256Proc)(HASH256_MSG uMsg, WPARAM wParam, LPARAM lParam, void* pUserData);

	// ͬ�����������ͨ��OnHash256Proc���أ�������������ͨ��OnHash256Proc���أ�Ҳ����ͨ��pstrHashResult���ء�
	// pUserData: �û��Զ����OnHash256Proc������ͨ��OnHash256Proc���һ���������ݡ�
	int GetDataHash256(const BYTE* pbtData, size_t nDataLen, BOOL bUpperCase, std::wstring *pstrHashResult, Hash256Proc OnHash256Proc, void* pUserData);

	// ͬ�����������ͨ��OnHash256Proc���أ�������������ͨ��OnHash256Proc���أ�Ҳ����ͨ��pstrHashResult���ء�
	// ��GetFileHash256��Ϊstd::thread�̺߳�����ʱ��Ϊ�˷��㴫�Σ����ļ�·��������Ƴ�const wstring&���ͣ�������LPCWSTR���ͣ���ΪLPCWSTR��ǳ������
	int GetFileHash256(const std::wstring &strFilePath, BOOL bUpperCase, std::wstring *pstrHashResult, Hash256Proc OnHash256Proc, void* pUserData);

	// �첽��������Ⱥͽ��ͨ��OnHash256Proc����
	int GetFileHash256Async(const std::wstring& strFilePath, BOOL bUpperCase, Hash256Proc OnHash256Proc, void* pUserData);
}