#pragma once

#include <windows.h>


typedef BOOL(*IsEnableExceptionCatchPtr)();

int VectorHandle(LPCWSTR lpszDumpPath, LPCWSTR lpszDumpName, int nLimitDumpCount, IsEnableExceptionCatchPtr fn);
int UnVectorHandle();
