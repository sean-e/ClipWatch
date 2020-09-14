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

// TaskBarWnd.cpp : Implementation of TaskBarWnd
#include "stdafx.h"
#include "TaskBarWnd.h"
#include <string>
#include <VersionHelpers.h>
#include "FTrace.h"


const UINT WM_MY_NOTIFYICON = WM_APP + 1;
UINT WM_TASKBARCREATED = 0;
ClipboardHistory *g_pClipHistory = nullptr;


/////////////////////////////////////////////////////////////////////////////
// TaskBarWnd

TaskBarWnd::TaskBarWnd(AppSettings settings) :
	mNextWnd(nullptr),
	mAmInChain(false),
	mRechainTimerId(0),
	mAppSettings(settings),
	mMainWnd(nullptr)
{
	LPCWSTR pChStr = mAppSettings->GetValue(L"settings", L"hotkey", L"V");
	WCHAR chStr[2] = L"";
	if (islower(pChStr[0]))
		chStr[0] = _toupper(pChStr[0]);
	else
		chStr[0] = pChStr[0];
	mAppSettings->SetValue(L"settings", L"hotkey", chStr);

	UINT hotkeyMod=0, hotkeyMod2=0;
	for (int idx = 2; idx; idx--)
	{
		LPCWSTR lpHotkeyMod;
		if (idx == 2)
			lpHotkeyMod = mAppSettings->GetValue(L"settings", L"hotkeyModifier1", L"CTRL");
		else
			lpHotkeyMod = mAppSettings->GetValue(L"settings", L"hotkeyModifier2", L"SHIFT");

		if (lpHotkeyMod && _wcsicmp(lpHotkeyMod, L"CTRL") == 0)
		{
			hotkeyMod = MOD_CONTROL;
			lpHotkeyMod = L"CTRL";
			mClipHistHotkey += L"Ctrl+";
		}
		else if (lpHotkeyMod && _wcsicmp(lpHotkeyMod, L"ALT") == 0)
		{
			hotkeyMod = MOD_ALT;
			lpHotkeyMod = L"ALT";
			mClipHistHotkey += L"Alt+";
		}
		else if (lpHotkeyMod && _wcsicmp(lpHotkeyMod, L"SHIFT") == 0)
		{
			hotkeyMod = MOD_SHIFT;
			lpHotkeyMod = L"SHIFT";
			mClipHistHotkey += L"Shift+";
		}
		else if (lpHotkeyMod && _wcsicmp(lpHotkeyMod, L"WIN") == 0)
		{
			hotkeyMod = MOD_WIN;
			lpHotkeyMod = L"WIN";
			mClipHistHotkey += L"Win+";
		}
		else
		{
			hotkeyMod = 0;
			lpHotkeyMod = L"valid values are CTRL, ALT, SHIFT and WIN";
		}
	
		if (idx == 2)
		{
			mAppSettings->SetValue(L"settings", L"hotkeyModifier1", lpHotkeyMod);
			hotkeyMod2 = hotkeyMod;
		}
		else
			mAppSettings->SetValue(L"settings", L"hotkeyModifier2", lpHotkeyMod);
	}

	mClipHistHotkey += chStr;
	mAutopaste = mAppSettings->GetValueInt(L"settings", L"autopaste", 1);
	mUseTimer = mAppSettings->GetValueInt(L"settings", L"useTimer", 1);
	mSendInput = mAppSettings->GetValueInt(L"settings", L"sendinput", 1);
	mMaxHistoryCount = mAppSettings->GetValueInt(L"settings", L"maxItems", 50);
	mSetOtherHotKeys = mAppSettings->GetValueInt(L"settings", L"windowMoveHotKeys", 1);
	
	mClipHistory = std::make_shared<ClipboardHistory>(mMaxHistoryCount);
	RECT rcPos = { 0, 0, 0, 0 };
	WM_TASKBARCREATED = ::RegisterWindowMessage(L"TaskbarCreated");
	Create(::GetDesktopWindow(), rcPos, WND_NAME, 0, 0, _U_MENUorID((unsigned int)0));
	::RegisterHotKey(m_hWnd, IDC_DISPLAYAPP, hotkeyMod|hotkeyMod2, chStr[0]);

	if (mSetOtherHotKeys)
	{
		mSetOtherHotKeys = false;
		BOOL dummy = false;
		OnToggleExtraHotKeys(0, 0, nullptr, dummy);
	}
}

TaskBarWnd::~TaskBarWnd()
{
}

void
TaskBarWnd::DisplayWindow()
{
	if (mMainWnd)
		mMainWnd->RedisplayWindow();
	else
		mMainWnd = new ClipWatchFrame(this, mAppSettings, mClipHistory);
}

LRESULT
TaskBarWnd::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	OnActivate(0, 0, nullptr, bHandled);
	bHandled = false;
	return 0;
}

LRESULT
TaskBarWnd::OnFileExit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	ClipboardChain(false);
	DestroyWindow();
	::PostQuitMessage(0);
	return 0;
}

LRESULT
TaskBarWnd::OnActivate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	ClipboardChain(true);
	static BOOL once = TRUE;
	DWORD msg;
	if (once)
	{
		msg = NIM_ADD;
		once = FALSE;
	}
	else
		msg = NIM_MODIFY;

	SendTrayNotify(msg, IDI_MAINFRAME_ENABLE, WND_NAME);
	bHandled = true;
	return 1;
}

LRESULT
TaskBarWnd::OnDrawClipboard(UINT uMsg, 
							WPARAM wParam, 
							LPARAM lParam, 
							BOOL& bHandled)
{
	if (mAmInChain && mClipHistory)
	{
		static UINT cbfmts[] = {CF_UNICODETEXT, CF_HDROP, CF_TEXT};
		BOOL bOpenedCB = FALSE;
		BOOL bHistUpdated = FALSE;
		HANDLE hglb = nullptr;

		int fmt = ::GetPriorityClipboardFormat(cbfmts, sizeof(cbfmts));
		if (fmt > 0)
			bOpenedCB = OpenClipboard();
		if (bOpenedCB)
		{
			switch (fmt)
			{
			case CF_UNICODETEXT:
				hglb = ::GetClipboardData(CF_UNICODETEXT);
				if (hglb)
				{
					LPCWSTR lpstr = (LPCWSTR) ::GlobalLock(hglb);
					bHistUpdated = mClipHistory->Add(lpstr, false);
				}
				break;
			case CF_TEXT:
				hglb = ::GetClipboardData(CF_TEXT);
				if (hglb)
				{
					LPCSTR lpstr = (LPCSTR) ::GlobalLock(hglb);
					CString str(lpstr);
					bHistUpdated = mClipHistory->Add(str, false);
				}
				break;
			case CF_HDROP:
				hglb = ::GetClipboardData(CF_HDROP);
				if (hglb)
				{
					HDROP hDrop = (HDROP) ::GlobalLock(hglb);
					const UINT kFileCnt = ::DragQueryFile(hDrop, 0xffffffff, nullptr, 0);
					CString str;
					for (unsigned int idx = 0; idx < kFileCnt; ++idx)
					{
						UINT strSize = ::DragQueryFile(hDrop, idx, nullptr, 0);
						std::vector<WCHAR> pStr(strSize + 2);
						::DragQueryFile(hDrop, idx, &pStr[0], strSize+1);
						str += &pStr[0];
						if (1 != kFileCnt)
							str += L"\r\n";
					}
					bHistUpdated = mClipHistory->Add(str, false);
				}
				break;
			default:
				;// ??
			}
		}

		if (hglb)
			::GlobalUnlock(hglb);
		if (bOpenedCB)
			::CloseClipboard();
		if (bHistUpdated && mMainWnd)
			mMainWnd->UpdateData();
	}
	
	if (mNextWnd)
	{
		if (mNextWnd == m_hWnd)
		{
			FTRACE(L"ERR: TaskBarWnd::OnDrawClipboard nextWnd == this\n");
			mNextWnd = nullptr;
		}
		else
		{
			::SendMessage(mNextWnd, WM_DRAWCLIPBOARD, wParam, lParam);
		}
	}

	bHandled = true;
	return 1;
}

LRESULT
TaskBarWnd::OnDisplayApp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	DisplayWindow();
	if (mMainWnd)
	{
		::SetForegroundWindow(mMainWnd->m_hWnd);
		bHandled = true;
		return kAcknowledgeOpen;
	}

	return 0;
}

LRESULT
TaskBarWnd::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	::UnregisterHotKey(m_hWnd, IDC_DISPLAYAPP);
	if (mSetOtherHotKeys)
	{
		::UnregisterHotKey(m_hWnd, IDM_MOVE_APP_LEFT);
		::UnregisterHotKey(m_hWnd, IDM_MOVE_APP_RIGHT);
		::UnregisterHotKey(m_hWnd, IDM_MOVE_APP_UP);
		::UnregisterHotKey(m_hWnd, IDM_MOVE_APP_DOWN);

		::UnregisterHotKey(m_hWnd, IDM_MINIMIZE_ACTIVE_APP);
		::UnregisterHotKey(m_hWnd, IDM_MAXIMIZE_ACTIVE_APP);
	}

	ClipboardChain(false);
	SendTrayNotify(NIM_DELETE, 0, nullptr);
	mAppSettings->SetValueInt(L"settings", L"autopaste", mAutopaste);
	mAppSettings->SetValueInt(L"settings", L"sendinput", mSendInput);
	mAppSettings->SetValueInt(L"settings", L"maxItems", mMaxHistoryCount);
	mAppSettings->SetValueInt(L"settings", L"windowMoveHotKeys", mSetOtherHotKeys);

	if (mMainWnd)
		mMainWnd->DestroyWindow();

	mClipHistory = nullptr;

	bHandled = FALSE;
	return 0;
}

LRESULT
TaskBarWnd::OnClearHistory(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if (mClipHistory)
		mClipHistory->Clear();
	if (mMainWnd)
		mMainWnd->UpdateData();
	bHandled = TRUE;
	return 1;
}

LRESULT
TaskBarWnd::OnDisplayMenu(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	POINT	pt;
	HMENU	hMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_TRAYMENU));
	HMENU	hPopMenu = ::GetSubMenu(hMenu, 0);
	
	::GetCursorPos(&pt);
	::SetForegroundWindow(m_hWnd);				// Q135788

	// add hotkey binding to menu item text
	CString itemTxt;
	itemTxt.Format(L"&Open Clipboard History\t%s", GetMainHotkey());
	MENUITEMINFO mii;
	ZeroMemory(&mii, sizeof(MENUITEMINFO));
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_STRING;
	mii.fType = MFT_STRING;
	mii.cch = itemTxt.GetLength();
	mii.dwTypeData = itemTxt.LockBuffer();
	::SetMenuItemInfo(hPopMenu, IDC_DISPLAYAPP, MF_BYCOMMAND, &mii);
	itemTxt.UnlockBuffer();

	::CheckMenuRadioItem(hPopMenu, IDC_ACTIVE, IDC_SUSPEND, (mAmInChain ? IDC_ACTIVE : IDC_SUSPEND), MF_BYCOMMAND);
	::SetMenuDefaultItem(hPopMenu, IDC_DISPLAYAPP, MF_BYCOMMAND);
	::CheckMenuItem(hPopMenu, IDM_AUTOPASTE, 
		(mAutopaste ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
	::CheckMenuItem(hPopMenu, IDM_SENDINPUT, 
		(mSendInput ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
	::CheckMenuItem(hPopMenu, ID_POPUP_ENABLEEXTRAHOTKEYS, 
		(mSetOtherHotKeys ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
	if (!mAutopaste)
		::EnableMenuItem(hPopMenu, IDM_SENDINPUT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

	::TrackPopupMenu(hPopMenu, TPM_LEFTALIGN|TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, nullptr);
	PostMessage(WM_NULL);		// Q135788
	::DestroyMenu(hMenu);
	::DestroyMenu(hPopMenu);

	bHandled = TRUE;
	return 1;
}

LRESULT
TaskBarWnd::OnEndSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (wParam)
	{
		mClipHistory->Save();
		mAppSettings->WriteFile();
	}
	bHandled = true;
	return 0;
}

LRESULT
TaskBarWnd::OnAutopaste(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	mAutopaste = mAutopaste ? 0 : 1;
	bHandled = true;
	return 1;
}

LRESULT
TaskBarWnd::OnSendinput(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	mSendInput = mSendInput ? 0 : 1;
	bHandled = true;
	return 1;
}

void
TaskBarWnd::ClipboardChain(bool add)
{
	if (add && !mAmInChain)
	{
		mNextWnd = SetClipboardViewer();
		mAmInChain = true;

		if (mUseTimer)
			mRechainTimerId = SetTimer(69, 5 * 60 * 1000);
	}
	else if (!add && mAmInChain)
	{
		mAmInChain = false;
		if (mNextWnd == m_hWnd)
		{
			FTRACE(L"ERR: TaskBarWnd::ClipboardChain nextWnd == this\n");
			ChangeClipboardChain(nullptr);
		}
		else
			ChangeClipboardChain(mNextWnd);
		mNextWnd = nullptr;

		if (mRechainTimerId)
		{
			KillTimer(mRechainTimerId);
			mRechainTimerId = 0;
		}
	}
}

LRESULT
TaskBarWnd::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (wParam == mRechainTimerId)
	{
		if (mAmInChain)
		{
//			FTRACE(L"Timer handled while chained\n");
			ClipboardChain(false);
			ClipboardChain(true);
		}
		else
		{
			FTRACE(L"Timer handled while not chained\n");
		}
		bHandled = true;
		return 0;
	}
	bHandled = false;
	return 1;
}

LRESULT 
TaskBarWnd::OnSuspend(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	ClipboardChain(false);
	SendTrayNotify(NIM_MODIFY, IDI_MAINFRAME_DISABLE, WND_NAME L" (disabled)");
	bHandled = true;
	return 1;
}

LRESULT
TaskBarWnd::OnChangeCbChain(UINT uMsg, WPARAM hWndRemove, LPARAM hWndAfter, BOOL& bHandled)
{
	// If the next window is closing, repair the chain
	if (mNextWnd == (HWND)hWndRemove)
	{
		if ((HWND)hWndAfter == m_hWnd)
		{
			FTRACE(L"ERR: TaskBarWnd::OnChangeCbClipboard nextWnd == this\n");
			mNextWnd = nullptr;
		}
		else
			mNextWnd = (HWND)hWndAfter;
	}
	// Otherwise, pass the message to the next link
	else if (mNextWnd && mNextWnd != m_hWnd)
		::SendMessage(mNextWnd, WM_CHANGECBCHAIN, (WPARAM)hWndRemove, (LPARAM)hWndAfter);
	bHandled = true;
	return 1;
}

LRESULT
TaskBarWnd::OnIconNotification(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	switch (lParam)	{
	case WM_LBUTTONDOWN:
		OnDisplayApp(0, 0, nullptr, bHandled);
		break;
	case WM_RBUTTONUP:
		OnDisplayMenu(0, 0, nullptr, bHandled);
	 	break;
	}

	bHandled = true;
	return 1;
}

LRESULT
TaskBarWnd::OnTaskBarCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	SendTrayNotify(NIM_ADD, IDI_MAINFRAME_ENABLE, L"Clipboard Watcher");
	bHandled = TRUE;
	return 1;
}

LRESULT
TaskBarWnd::OnHotkey(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	switch (wParam)
	{
	case IDC_DISPLAYMENU:
		return OnDisplayMenu(0, 0, nullptr, bHandled);
	case IDC_DISPLAYAPP:
		return OnDisplayApp(-5, 0, nullptr, bHandled);
	case IDM_MINIMIZE_ACTIVE_APP:
		OnMinimizeActiveApp();
		break;
	case IDM_MAXIMIZE_ACTIVE_APP:
		OnMaximizeActiveApp();
		break;
	case IDM_MOVE_APP_LEFT:
		OnMoveActiveApp(dirLeft);
		break;
	case IDM_MOVE_APP_RIGHT:
		OnMoveActiveApp(dirRight);
		break;
	case IDM_MOVE_APP_UP:
		OnMoveActiveApp(dirUp);
		break;
	case IDM_MOVE_APP_DOWN:
		OnMoveActiveApp(dirDown);
		break;
	}
	bHandled = true;
	return 1;
}

BOOL
TaskBarWnd::SendTrayNotify(DWORD dwMessage, UINT uID, PTSTR pszTip)
{
	NOTIFYICONDATA	tnd;
	BOOL			ret;
	HICON			hIcon = uID ? (HICON)::LoadImage(_Module.GetModuleInstance(), 
		MAKEINTRESOURCE(uID), IMAGE_ICON, 16, 16, 0) : nullptr;

	tnd.cbSize				= sizeof(NOTIFYICONDATA);
	tnd.hWnd				= m_hWnd;
	tnd.uID					= 100;

	tnd.uFlags				= NIF_MESSAGE|(uID?NIF_ICON:0)|(pszTip?NIF_TIP:0);
	tnd.uCallbackMessage	= WM_MY_NOTIFYICON;
	tnd.hIcon				= hIcon;
	if (pszTip)
		::lstrcpyn(tnd.szTip, pszTip, sizeof(tnd.szTip));

	ret = ::Shell_NotifyIcon(dwMessage, &tnd);

	if (hIcon)
		::DestroyIcon(hIcon);

	return ret;
}

LRESULT
TaskBarWnd::OnToggleExtraHotKeys(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if (mSetOtherHotKeys)
	{
		::UnregisterHotKey(m_hWnd, IDM_MOVE_APP_LEFT);
		::UnregisterHotKey(m_hWnd, IDM_MOVE_APP_RIGHT);
		::UnregisterHotKey(m_hWnd, IDM_MOVE_APP_UP);
		::UnregisterHotKey(m_hWnd, IDM_MOVE_APP_DOWN);
	}
	else
	{
		// win+ctrl+shift+right does not work on Win7
		// alt+shift+arrow works but conflicts with block edit in Visual Studio
		// alt+ctrl+shift+arrow does not work over remote desktop
		::RegisterHotKey(m_hWnd, IDM_MOVE_APP_LEFT, MOD_ALT|MOD_CONTROL|MOD_SHIFT, VK_LEFT);
		::RegisterHotKey(m_hWnd, IDM_MOVE_APP_RIGHT, MOD_ALT|MOD_CONTROL|MOD_SHIFT, VK_RIGHT);
		::RegisterHotKey(m_hWnd, IDM_MOVE_APP_UP, MOD_ALT|MOD_CONTROL|MOD_SHIFT, VK_UP);
		::RegisterHotKey(m_hWnd, IDM_MOVE_APP_DOWN, MOD_ALT|MOD_CONTROL|MOD_SHIFT, VK_DOWN);
	}

	if (!::IsWindows7OrGreater())
	{
		if (mSetOtherHotKeys)
		{
			::UnregisterHotKey(m_hWnd, IDM_MINIMIZE_ACTIVE_APP);
			::UnregisterHotKey(m_hWnd, IDM_MAXIMIZE_ACTIVE_APP);
		}
		else
		{
			::RegisterHotKey(m_hWnd, IDM_MINIMIZE_ACTIVE_APP, MOD_WIN, VK_DOWN);
			::RegisterHotKey(m_hWnd, IDM_MAXIMIZE_ACTIVE_APP, MOD_WIN, VK_UP);
		}
	}

	mSetOtherHotKeys = !mSetOtherHotKeys;

	bHandled = true;
	return 1;
}

void
TaskBarWnd::OnMinimizeActiveApp()
{
	HWND fgWnd = ::GetForegroundWindow();
	if (!fgWnd)
		return;

	WINDOWPLACEMENT wp;
	::ZeroMemory(&wp, sizeof(WINDOWPLACEMENT));
	::GetWindowPlacement(fgWnd, &wp);
	// if currently maximized then restore, else miminize
	if (wp.showCmd == SW_MAXIMIZE)
		::ShowWindow(fgWnd, SW_RESTORE);
	else
		::ShowWindow(fgWnd, SW_MINIMIZE);
}

void
TaskBarWnd::OnMaximizeActiveApp()
{
	HWND fgWnd = ::GetForegroundWindow();
	if (fgWnd)
		::ShowWindow(fgWnd, SW_MAXIMIZE);
}

void
TaskBarWnd::OnMoveActiveApp(Direction dir)
{
	HWND fgWnd = ::GetForegroundWindow();
	if (!fgWnd)
		return;

	WINDOWPLACEMENT wp;
	::ZeroMemory(&wp, sizeof(WINDOWPLACEMENT));
	::GetWindowPlacement(fgWnd, &wp);
	if (SW_MAXIMIZE == wp.showCmd || SW_MINIMIZE == wp.showCmd)
		return;

	if (dirLeft == dir || dirRight == dir)
	{
		wp.rcNormalPosition.left += (5 * (dirLeft == dir ? -1 : 1));
		wp.rcNormalPosition.right += (5 * (dirLeft == dir ? -1 : 1));
	}
	else if (dirUp == dir || dirDown == dir)
	{
		wp.rcNormalPosition.top += (5 * (dirUp == dir ? -1 : 1));
		wp.rcNormalPosition.bottom += (5 * (dirUp == dir ? -1 : 1));
	}
	else
	{
		_ASSERTE(!"OnMoveActiveApp - unhandled direction");
	}

	::SetWindowPlacement(fgWnd, &wp);
}

bool
TaskBarWnd::TranslateAccelerator(MSG* msg)
{
	if (mMainWnd)
		return mMainWnd->TranslateAccelerator(msg);

	return false;
}

void
TaskBarWnd::MainWndDestroyed()
{
	mMainWnd = nullptr;
}
