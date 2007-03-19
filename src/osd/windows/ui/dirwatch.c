/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#include <windows.h>
#include <string.h>
#include <tchar.h>
#include "dirwatch.h"

typedef BOOL (WINAPI *READDIRECTORYCHANGESFUNC)(HANDLE hDirectory, LPVOID lpBuffer,
		DWORD nBufferLength, BOOL bWatchSubtree, DWORD dwNotifyFilter,
		LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped,
		LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);


struct DirWatcherEntry
{
	struct DirWatcherEntry *pNext;
	HANDLE hDir;
	WORD nIndex;
	WORD nSubIndex;
	BOOL bWatchSubtree;
	OVERLAPPED overlapped;

	union
	{
		FILE_NOTIFY_INFORMATION notify;
		BYTE buffer[1024];
	} u;

	TCHAR szDirPath[1];
};



struct DirWatcher
{
	HMODULE hKernelModule;
	READDIRECTORYCHANGESFUNC pfnReadDirectoryChanges;
	
	HWND hwndTarget;
	UINT nMessage;

	HANDLE hRequestEvent;
	HANDLE hResponseEvent;
	HANDLE hThread;
	CRITICAL_SECTION crit;
	struct DirWatcherEntry *pEntries;

	// These are posted externally
	BOOL bQuit;
	BOOL bWatchSubtree;
	WORD nIndex;
	LPCTSTR pszPathList;
};



static void DirWatcher_SetupWatch(PDIRWATCHER pWatcher, struct DirWatcherEntry *pEntry)
{
	DWORD nDummy;

	memset(&pEntry->u, 0, sizeof(pEntry->u));

	pWatcher->pfnReadDirectoryChanges(pEntry->hDir,
		&pEntry->u,
		sizeof(pEntry->u),
		pEntry->bWatchSubtree,
		FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE,
		&nDummy,
		&pEntry->overlapped,
		NULL);
}



static void DirWatcher_FreeEntry(struct DirWatcherEntry *pEntry)
{
	if (pEntry->hDir)
		CloseHandle(pEntry->hDir);
	free(pEntry);
}



static BOOL DirWatcher_WatchDirectory(PDIRWATCHER pWatcher, int nIndex, int nSubIndex,
	LPCTSTR pszPath, BOOL bWatchSubtree)
{
	struct DirWatcherEntry *pEntry;
	HANDLE hDir;

	pEntry = malloc(sizeof(*pEntry) + (_tcslen(pszPath) * sizeof(pszPath)));
	if (!pEntry)
		goto error;
	memset(pEntry, 0, sizeof(*pEntry));
	_tcscpy(pEntry->szDirPath, pszPath);
	pEntry->overlapped.hEvent = pWatcher->hRequestEvent;

	hDir = CreateFile(pszPath, FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
	if (!hDir || (hDir == INVALID_HANDLE_VALUE))
		goto error;

	// Populate the entry
	pEntry->hDir = hDir;
	pEntry->bWatchSubtree = bWatchSubtree;
	pEntry->nIndex = nIndex;
	pEntry->nSubIndex = nSubIndex;

	// Link in the entry
	pEntry->pNext = pWatcher->pEntries;
	pWatcher->pEntries = pEntry;

	DirWatcher_SetupWatch(pWatcher, pEntry);
	return TRUE;

error:
	if (pEntry)
		DirWatcher_FreeEntry(pEntry);
	return FALSE;
}



static void DirWatcher_Signal(PDIRWATCHER pWatcher, struct DirWatcherEntry *pEntry)
{
	LPTSTR pszFileName;
	LPTSTR pszFullFileName;
	BOOL bPause;
	HANDLE hFile;
	int nTries;

#ifdef UNICODE
	pszFileName = pEntry->u.notify.FileName;
#else
	{
		int nLength;
		nLength = WideCharToMultiByte(CP_ACP, 0, pEntry->u.notify.FileName, -1, NULL, 0, NULL, NULL);
		pszFileName = (LPSTR) alloca(nLength * sizeof(*pszFileName));
		WideCharToMultiByte(CP_ACP, 0, pEntry->u.notify.FileName, -1, pszFileName, nLength, NULL, NULL);
	}
#endif

	// get the full path to this new file
	pszFullFileName = (LPTSTR) alloca((_tcslen(pEntry->szDirPath) + _tcslen(pszFileName) + 2) * sizeof(*pszFullFileName));
	_tcscpy(pszFullFileName, pEntry->szDirPath);
	_tcscat(pszFullFileName, TEXT("\\"));
	_tcscat(pszFullFileName, pszFileName);

	// attempt to busy wait until any result other than ERROR_SHARING_VIOLATION
	// is generated
	nTries = 100;
	do
	{
		hFile = CreateFile(pszFullFileName, GENERIC_READ,
			FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
			CloseHandle(hFile);

		bPause = (nTries--) && (hFile == INVALID_HANDLE_VALUE)
			&& (GetLastError() == ERROR_SHARING_VIOLATION);
		if (bPause)
			Sleep(10);
	}
	while(bPause);

	// send the message (assuming that we have a target)
	if (pWatcher->hwndTarget)
	{
		SendMessage(pWatcher->hwndTarget,
			pWatcher->nMessage,
			(pEntry->nIndex << 16) | (pEntry->nSubIndex << 0),
			(LPARAM) pszFileName);
	}

	DirWatcher_SetupWatch(pWatcher, pEntry);
}



static DWORD WINAPI DirWatcher_ThreadProc(LPVOID lpParameter)
{
	LPTSTR pszPathList;
	LPTSTR s;
	int nSubIndex;
	PDIRWATCHER pWatcher = (PDIRWATCHER) lpParameter;
	struct DirWatcherEntry *pEntry;
	struct DirWatcherEntry **ppEntry;

	do
	{
		WaitForSingleObject(pWatcher->hRequestEvent, INFINITE);

		if (pWatcher->pszPathList)
		{
			// remove any entries with the same nIndex
			ppEntry = &pWatcher->pEntries;
			while(*ppEntry)
			{
				if ((*ppEntry)->nIndex == pWatcher->nIndex)
				{
					pEntry = *ppEntry;
					*ppEntry = pEntry->pNext;
					DirWatcher_FreeEntry(pEntry);
				}
				else
				{
					ppEntry = &(*ppEntry)->pNext;
				}
			}

			// allocate our own copy of the path list
			pszPathList = (LPTSTR) alloca((_tcslen(pWatcher->pszPathList) + 1) * sizeof(*pWatcher->pszPathList));
			_tcscpy(pszPathList, pWatcher->pszPathList);
			
			nSubIndex = 0;
			do
			{
				s = _tcschr(pszPathList, ';');
				if (s)
					*s = '\0';

				if (*pszPathList)
				{
					DirWatcher_WatchDirectory(pWatcher, pWatcher->nIndex,
						nSubIndex++, pszPathList, pWatcher->bWatchSubtree);
				}

				pszPathList = s ? s + 1 : NULL;
			}
			while(pszPathList);

			pWatcher->pszPathList = NULL;
			pWatcher->bWatchSubtree = FALSE;
		}
		else
		{
			// we have to go through the list and find what has been hit
			for (pEntry = pWatcher->pEntries; pEntry; pEntry = pEntry->pNext)
			{
				if (pEntry->u.notify.Action != 0)
				{
					DirWatcher_Signal(pWatcher, pEntry);
				}
			}
		}

		SetEvent(pWatcher->hResponseEvent);
	}
	while(!pWatcher->bQuit);
	return 0;
}



PDIRWATCHER DirWatcher_Init(HWND hwndTarget, UINT nMessage)
{
	struct DirWatcher *pWatcher = NULL;
	DWORD nThreadID;

	// This feature does not exist on Win9x
	if (GetVersion() >= 0x80000000)
		goto error;

	pWatcher = malloc(sizeof(struct DirWatcher));
	if (!pWatcher)
		goto error;
	memset(pWatcher, 0, sizeof(*pWatcher));
	InitializeCriticalSection(&pWatcher->crit);

	pWatcher->hKernelModule = LoadLibrary(TEXT("kernel32.dll"));
	if (!pWatcher->hKernelModule)
		goto error;

	pWatcher->pfnReadDirectoryChanges = (READDIRECTORYCHANGESFUNC)
		GetProcAddress(pWatcher->hKernelModule, "ReadDirectoryChangesW");
	if (!pWatcher->pfnReadDirectoryChanges)
		goto error;

	pWatcher->hRequestEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!pWatcher->hRequestEvent)
		goto error;

	pWatcher->hResponseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!pWatcher->hRequestEvent)
		goto error;

	pWatcher->hThread = CreateThread(NULL, 0, DirWatcher_ThreadProc,
		(LPVOID) pWatcher, 0, &nThreadID);

	pWatcher->hwndTarget = hwndTarget;
	pWatcher->nMessage = nMessage;
	return pWatcher;

error:
	if (pWatcher)
		DirWatcher_Free(pWatcher);
	return NULL;
}



BOOL DirWatcher_Watch(PDIRWATCHER pWatcher, WORD nIndex, LPCTSTR pszPathList, BOOL bWatchSubtrees)
{
	EnterCriticalSection(&pWatcher->crit);

	pWatcher->nIndex = nIndex;
	pWatcher->pszPathList = pszPathList;
	pWatcher->bWatchSubtree = bWatchSubtrees;
	SetEvent(pWatcher->hRequestEvent);

	WaitForSingleObject(pWatcher->hResponseEvent, INFINITE);
	LeaveCriticalSection(&pWatcher->crit);
	return TRUE;
}



void DirWatcher_Free(PDIRWATCHER pWatcher)
{
	struct DirWatcherEntry *pEntry;
	struct DirWatcherEntry *pNextEntry;

	if (pWatcher->hThread)
	{
		EnterCriticalSection(&pWatcher->crit);
		pWatcher->bQuit = TRUE;
		SetEvent(pWatcher->hRequestEvent);
		WaitForSingleObject(pWatcher->hThread, 1000);
		LeaveCriticalSection(&pWatcher->crit);
		CloseHandle(pWatcher->hThread);
	}

	DeleteCriticalSection(&pWatcher->crit);

	pEntry = pWatcher->pEntries;
	while(pEntry)
	{
		pNextEntry = pEntry->pNext;
		DirWatcher_FreeEntry(pEntry);
		pEntry = pNextEntry;
	}

	if (pWatcher->hKernelModule)
		FreeLibrary(pWatcher->hKernelModule);
	if (pWatcher->hRequestEvent)
		CloseHandle(pWatcher->hRequestEvent);
	if (pWatcher->hResponseEvent)
		CloseHandle(pWatcher->hResponseEvent);
	free(pWatcher);
}



