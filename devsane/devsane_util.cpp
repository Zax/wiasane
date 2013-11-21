#include "devsane_util.h"

#ifdef _DEBUG
#include <stdio.h>
#endif

#include <tchar.h>
#include <strsafe.h>
#include <malloc.h>

#include "resource.h"
#include "strutil.h"


VOID Trace(_In_ LPCTSTR pszFormat, ...)
{
#ifdef _DEBUG
	PTSTR lpMsg, lpOut;
	va_list argList;
	HANDLE hHeap;
	size_t cbLen;
	HRESULT hr;

	hHeap = GetProcessHeap();
	if (hHeap) {
		va_start(argList, pszFormat);
		hr = StringCchAVPrintf(hHeap, &lpMsg, &cbLen, pszFormat, argList);
		va_end(argList);
		if (SUCCEEDED(hr)) {
			hr = StringCchAPrintf(hHeap, &lpOut, &cbLen, TEXT("devsane: %s\r\n"), lpMsg);
			if (SUCCEEDED(hr)) {
				OutputDebugString(lpOut);
				HeapFree(hHeap, 0, lpOut);
			}
			HeapFree(hHeap, 0, lpMsg);
		}
	}
#else
	UNREFERENCED_PARAMETER(pszFormat);
#endif
}


DWORD CreateInstallInfo(_In_ HANDLE hHeap, _Inout_ PINSTALLERINFO pInstallerInfo, _Inout_ LPVOID *plpData)
{
	TCHAR szModuleFileName[MAX_PATH + 1];
	LPTSTR lpszSubBlock, lpszValue;
	LPWORD lpwLanguage;
	DWORD dwLen, dwHandle;
	size_t cbSubBlock;
	HMODULE hModule;
	UINT cbSize;
	HRESULT hr;
	BOOL res;

	if (!pInstallerInfo || !plpData)
		return ERROR_INVALID_PARAMETER;

	hModule = GetModuleInstance();
	if (!hModule)
		return ERROR_OUTOFMEMORY;

	dwLen = GetModuleFileName(hModule, szModuleFileName, MAX_PATH);
	if (!dwLen)
		return GetLastError();

	dwLen = GetFileVersionInfoSize(szModuleFileName, &dwHandle);
	if (!dwLen)
		return GetLastError();

	*plpData = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwLen);
	if (!*plpData)
		return ERROR_OUTOFMEMORY;

	ZeroMemory(pInstallerInfo, sizeof(INSTALLERINFO));
	LoadString(hModule, IDS_APPLICATION_ID, (LPWSTR) &pInstallerInfo->pApplicationId, 0);

	res = GetFileVersionInfo(szModuleFileName, dwHandle, dwLen, *plpData);
	if (res) {
		res = VerQueryValue(*plpData, TEXT("\\VarFileInfo\\Translation"), (LPVOID*) &lpwLanguage, &cbSize);
		if (res) {
			Trace(TEXT("Translation: %04x%04x (%d)"), lpwLanguage[0], lpwLanguage[1], cbSize);

			hr = StringCbAPrintf(hHeap, &lpszSubBlock, &cbSubBlock, TEXT("\\StringFileInfo\\%04x%04x\\ProductName"), lpwLanguage[0], lpwLanguage[1]);
			if (SUCCEEDED(hr)) {
				Trace(TEXT("SubBlock: %s (%d)"), lpszSubBlock, cbSubBlock);

				res = VerQueryValue(*plpData, lpszSubBlock, (LPVOID*) &lpszValue, &cbSize);
				if (res) {
					pInstallerInfo->pDisplayName = lpszValue;
					pInstallerInfo->pProductName = lpszValue;
				} else
					Trace(TEXT("VerQueryValue 2 failed: %08X"), GetLastError());
			} else
				Trace(TEXT("StringCbAPrintf 1 failed: %08X"), hr);

			hr = StringCbAPrintf(hHeap, &lpszSubBlock, &cbSubBlock, TEXT("\\StringFileInfo\\%04x%04x\\CompanyName"), lpwLanguage[0], lpwLanguage[1]);
			if (SUCCEEDED(hr)) {
				Trace(TEXT("SubBlock: %s (%d)"), lpszSubBlock, cbSubBlock);

				res = VerQueryValue(*plpData, lpszSubBlock, (LPVOID*) &lpszValue, &cbSize);
				if (res) {
					pInstallerInfo->pMfgName = lpszValue;
				} else
					Trace(TEXT("VerQueryValue 3 failed: %08X"), GetLastError());
			} else
				Trace(TEXT("StringCbAPrintf 2 failed: %08X"), hr);
		} else
			Trace(TEXT("VerQueryValue 1 failed: %08X"), GetLastError());
	} else
		Trace(TEXT("GetFileVersionInfo failed: %08X"), GetLastError());

	Trace(TEXT("ApplicationID: %s"), pInstallerInfo->pApplicationId);
	Trace(TEXT("DisplayName:   %s"), pInstallerInfo->pDisplayName);
	Trace(TEXT("ProductName:   %s"), pInstallerInfo->pProductName);
	Trace(TEXT("MfgName:       %s"), pInstallerInfo->pMfgName);

	return ERROR_SUCCESS;
}
