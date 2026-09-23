#pragma once

#include <windows.h>

int DeleteLocalHtmlNode(LPCWSTR lpszHtmlFile);
int DeleteLocalHtmlNodeByWebBrowser(LPCWSTR lpszHtmlFile);

// 可以成功save，但save后的，并没有编码。
int HtmlSaveAsMhtml(LPCWSTR lpszHtmlFile);

int Word2(LPCWSTR lpszHtmlFile);

int ModifyOnlineHtml(LPCWSTR lpszUrl);
