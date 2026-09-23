#include "FileReadWrite.h"
#include <stdio.h>

int WriteToFile(LPCWSTR lpszFile, LPCSTR lpszBuf, size_t cchLen)
{
	FILE* fp = nullptr;
	errno_t err = _wfopen_s(&fp, lpszFile, L"wb");
	if (nullptr == fp)
	{
		return err;
	}

	fwrite(lpszBuf, cchLen, 1, fp);

	if (nullptr != fp)
	{
		fclose(fp);
		fp = nullptr;
	}

	return 0;
}

int WriteToFile(LPCWSTR lpszFile, LPCWSTR lpszBuf, size_t cchLen)
{
	FILE* fp = nullptr;
	errno_t err = _wfopen_s(&fp, lpszFile, L"wb");
	if (nullptr == fp)
	{
		return err;
	}

	fwrite(lpszBuf, sizeof(WCHAR) * cchLen, 1, fp);

	if (nullptr != fp)
	{
		fclose(fp);
		fp = nullptr;
	}

	return 0;
}

int ReadFromFile(LPCWSTR lpszFile, std::vector<BYTE>* pvBuf)
{
	if (nullptr == pvBuf)
	{
		return E_INVALIDARG;
	}

	FILE* fp = nullptr;
	errno_t err = _wfopen_s(&fp, lpszFile, L"rb");
	if (nullptr == fp)
	{
		return err;
	}

	fseek(fp, 0, SEEK_END);
	INT64 n = _ftelli64(fp);

	pvBuf->resize(n);

	fseek(fp, 0, SEEK_SET);
	fread(&(*pvBuf)[0], n, 1, fp);
	fclose(fp);

	return 0;
}