/*
* ClipWatch clipboard extender/history/utility
* Copyright (C) 2001-2004, 2009, 2013-2014, 2018, 2020 Sean Echevarria
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

// ClipWatchFrame.cpp : Implementation of ClipWatchFrame
#include "stdafx.h"
#include "ClipWatchFrame.h"
#include <WinUser.h>
#include "ClipboardHistory.h"
#include "TaskBarWnd.h"
#include "IniFile.h"
#include "FTrace.h"
#include <commctrl.h>
#include <VersionHelpers.h>


static int s_colTextW;
static int s_colSizeW;
static int s_colPinW;
static int s_rcWidth;
static int s_rcHeight;


/////////////////////////////////////////////////////////////////////////////
// ClipWatchFrame

ClipWatchFrame::ClipWatchFrame(
	TaskBarWnd *taskWnd, 
	AppSettings settings, 
	std::shared_ptr<ClipboardHistory> clipHistory, 
	BOOL show /*= TRUE*/)  :
 	mTaskWnd(taskWnd),
	mOptionsMenu(nullptr),
	mMainMenu(nullptr),
	mAttemptAutoPaste(FALSE),
	mTargetWnd(nullptr),
	mForegroundWnd(nullptr),
	mSettings(settings),
	mClipHist(clipHistory)
{
	FTRACE(L"ClipWatchFrame::ctor\n");
	_ASSERTE(mTaskWnd);
	_ASSERTE(mClipHist);
	RECT rcPos;
	POINT	pt;

	static bool once = true;
	if (once)
	{
		s_colTextW = mSettings->GetValueInt(L"settings", L"textColWidth", 260);
		s_colSizeW = mSettings->GetValueInt(L"settings", L"sizeColWidth", 60);
		s_colPinW = mSettings->GetValueInt(L"settings", L"pinColWidth", 30);
		s_rcHeight = mSettings->GetValueInt(L"settings", L"height", 240);
		s_rcWidth = mSettings->GetValueInt(L"settings", L"width");
		if (!s_rcWidth)
			s_rcWidth = s_colTextW + s_colSizeW + s_colPinW
						+(GetSystemMetrics(SM_CYSIZEFRAME)*2)
						+GetSystemMetrics(SM_CXVSCROLL);
		once = false;
	}

	mTargetWndClassName[0] = L'\0';

	// do this before creating our wnds
	if (show)
		SetTargetWindow();

	// setup display rect
	// display so that mouse cursor is inside top left corner of window
	::GetCursorPos(&pt);
	
	static const int kCursorOffsetFudgeW = 50;
	static const int kCursorOffsetFudgeH = 100;
	rcPos.top = pt.y-kCursorOffsetFudgeH;
	rcPos.left = pt.x-kCursorOffsetFudgeW;
	rcPos.right = rcPos.left+s_rcWidth;
	rcPos.bottom = rcPos.top+s_rcHeight;

	// but adjust so that it is wholly visible
	RECT rcDesktop;
	HWND hCurWnd = ::WindowFromPoint(pt);
	::GetWindowRect(::GetDesktopWindow(), &rcDesktop);
	if (hCurWnd)
	{
		HMONITOR hMonitor = ::MonitorFromWindow(hCurWnd, MONITOR_DEFAULTTOPRIMARY);
		if (hMonitor)
		{
			MONITORINFO mi;
			mi.cbSize = sizeof(mi);
			if (::GetMonitorInfo(hMonitor, &mi))
				rcDesktop = mi.rcMonitor;
		}
	}

	if (!(rcPos.top >= rcDesktop.top &&
		rcPos.left >= rcDesktop.left &&
		rcPos.bottom <= rcDesktop.bottom &&
		rcPos.right <= rcDesktop.right))
	{
		const int offsetDesktop = 50;
		int diff;
		if (rcPos.top < rcDesktop.top)
		{
			diff = abs(rcPos.top - rcDesktop.top) + offsetDesktop;
			rcPos.top += diff;
			rcPos.bottom += diff;
		}
		if (rcPos.left < rcDesktop.left)
		{
			diff = abs(rcPos.left - rcDesktop.left) + offsetDesktop;
			rcPos.left += diff;
			rcPos.right += diff;
		}
		if (rcPos.bottom > rcDesktop.bottom)
		{
			diff = ::abs(rcPos.bottom - rcDesktop.bottom) + offsetDesktop;
			rcPos.bottom -= diff;
			rcPos.top -= diff;
		}
		if (rcPos.right > rcDesktop.right)
		{
			diff = ::abs(rcPos.right - rcDesktop.right) + offsetDesktop;
			rcPos.right -= diff;
			rcPos.left -= diff;
		}
	}

	const HICON hIcon = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_MAINFRAME_ENABLE));
	GetWndClassInfo().m_wc.hIcon = hIcon;
	mMainMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME));
	mAccelerators = ::LoadAccelerators(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_ACCELERATORS));
	Create(::GetDesktopWindow(), rcPos, CLIP_DISPLAY_WND_TITLE, 0, 0, (UINT)mMainMenu);

	if (show)
	{
		ShowWindow(SW_SHOWNORMAL);
		::SetForegroundWindow(m_hWnd);
	}
}

ClipWatchFrame::~ClipWatchFrame()
{
	FTRACE(L"ClipWatchFrame::dtor\n");
	_ASSERTE(mTaskWnd);
	if (DoAutopaste() && mTargetWnd)
	{
		const HWND kOriginalTarget = mTargetWnd;
		HWND hLastParent = kOriginalTarget;
		while (hLastParent != nullptr)
		{
			mTargetWnd = hLastParent;
			hLastParent = ::GetParent(mTargetWnd);

			WCHAR clsName[kClassNameLength] = L"";
			if (::GetClassName(mTargetWnd, clsName, kClassNameLength))
			{
				if (wcsstr(clsName, L"32770"))
				{
					FTRACE2(L"break on dialog %08x, parent %08x\n", mTargetWnd, hLastParent);
					if (!::wcscmp(mTargetWndClassName, L"edit"))
						mTargetWnd = kOriginalTarget;
					break;
				}
				else if (::wcsstr(clsName, L"Xaml Host"))
				{
					// [cwissue:2]
					FTRACE2(L"break on Xaml Host %08x, parent %08x\n", mTargetWnd, hLastParent);
					mTargetWnd = nullptr;
					break;
				}
			}

			LONG style, exStyle;
			style = ::GetWindowLong(mTargetWnd, GWL_STYLE);
			exStyle = ::GetWindowLong(mTargetWnd, GWL_EXSTYLE);
			if (((style & WS_DLGFRAME) && (style & DS_MODALFRAME)) ||
				(exStyle & WS_EX_APPWINDOW) || (exStyle & WS_EX_DLGMODALFRAME))
			{
				FTRACE2(L"break on style %08x, parent %08x\n", mTargetWnd, hLastParent);
				break;
			}
		}

		if (mTargetWnd)
		{
			if (::SetForegroundWindow(mTargetWnd))
			{
				FTRACE1(L"SetForegroundWindow succeeded %08x\n", mTargetWnd);
				if (mForegroundWnd != mTargetWnd)
				{
					if (::SetForegroundWindow(mForegroundWnd))
						FTRACE1(L"SetForegroundWindow 2 succeeded %08x\n", mForegroundWnd);
					else
						FTRACE1(L"FAIL: SetForegroundWindow 2 %08x\n", mForegroundWnd);
				}
			}
			else
				FTRACE1(L"FAIL: SetForegroundWindow %08x\n", mTargetWnd);

			mTargetWnd = nullptr;
		}
		mForegroundWnd = nullptr;
	}
}

LRESULT
ClipWatchFrame::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	FTRACE(L"ClipWatchFrame::OnCreate\n");
	RECT rcClient;
	GetClientRect(&rcClient);
	USES_CONVERSION;

	// setup the list ctrl
	mListview.Create(m_hWnd, rcClient, nullptr,
		WS_CHILD|WS_VISIBLE|WS_VSCROLL|LVS_REPORT|LVS_SHAREIMAGELISTS|LVS_SINGLESEL|LVS_SHOWSELALWAYS,
		0,
		IDC_LISTVIEW);
	mListview.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, /*LVS_EX_TRACKSELECT|*/LVS_EX_FULLROWSELECT);
	mListview.InsertColumn(0, L"Clipboard Text", LVCFMT_LEFT, s_colTextW, -1);
	mListview.InsertColumn(1, L"Size", LVCFMT_LEFT,	s_colSizeW, -1);
	mListview.InsertColumn(2, L"Pin", LVCFMT_LEFT,	s_colPinW, -1);
//	mListview.SetHoverTime(1);

	UpdateData();
	mListview.SetFocus();

	bHandled = false;
	return 0;
}

LRESULT
ClipWatchFrame::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	FTRACE(L"ClipWatchFrame::OnDestroy\n");
	mTaskWnd->MainWndDestroyed();
	SaveDimensions();

	if (mMainMenu)
	{
		::DestroyMenu(mMainMenu);
		mMainMenu = nullptr;
	}

	if (mAccelerators)
	{
		::DestroyAcceleratorTable(mAccelerators);
		mAccelerators = nullptr;
	}

	bHandled = FALSE;
	return 0;
}

bool
ClipWatchFrame::SelectListViewItem(bool doPaste /*= true*/)
{
	int item = mListview.GetSelectedIndex();
	if (item == -1)
		return false;

	bool pinned;
	CString strItem(mClipHist->GetItem(GetClipItemHistIndex(item), pinned));
	if (!strItem.GetLength())
		return false;

	if (::OpenClipboard(m_hWnd))
	{
		// add to clipboard and then call paste
		HGLOBAL hData = ::GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, (strItem.GetLength() + 1) * sizeof(WCHAR));
		LPVOID pTxt = ::GlobalLock(hData);
		::memcpy(pTxt, strItem, (strItem.GetLength() + 1) * sizeof(WCHAR));
		
		::EmptyClipboard();
		::SetClipboardData(CF_UNICODETEXT, hData);
		::CloseClipboard();
		// The Clipboard now owns the allocated memory and will delete this
		//  data object when new data is put on the Clipboard
		::GlobalUnlock(hData);

		if (doPaste)
			mAttemptAutoPaste = TRUE;
		PostMessage(WM_CLOSE, 0, 0);
		return true;
	}

	return false;
}

bool
ClipWatchFrame::DoAutopaste()
{
	bool retval = false;

	if (mAttemptAutoPaste && mTaskWnd->GetAutopaste())
	{
		if (mTargetWnd && ::IsWindow(mTargetWnd))
		{
			FTRACE1(L"mTargetWnd: %08x\n", mTargetWnd);
			bool keepTrying = true;

			// mode 1
			if (TargetWndSupportsPasteCmd())
			{
				if (::SendMessage(mTargetWnd, WM_COMMAND, ID_EDIT_PASTE, 0) == 1)
				{
					FTRACE(L"ID_EDIT_PASTE succeeded\n");
					keepTrying = false;
				}
				else
				{
					FTRACE(L"FAIL: ID_EDIT_PASTE\n");
				}
			}

			// mode 2
			if (keepTrying && TargetWndSupportsPasteMsg())
			{
				FTRACE(L"sending WM_PASTE\n");
				// WM_PASTE has no return value - so can't check for success/fail
				::SendMessage(mTargetWnd, WM_PASTE, 0, 0);
				keepTrying = false;
			}

			// mode 3
			if (keepTrying && mTaskWnd->GetSendinput() && TargetWndSupportsStandardPasteAccel())
			{
				FTRACE(L"attempting SendInput\n");
				// bug in SendInput: KEYEVENTF_KEYUP does not work for VK_CONTROL
				::keybd_event(VK_CONTROL, 0, 0, 0);
				::keybd_event(L'V', 0, 0, 0);
				::keybd_event(L'V', 0, KEYEVENTF_KEYUP, 0);
				::keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
				keepTrying = false;
			}

			retval = true;
		}
		else
		{
			FTRACE1(L"bad targetWnd %08x\n", mTargetWnd);
			mTargetWnd = nullptr;
		}
	}

	return retval;
}

void
ClipWatchFrame::UpdateData(CString filterTxt /*= CString()*/)
{
	HCURSOR hcurPrev = ::SetCursor((HCURSOR)IDC_WAIT);
	const int maxLen = 205;
	CString str;
	LV_ITEM lvi;

	ZeroMemory(&lvi, sizeof(LV_ITEM));
	mListview.SetRedraw(FALSE);
	mListview.DeleteAllItems();

	ClipItemIndex prevIdxes;
	const bool filterCurrentSet = filterTxt.GetLength() && 0 == filterTxt.Find(mFilterText) && !mClipIndexes.empty();
	if (filterCurrentSet)
		prevIdxes.swap(mClipIndexes);
	mClipIndexes.clear();
	SetFilterText(filterTxt);

	int insertIdx = 0;
	for (int idx = 0; true; idx++)
	{
		int clipHistIdx = idx;
		if (filterCurrentSet)
		{
			if (idx >= (int)prevIdxes.size())
				break;

			clipHistIdx = prevIdxes[idx];
		}

		bool pinned;
		str = mClipHist->GetItem(clipHistIdx, pinned);
		if (str.IsEmpty())
			break;

		if (filterTxt.GetLength() && !ShouldDisplayItem(str))
			continue;

		mClipIndexes.push_back(clipHistIdx);

		const int sLen = str.GetLength();

		// limit amount displayed
		if (sLen > maxLen)
		{
			str.SetAt(maxLen-4, L'.');
			str.SetAt(maxLen-3, L'.');
			str.SetAt(maxLen-2, L'.');
			str = str.Left(maxLen - 1);
		}

		// special case chars
		if (str.FindOneOf(L"\r\n\t\f") != -1)
		{
			CString sav(str);
			const int newLen = str.GetLength();
			bool hadSpace = false;
			str = L"";
			for (int idx = 0; idx<newLen; idx++)
			{
				WCHAR ch = sav[idx];
				switch (ch) {
				case L'\r':
					hadSpace = false;
					str += L"\\r";
					break;
				case L'\n':
					hadSpace = false;
					str += L"\\n";
					break;
				case L'\t':
					hadSpace = false;
					str += L"\\t";
					break;
				case L'\f':
					hadSpace = false;
					str += L"\\f";
					break;
				case L' ':
					if (!hadSpace)
					{
						hadSpace = true;
						str += L' ';
					}
					break;
				default:
					hadSpace = false;
					str += ch;
				}
			}
		}

		// insert text
		lvi.iItem = insertIdx++;
		lvi.iSubItem = 0;
		lvi.pszText = const_cast<LPWSTR>((LPCWSTR)str);
		lvi.mask = LVIF_TEXT;
		mListview.InsertItem(&lvi);

		// insert clipboard text length
		lvi.iSubItem = 1;
		WCHAR sizeStr[20];
		wsprintf(sizeStr, L"%d", sLen);
		lvi.pszText = sizeStr;
		mListview.SetItem(&lvi);

		if (pinned)
		{
			lvi.iSubItem = 2;
			WCHAR pinStr[2] = L"";
			wcscpy_s(pinStr, L"+");
			lvi.pszText = pinStr;
			mListview.SetItem(&lvi);
		}
	}

	_ASSERTE(insertIdx == (int)mClipIndexes.size());

	if (mListview.GetItemCount())
		mListview.SetItemState(0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	mListview.SetRedraw();

	::SetCursor(hcurPrev);
}

void
ClipWatchFrame::SetFilterText(const CString &filterTxt)
{
	mFilterText = filterTxt;

	mCaseSensitive = false;

	// if filterTxt has any upper-case letters, then assume case-sensitive, 
	// otherwise case-insensitive

	for (int idx = 0; idx < filterTxt.GetLength(); ++idx)
	{
		TCHAR ch = filterTxt[idx];
		if (isupper(ch))
		{
			mCaseSensitive = true;
			break;
		}
	}
}

void
ClipWatchFrame::SaveDimensions()
{
	// save sizes for next invoke
	WINDOWPLACEMENT wp;
	ZeroMemory(&wp, sizeof(WINDOWPLACEMENT));
	wp.length = sizeof(WINDOWPLACEMENT);
	if (GetWindowPlacement(&wp))
	{
		s_rcWidth = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
		s_rcHeight = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
	}
	s_colTextW = mListview.GetColumnWidth(0);
	s_colSizeW = mListview.GetColumnWidth(1);
	s_colPinW = mListview.GetColumnWidth(2);

	mSettings->SetValueInt(L"settings", L"width", s_rcWidth);
	mSettings->SetValueInt(L"settings", L"height", s_rcHeight);
	mSettings->SetValueInt(L"settings", L"textColWidth", s_colTextW);
	mSettings->SetValueInt(L"settings", L"sizeColWidth", s_colSizeW);
	mSettings->SetValueInt(L"settings", L"pinColWidth", s_colPinW);
}

LRESULT
ClipWatchFrame::OnAppExit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	mTaskWnd->PostMessage(WM_COMMAND, ID_APP_EXIT, 0);
	bHandled = true;
	return 1;
}

LRESULT
ClipWatchFrame::OnActivate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	mTaskWnd->SendMessage(WM_COMMAND, IDC_ACTIVE, 0);
	bHandled = true;
	return 1;
}

LRESULT
ClipWatchFrame::OnSuspend(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	mTaskWnd->SendMessage(WM_COMMAND, IDC_SUSPEND, 0);
	bHandled = true;
	return 1;
}

LRESULT
ClipWatchFrame::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	switch (wParam) {
	case VK_DOWN:
	case VK_UP:
	case VK_NEXT:
	case VK_PRIOR:
	case VK_RIGHT:
	case VK_LEFT:
	case VK_HOME:
	case VK_END:
		mListview.SetFocus();
		mListview.SendMessage(uMsg, wParam, lParam);
		bHandled = TRUE;
		return 1;
	}
	bHandled = FALSE;
	return 0;
}

LRESULT
ClipWatchFrame::OnListViewKeyDown(int idCtrl, LPNMHDR notifyCode, BOOL& bHandled)
{
	LPNMLVKEYDOWN keyInfo = reinterpret_cast<LPNMLVKEYDOWN>(notifyCode);
	if (keyInfo->wVKey == VK_RETURN)
	{
		SelectListViewItem();
		bHandled = TRUE;
		return 1;
	}

	bHandled = FALSE;
	return 0;
}

LRESULT
ClipWatchFrame::OnInitMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	const HMENU hMen = reinterpret_cast<HMENU>(wParam);

	const HMENU hEditMenu = ::GetSubMenu(hMen, 1);
	if (hEditMenu)
	{
		int item = mListview.GetSelectedIndex();
		if (item != -1)
		{
			bool pinned;
			const CString strItem(mClipHist->GetItem(GetClipItemHistIndex(item), pinned));
			::CheckMenuItem(hEditMenu, ID_EDIT_PIN, 
				(pinned ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		}
	}

	mOptionsMenu = ::GetSubMenu(hMen, 2);
	if (mOptionsMenu)
	{
		::CheckMenuRadioItem(mOptionsMenu, IDC_ACTIVE, IDC_SUSPEND,
					(mTaskWnd->GetMonitorActive() ? IDC_ACTIVE : IDC_SUSPEND), MF_BYCOMMAND);
		::CheckMenuItem(mOptionsMenu, IDM_AUTOPASTE, 
					(mTaskWnd->GetAutopaste() ? MF_CHECKED : MF_UNCHECKED) |MF_BYCOMMAND);
		::CheckMenuItem(mOptionsMenu, IDM_SENDINPUT, 
					(mTaskWnd->GetSendinput() ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		if (!mTaskWnd->GetAutopaste())
			::EnableMenuItem(mOptionsMenu, IDM_SENDINPUT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		::CheckMenuItem(mOptionsMenu, ID_POPUP_ENABLEEXTRAHOTKEYS, 
					(mTaskWnd->GetExtraHotkeysEnabled() ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
	}

	bHandled = FALSE;
	return 0;
}

LRESULT
ClipWatchFrame::OnClearHistory(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	mTaskWnd->SendMessage(WM_COMMAND, IDC_CLEARHISTORY, 0);
	bHandled = true;
	return 1;
}

LRESULT
ClipWatchFrame::OnAutopaste(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	mTaskWnd->SendMessage(WM_COMMAND, IDM_AUTOPASTE, 0);
	if (mTaskWnd->GetAutopaste())
		::EnableMenuItem(mOptionsMenu, IDM_SENDINPUT, MF_BYCOMMAND | MF_ENABLED & ~MF_GRAYED);
	else
		::EnableMenuItem(mOptionsMenu, IDM_SENDINPUT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

	bHandled = true;
	return 1;
}

LRESULT
ClipWatchFrame::OnSendinput(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	mTaskWnd->SendMessage(WM_COMMAND, IDM_SENDINPUT, 0);
	bHandled = true;
	return 1;
}

LRESULT
ClipWatchFrame::OnSize(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*lResult*/)
{
	RECT rcClient;
	GetClientRect(&rcClient);

	if (mInSearchMode)
	{
		NONCLIENTMETRICS ncm{ sizeof(NONCLIENTMETRICS), 0 };
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
		const int kEditHt = ncm.iMenuHeight;
		const int kClientHt = rcClient.bottom - rcClient.top;
		rcClient.bottom = rcClient.top + kEditHt;
		mEditCtrl.MoveWindow(&rcClient);
		rcClient.top = kEditHt + 1;
		rcClient.bottom = rcClient.top + kClientHt - kEditHt;
	}

	// setup the list ctrl
	mListview.MoveWindow(&rcClient);
	int newWidth = rcClient.right - rcClient.left;
	int scrollWidth = 0;
	if (mListview.GetStyle() & WS_VSCROLL)
		scrollWidth = GetSystemMetrics(SM_CXVSCROLL);
	s_colSizeW = mListview.GetColumnWidth(1);
	s_colPinW = mListview.GetColumnWidth(2);
	newWidth -= (scrollWidth + s_colSizeW + s_colPinW);
	if (newWidth > 60)	// don't auto shrink beyond some minimum
		mListview.SetColumnWidth(0, newWidth);
	return 0;
}

LRESULT
ClipWatchFrame::OnFileSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	mClipHist->Save();
	bHandled = true;
	return 1;
}

LRESULT
ClipWatchFrame::OnListViewRightClick(int /*idCtrl*/, LPNMHDR /*notifyCode*/, BOOL& bHandled)
{
	bHandled = TRUE;
	const int itemIdx = mListview.GetSelectedIndex();
	if (itemIdx == -1)
		return 1;

	POINT	pt;
	HMENU	hMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_LISTCONTEXT));
	HMENU	hPopMenu = ::GetSubMenu(hMenu, 0);
	
	::GetCursorPos(&pt);
	::SetForegroundWindow(m_hWnd); // Q135788
	::SetMenuDefaultItem(hPopMenu, ID_EDIT_PASTE, MF_BYCOMMAND);
	bool pinned;
	const CString strItem(mClipHist->GetItem(GetClipItemHistIndex(itemIdx), pinned));
	::CheckMenuItem(hPopMenu, ID_EDIT_PIN, (pinned ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

	::TrackPopupMenu(hPopMenu, TPM_LEFTALIGN|TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, nullptr);
	PostMessage(WM_NULL); // Q135788
	::DestroyMenu(hMenu);
	::DestroyMenu(hPopMenu);

	return 1;
}

int
ClipWatchFrame::ExecuteItem(int itemIdx)
{
	bool pinned;
	CString strItem(mClipHist->GetItem(GetClipItemHistIndex(itemIdx), pinned));
	if (!strItem.GetLength())
		return 0;

	// strip whitespace if URL
	bool strip = false;
	int pos;

	pos = strItem.Find(L"http://");
	if (-1 != pos && pos < 10)
		strip = true;

	if (!strip)
	{
		pos = strItem.Find(L"www");
		if (-1 != pos && pos < 10)
			strip = true;
	}

	if (!strip)
	{
		pos = strItem.Find(L"https://");
		if (-1 != pos && pos < 10)
			strip = true;
	}

	if (strip)
	{
		if ((pos = strItem.FindOneOf(L" \t\r\n")) != -1)
			strItem = strItem.Left(pos);
	}

	// move to head of list (and save modified version if stripped)
	mClipHist->MoveToHead(GetClipItemHistIndex(itemIdx), strItem);

	::ShellExecute(::GetDesktopWindow(), L"open", strItem, nullptr, nullptr, SW_SHOW);

	PostMessage(WM_CLOSE, 0, 0);
	return 1;
}

void
ClipWatchFrame::RedisplayWindow()
{
	SetTargetWindow();
	ShowWindow(SW_SHOWNORMAL);
	::SetForegroundWindow(m_hWnd);
}

void
ClipWatchFrame::SetTargetWindow()
{
	mForegroundWnd = mTargetWnd = ::GetForegroundWindow();
	GUITHREADINFO gti;
	gti.cbSize = sizeof(GUITHREADINFO);
	if (::GetGUIThreadInfo(0, &gti) && gti.hwndFocus)
	{
		if (mTargetWnd != gti.hwndFocus)
		{
			FTRACE2(L"foc: %08x -> %08x\n", mTargetWnd, gti.hwndFocus);
			mTargetWnd = gti.hwndFocus;
		}
	}
	else
	{
		FTRACE(L"FAIL: GetGUIThreadInfo\n");
	}

	mTargetWndClassName[0] = L'\0';
	if (mTargetWnd)
	{
		::GetClassName(mTargetWnd, mTargetWndClassName, kClassNameLength);
		if (mTargetWndClassName[0])
			::_wcslwr_s(mTargetWndClassName);
	}
}

int
ClipWatchFrame::GetClipItemHistIndex(int uiItemIdx) const
{
	_ASSERTE(uiItemIdx < (int)mClipIndexes.size());
	return mClipIndexes[uiItemIdx];
}

bool
ClipWatchFrame::ShouldDisplayItem(CString item) const
{
	if (!mCaseSensitive)
		item.MakeLower();

	for (int idx = 0; ;)
	{
		CString curFilt(mFilterText.Tokenize(L" ", idx));
		if (curFilt.IsEmpty())
			break;

		if (-1 == item.Find(curFilt))
			return false;
	}

	return true;
}

bool
ClipWatchFrame::IsTargetWndListed(LPCWSTR iniSectionName,
								  LPCWSTR iniItemName, 
								  LPCWSTR defaultVal /*= nullptr*/) const
{
	bool retval = false;
	if (mTargetWndClassName[0])
	{
		LPCWSTR items = mSettings->GetValue(iniSectionName, iniItemName, defaultVal);
		if (items)
		{
			int itemsLen = ::wcslen(items) + 1;
			std::vector<WCHAR> exCopy(itemsLen);
			::wcscpy_s(&exCopy[0], itemsLen, items);
			::_wcslwr_s(&exCopy[0], itemsLen);
			WCHAR * nextTok = nullptr;

			const WCHAR kDelimiters[] = L";";
			WCHAR * token = ::wcstok_s(&exCopy[0], kDelimiters, &nextTok);

			while (token != nullptr)
			{
				if (!::wcscmp(mTargetWndClassName, token))
				{
					retval = true;
					break;
				}
				token = ::wcstok_s(nullptr, kDelimiters, &nextTok);
			}
		}
	}
	return retval;
}

bool
ClipWatchFrame::IsTargetWndExcluded(LPCWSTR iniItemName, 
									LPCWSTR defaultVal /*= nullptr*/) const
{
	return IsTargetWndListed(L"exclusions", iniItemName, defaultVal);
}

bool
ClipWatchFrame::IsTargetWndIncluded(LPCWSTR iniItemName, 
									LPCWSTR defaultVal /*= nullptr*/) const
{
	return IsTargetWndListed(L"inclusions", iniItemName, defaultVal);
}

bool
ClipWatchFrame::TargetWndSupportsPasteCmd() const
{
	// default to true; need only check for exclusions
	return !IsTargetWndExcluded(L"mode1");
}

bool
ClipWatchFrame::TargetWndSupportsPasteMsg() const
{
	// default to false; check for inclusions first, then check criteria and then exclusions
	bool retval = false;
	if (mTargetWndClassName[0])
	{
		retval = IsTargetWndIncluded(L"mode2");
		if (!retval && ::wcsstr(mTargetWndClassName, L"edit"))
		{
			return !IsTargetWndExcluded(L"mode2", L"VsTextEditPane;");
		}
	}
	return retval;
}

bool
ClipWatchFrame::TargetWndSupportsStandardPasteAccel() const
{
	// default to true; need only check for exclusions
	return !IsTargetWndExcluded(L"mode3");
}

void
ClipWatchFrame::OnFinalMessage(HWND /*hWnd*/)
{
	delete this;
}

LRESULT
ClipWatchFrame::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	ShowWindow(SW_MINIMIZE);	// causes working set to get flushed
	bHandled = FALSE;
	return 0;
}

LRESULT
ClipWatchFrame::OnWindowClose(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	PostMessage(WM_CLOSE, 0, 0);
	bHandled = true;
	return 1;
}

LRESULT
ClipWatchFrame::OnListViewSelect(int /*idCtrl*/, LPNMHDR /*notifyCode*/, BOOL& bHandled)
{
	SelectListViewItem();
	bHandled = TRUE;
	return 1;
}

LRESULT
ClipWatchFrame::OnCopyItemToClipboard(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	SelectListViewItem(false);
	bHandled = TRUE;
	return 1;
}

LRESULT
ClipWatchFrame::OnPasteItemExternal(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	SelectListViewItem();
	bHandled = TRUE;
	return 1;
}

LRESULT
ClipWatchFrame::OnSelectAll(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	HWND foc = GetFocus();
	if (mEditCtrl.m_hWnd == foc)
	{
		mEditCtrl.SetSel(0, -1);
		bHandled = TRUE;
	}

	return S_OK;
}

LRESULT 
ClipWatchFrame::OnDeleteItem(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	HWND foc = GetFocus();
	if (mEditCtrl.m_hWnd == foc)
	{
		const CString filterTxt(GetTextFromFilterEdit());
		int st = 0, en = 0;
		mEditCtrl.GetSel(st, en);
		// if has selection, or no selection and caret is not at end of text, then send back to the search edit
		if (st != en || st != filterTxt.GetLength())
		{
			mEditCtrl.SendMessage(WM_KEYDOWN, VK_DELETE);
			OnFilterEditTextChange(EN_CHANGE, IDC_FILTER_EDIT, mEditCtrl.m_hWnd, bHandled);
			bHandled = true;
			return 1;
		}
		
		// no selection and caret is at end of filter text, so allow delete of list item
	}
	else if (mListview.m_hWnd != foc)
	{
		// this is an else because we want to allow DEL to work in some 
		// cases when focus is in the search edit
		return 0;
	}

	int item = mListview.GetSelectedIndex();
	if (item == -1)
		return 0;

	const int clipHistIdx = GetClipItemHistIndex(item);

	bool doDelete = true;
	bool pinned;
	const CString strItem(mClipHist->GetItem(clipHistIdx, pinned));
	if (pinned)
	{
		if (MessageBox(L"This is a pinned item.  Are you sure you want to delete it?", L"Delete Pinned Item?", MB_YESNO) == IDNO)
			doDelete = false;
	}

	if (doDelete)
	{
		mClipHist->Remove(clipHistIdx);
		mClipIndexes.clear();
		UpdateData(mFilterText);
		mListview.SetItemState(item, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		if (!ListView_IsItemVisible(mListview.m_hWnd, item))
		{
			mListview.EnsureVisible(item, FALSE);
			for (int idx = 0; idx < 5; ++idx)
				mListview.SendMessage(WM_VSCROLL, SB_LINEDOWN, 0);
		}
		mListview.EnsureVisible(item, FALSE);
	}

	bHandled = true;
	return 1;
}

LRESULT
ClipWatchFrame::OnControlNavigate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	HWND foc = GetFocus();
	if (mEditCtrl.m_hWnd == foc)
		mListview.SetFocus();
	else if (mListview.m_hWnd == foc)
		if (mEditCtrl.IsWindow() && mEditCtrl.IsWindowVisible())
			mEditCtrl.SetFocus();

	bHandled = true;
	return 1;
}

LRESULT
ClipWatchFrame::OnPinItem(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	int item = mListview.GetSelectedIndex();
	if (item == -1)
		return 0;

	mClipHist->TogglePin(GetClipItemHistIndex(item));
	UpdateData(mFilterText);
	mListview.SetItemState(item, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	if (!ListView_IsItemVisible(mListview.m_hWnd, item))
	{
		mListview.EnsureVisible(item, FALSE);
		for (int idx = 0; idx < 5; ++idx)
			mListview.SendMessage(WM_VSCROLL, SB_LINEDOWN, 0);
	}
	mListview.EnsureVisible(item, FALSE);
	mClipHist->Save();
	bHandled = TRUE;
	return 1;
}

LRESULT
ClipWatchFrame::OnToggleSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
	mInSearchMode = !mInSearchMode;

	if (mInSearchMode)
	{
		if (!mEditCtrl.m_hWnd)
		{
			// setup the filter edit ctrl
			mEditCtrl.Create(m_hWnd, RECT(), nullptr,
				WS_CHILD,
				0,
				IDC_FILTER_EDIT);

			HFONT fnt = mListview.GetFont();
			mEditCtrl.SetFont(fnt);
			mEditCtrl.SetForwardingTarget(mListview.m_hWnd);
		}

		const CString filterTxt(GetTextFromFilterEdit());
		if (filterTxt.GetLength())
		{
			mPendingFilter = true;
			mFilterTimerId = 100;
			SetTimer(mFilterTimerId, 250);
		}
	}
	else
	{
		KillTimer(mFilterTimerId);
		mFilterTimerId = 0;
		mPendingFilter = false;

		// restore full, unfiltered list
		UpdateData();
	}

	BOOL junk;
	OnSize(0, 0, 0, junk);
	mEditCtrl.ShowWindow(mInSearchMode ? SW_SHOW : SW_HIDE);
	if (mInSearchMode)
		mEditCtrl.SetFocus();

	bHandled = TRUE;
	return 1;
}

LRESULT
ClipWatchFrame::OnExecuteItem(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	int item = mListview.GetSelectedIndex();
	if (item == -1)
		return 0;

	ExecuteItem(item);
	bHandled = TRUE;
	return 1;
}

LRESULT
ClipWatchFrame::OnToggleHotKeys(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	return mTaskWnd->OnToggleExtraHotKeys(wNotifyCode, wID, hWndCtl, bHandled);
}

LRESULT
ClipWatchFrame::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = TRUE;

	if (!mEditCtrl.IsWindow() || !mEditCtrl.IsWindowVisible())
	{
		_ASSERTE(!mInSearchMode);
		KillTimer(mFilterTimerId);
		mFilterTimerId = 0;
		mPendingFilter = false;
		return S_OK;
	}

	_ASSERTE(mInSearchMode);
	CString filterTxt(GetTextFromFilterEdit());
	mPendingFilter = false;
	if (filterTxt != mFilterText)
		UpdateData(filterTxt);

	return S_OK;
}

CString
ClipWatchFrame::GetTextFromFilterEdit()
{
	CString filterTxt;
	int nLen = mEditCtrl.GetWindowTextLength();
	mEditCtrl.GetWindowText(filterTxt.GetBufferSetLength(nLen), nLen + 1);
	filterTxt.ReleaseBuffer();
	filterTxt.Trim();
	return filterTxt;
}

LRESULT
ClipWatchFrame::OnFilterEditTextChange(int code, int idCtrl, HWND hwndCtl, BOOL& bHandled)
{
	_ASSERTE(idCtrl == IDC_FILTER_EDIT);
	_ASSERTE(code == EN_CHANGE);
	_ASSERTE(hwndCtl == mEditCtrl.m_hWnd);
	KillTimer(mFilterTimerId);
	mPendingFilter = true;
	SetTimer(mFilterTimerId, 250);
	bHandled = TRUE;
	return S_OK;
}

bool
ClipWatchFrame::TranslateAccelerator(MSG* msg)
{
	if (::TranslateAccelerator(m_hWnd, mAccelerators, msg))
		return true;

	return false;
}

LRESULT
ClipWatchFrame::OnHelp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	CString helpTxt;
	helpTxt.Format(
		L"Press %s at any time to display the clip history window.\r\n\r\nIn the list, press Ctrl+P to pin (or unpin) items in the list to prevent them from being pruned.\r\nRight-Click on list items for additional actions.\r\n", 
		mTaskWnd->GetMainHotkey());

	helpTxt += L"\r\nEnable Extra Hotkeys (on Options menu) for the following:\r\n";
	if (!::IsWindows7OrGreater())
	{
		helpTxt += L"Maximize active window \tWin+Up\r\n";
		helpTxt += L"Minimize active window \tWin+Down\r\n";
	}
	helpTxt += L"Move active window left \tCtrl+Alt+Shift+Left\r\n";
	helpTxt += L"Move active window right \tCtrl+Alt+Shift+Right\r\n";
	helpTxt += L"Move active window up \tCtrl+Alt+Shift+Up\r\n";
	helpTxt += L"Move active window down \tCtrl+Alt+Shift+Down\r\n";

	MessageBox(helpTxt, L"Help", MB_OK | MB_ICONINFORMATION);
	bHandled = TRUE;
	return 1;
}

LRESULT
ClipWatchFrame::OnAbout(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	CString msgTxt;
	msgTxt.Format(L"ClipWatch version something or another.\r\n"
		"\r\n"
		"https://github.com/sean-e/ClipWatch/\r\n"
		"https://sourceforge.net/projects/clipwatch/\r\n"
		"\r\n"
		"Copyright 2001-2009, 2013-2014, 2018, 2020 Sean Echevarria");

	MessageBox(msgTxt, L"About", MB_OK | MB_ICONINFORMATION);
	bHandled = TRUE;
	return 1;
}
