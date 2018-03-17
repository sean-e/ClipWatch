/*
* ClipWatch clipboard extender/history/utility
* Copyright (C) 2001-2004, 2013-2014, 2018 Sean Echevarria
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

// ClipboardHistory.h: interface for the ClipboardHistory class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CLIPBOARDHISTORY_H__8B1A03ED_5044_11D4_B580_00C04F8CF986__INCLUDED_)
#define AFX_CLIPBOARDHISTORY_H__8B1A03ED_5044_11D4_B580_00C04F8CF986__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlstr.h>
#include <list>


class ClipboardHistory  
{
public:
	ClipboardHistory(int maxItems);
	virtual ~ClipboardHistory();

	void Save();
	BOOL Add(LPCWSTR str, bool pinned);
	BOOL Add(const CString & str, bool pinned);
	void TogglePin(int idx);
	void Remove(int idx);
	void Clear();

	CString GetItem(int idx, bool &pinned);
	void MoveToHead(int idx, const CString & str);

protected:
	void Open();

private:
	struct ClippedItem
	{
		CString mText;
		bool	mPinned;

		ClippedItem(const CString& txt) :
			mText(txt),
			mPinned(false)
		{
		}

		ClippedItem(const CString& txt, bool keep) :
			mText(txt),
			mPinned(keep)
		{
		}
	};

	using ClipHist = std::list<ClippedItem>;
	ClipHist		mHistData;
	const int	mMaxItems;
};

#endif // !defined(AFX_CLIPBOARDHISTORY_H__8B1A03ED_5044_11D4_B580_00C04F8CF986__INCLUDED_)
