/*
* ClipWatch clipboard extender/history/utility
* Copyright (C) 2001-2004, 2009, 2013-2014 Sean Echevarria
*
* This file is part of ClipWatch.
*
* ClipWatch is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ClipWatch is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* Let me know if you modify, extend or use ClipWatch.
* Project site: https://sourceforge.net/projects/clipwatch/
* Contact Sean: http://www.creepingfog.com/sean/
*/

// ClipboardHistory.cpp: implementation of the ClipboardHistory class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ClipboardHistory.h"
#include "AutoHandle.h"


#define RECORD_DELIMITER L"\01"
static LPCWSTR kRecordDelimiter = RECORD_DELIMITER;
static LPCWSTR kFieldDelimiter = L"\02";

#define DAT_FILE_VERSION L"01"
static const CString sHeader(L"ClipWatchDat-Version:" DAT_FILE_VERSION);

static CString
GetDatFilePath()
{
	CString datPath;
	datPath = GetAppDataDir();
	datPath += L"\\cliphist.wdat";
	return datPath;
}

static void
ReadFileToBuf(LPCWSTR fileName, std::vector<WCHAR> &data)
{
	DWORD err;
	AutoHandle hFile;

	for (int retry = 5; retry; --retry)
	{
		hFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, nullptr,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (!hFile)
		{
			err = GetLastError();
			if (retry && err == 0x20)
			{
				Sleep(500);
				continue;
			}

			return;
		}

		break;
	}

	DWORD fsLow, fsHi;
	fsLow = GetFileSize(hFile, &fsHi);
	if (fsLow == 0xFFFFFFFF && (err = GetLastError()) != NO_ERROR)
	{ 
		_ASSERT(FALSE);
		return;
    }
	
	if (fsHi) 
	{
		_ASSERT(FALSE);
		// return or read as much as we can??
		return;
    }
	
	if (!fsLow)
		return;

	const int dataLen = (fsLow + 1) * sizeof(WCHAR);
	data = std::vector<WCHAR>(dataLen, L'\0');
	if (!::ReadFile(hFile, &data[0], fsLow, &fsHi, nullptr))
	{
		err = GetLastError();
		_ASSERT(FALSE);
	}
}

ClipboardHistory::ClipboardHistory(int maxItems) : mMaxItems(maxItems)
{
	Open();
}

ClipboardHistory::~ClipboardHistory()
{
	Save();
}

void
ClipboardHistory::Save()
{
	AutoHandle hFile = ::CreateFile(::GetDatFilePath(), GENERIC_WRITE, 0, nullptr,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (!hFile || INVALID_HANDLE_VALUE == hFile)
		return;

	DWORD bytesWritten;
	::WriteFile(hFile, sHeader, wcslen(sHeader) * sizeof(WCHAR), &bytesWritten, nullptr);
	::WriteFile(hFile, kRecordDelimiter, sizeof(WCHAR), &bytesWritten, nullptr);

	int idx = mHistData.size();
	if (idx > 0)
		idx--;

	CString pinStr;
	for (; idx >= 0; idx--)
	{
		bool pinned;
		LPCWSTR strItem = GetItem(idx, pinned);
		if (!strItem || !strItem[0])
			break;

		::WriteFile(hFile, strItem, wcslen(strItem) * sizeof(WCHAR), &bytesWritten, nullptr);
		::WriteFile(hFile, kFieldDelimiter, wcslen(kFieldDelimiter) * sizeof(WCHAR), &bytesWritten, nullptr);

		pinStr.Format(L"%d", pinned);
		::WriteFile(hFile, pinStr, pinStr.GetLength() * sizeof(WCHAR), &bytesWritten, nullptr);
		::WriteFile(hFile, kFieldDelimiter, wcslen(kFieldDelimiter) * sizeof(WCHAR), &bytesWritten, nullptr);

		::WriteFile(hFile, kRecordDelimiter, wcslen(kRecordDelimiter) * sizeof(WCHAR), &bytesWritten, nullptr);
	}
}

void
ClipboardHistory::Open()
{
	// not Clear since pinned items need to be removed also
	mHistData.clear();
	std::vector<WCHAR> fileBuf;
	::ReadFileToBuf(::GetDatFilePath(), fileBuf);
	if (!fileBuf.size())
		return;

	LPWSTR pBegin = &fileBuf[0];
	try
	{
		if (!pBegin || !*pBegin)
			throw std::runtime_error("empty file");

		LPWSTR pEnd = wcsstr(pBegin, kRecordDelimiter);
		if (!pEnd)
			throw std::runtime_error("malformed file");
	
		*pEnd++ = L'\0';
		if (!wcslen(pBegin))
			throw std::runtime_error("malformed file");

		CString fileVer(pBegin);
		if (fileVer != sHeader)
			throw std::runtime_error("incompatible version");

		pBegin = pEnd;
		while (pBegin && *pBegin)
		{
			LPWSTR pNextRecord = wcsstr(pBegin, kRecordDelimiter);
			if (pNextRecord)
				*pNextRecord++ = L'\0';

			LPWSTR pFieldEnd = wcsstr(pBegin, kFieldDelimiter);
			if (!pFieldEnd)
				break;

			*pFieldEnd++ = L'\0';
			const CString curTxt(pBegin);
			pBegin = pFieldEnd;

			pFieldEnd = wcsstr(pBegin, kFieldDelimiter);
			if (pFieldEnd)
				*pFieldEnd++ = L'\0';

			const CString curItemPinned(pBegin);
			if (!curTxt.IsEmpty() && !curItemPinned.IsEmpty())
				Add(curTxt, curItemPinned == L"1");

			pBegin = pNextRecord;
		}
	}
	catch (const std::runtime_error&)
	{
	}
}

BOOL
ClipboardHistory::Add(LPCWSTR strToAdd, bool pinned)
{
	if (!(strToAdd && *strToAdd))
		return FALSE;

	CString str(strToAdd);
	return Add(str, pinned);
}

BOOL
ClipboardHistory::Add(const CString & str, bool pinned)
{
	if (str.IsEmpty())
		return FALSE;

	// see if str already exists in list
	ClipHist::iterator iter;
	for (iter = mHistData.begin(); iter != mHistData.end(); iter++)
	{
		if ((*iter).mText == str)
		{
			if ((*iter).mPinned)
				pinned = true;

			// check to see if str is already at head
			if (iter == mHistData.begin())
				return FALSE;

			mHistData.erase(iter);
			break;
		}
	}

	// add str to list
	mHistData.push_front(ClippedItem(str, pinned));

	// limit size of history but don't count pinned items
	if ((int)mHistData.size() >= mMaxItems)
	{
		int newSize = 0;
		int pinnedCnt = 0;
		int maxItemCnt = mMaxItems;
		for (iter = mHistData.begin(); iter != mHistData.end(); )
		{
			if ((*iter).mPinned)
			{
				// pinned items don't count against max
				++maxItemCnt;
				++newSize;
				++pinnedCnt;
			}
			else if (newSize < maxItemCnt)
			{
				++newSize;
			}
			else
			{
				// remove cur item
				mHistData.erase(iter++);
				continue;
			}

			++iter;
		}

		_ASSERTE(newSize <= (mMaxItems + pinnedCnt));
	}

#ifdef _DEBUG
	::OutputDebugString(L"\n\n\tStartHistList\n");
	for (iter = mHistData.begin(); iter != mHistData.end(); iter++)
	{
		::OutputDebugString((*iter).mText);
		CString msg;
		msg.Format(L"\n  %d\n", (*iter).mPinned);
		::OutputDebugString(msg);
	}
#endif

	return TRUE;
}

LPCWSTR
ClipboardHistory::GetItem(int idx, bool &pinned)
{
	int cnt = 0;
	for (ClipHist::iterator iter = mHistData.begin(); iter != mHistData.end(); ++cnt, ++iter)
	{
		if (cnt == idx)
		{
			pinned = (*iter).mPinned;
			return (*iter).mText;
		}
	}

	return nullptr;
}

void
ClipboardHistory::MoveToHead(int idx, const CString & str)
{
	bool pinned = false;
	int i = 0;
	for (ClipHist::iterator iter = mHistData.begin(); iter != mHistData.end(); iter++, ++i)
	{
		if (i == idx)
		{
			pinned = (*iter).mPinned;
			mHistData.erase(iter);
			break;
		}
	}

	Add(str, pinned);
}

void
ClipboardHistory::Clear()
{
	for (ClipHist::iterator iter = mHistData.begin(); iter != mHistData.end(); )
	{
		if ((*iter).mPinned)
			++iter;
		else
			mHistData.erase(iter++);
	}
}

void
ClipboardHistory::Remove(int idx)
{
	int i = 0;
	for (ClipHist::iterator iter = mHistData.begin(); iter != mHistData.end(); iter++, ++i)
	{
		if (i == idx)
		{
			mHistData.erase(iter);
			return;
		}
	}
}

void
ClipboardHistory::TogglePin(int idx)
{
	int i = 0;
	for (ClipHist::iterator iter = mHistData.begin(); iter != mHistData.end(); iter++, ++i)
	{
		if (i == idx)
		{
			(*iter).mPinned = !(*iter).mPinned;
			return;
		}
	}
}
