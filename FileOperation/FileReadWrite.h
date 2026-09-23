#pragma once

#include <windows.h>
#include <vector>

int WriteToFile(LPCWSTR lpszFile, LPCSTR lpszBuf, size_t cchLen);
int WriteToFile(LPCWSTR lpszFile, LPCWSTR lpszBuf, size_t cchLen);
int ReadFromFile(LPCWSTR lpszFile, std::vector<BYTE>* pvBuf);
