/*
* ClipWatch clipboard extender/history/utility
* Copyright (C) 2001-2004, 2009, 2013 Sean Echevarria
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

// TaskBarWnd.h : Declaration of the TaskBarWnd

#ifndef __TASKBARWND_H_
#define __TASKBARWND_H_

#include "ClipWatchFrame.h"
#include "ClipboardHistory.h"
#include "resource.h"

extern const UINT WM_MY_NOTIFYICON;
extern UINT WM_TASKBARCREATED;

#define WND_CLASS_NAME		TEXT("Fester_ClipboardMonitorClass")
#define WND_NAME			TEXT("Clipboard Monitor")

/////////////////////////////////////////////////////////////////////////////
// TaskBarWnd

class TaskBarWnd : 
	public CWindowImpl<TaskBarWnd, CWindow, CWinTraits<0, WS_EX_APPWINDOW | WS_EX_TOOLWINDOW> >
{
	HWND	mNextWnd;
	BOOL	mAmInChain;
	int		mAutopaste;
	int		mSendInput;
	int		mMaxHistoryCount;
	int		mUseTimer;
	UINT	mRechainTimerId;
	BOOL	mSetOtherHotKeys;
	AppSettings mAppSettings;
	ClipWatchFrame	*mMainWnd;
	CString	mClipHistHotkey;
	std::shared_ptr<ClipboardHistory> mClipHistory;

public:
	TaskBarWnd(AppSettings settings);
	~TaskBarWnd();

	void DisplayWindow();
	BOOL GetMonitorActive() const { return mAmInChain; }
	int GetAutopaste() const { return mAutopaste; }
	int GetSendinput() const { return mSendInput; }
	int GetExtraHotkeysEnabled() const { return mSetOtherHotKeys; }
	void SetAutopaste(int val) { mAutopaste = val; }
	void MainWndDestroyed();
	CString GetMainHotkey() const { return mClipHistHotkey; }

	bool TranslateAccelerator(MSG* msg);
	LRESULT OnToggleExtraHotKeys(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	DECLARE_WND_CLASS_EX(WND_CLASS_NAME, 0, 0)

protected:
	BEGIN_MSG_MAP(TaskBarWnd)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_ENDSESSION, OnEndSession)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(IDC_ACTIVE, OnActivate)
		COMMAND_ID_HANDLER(IDC_SUSPEND, OnSuspend)
		COMMAND_ID_HANDLER(IDC_DISPLAYAPP, OnDisplayApp)
		COMMAND_ID_HANDLER(IDC_DISPLAYMENU, OnDisplayMenu)
		COMMAND_ID_HANDLER(IDC_CLEARHISTORY, OnClearHistory)
		COMMAND_ID_HANDLER(IDM_AUTOPASTE, OnAutopaste)
		COMMAND_ID_HANDLER(IDM_SENDINPUT, OnSendinput)
		COMMAND_ID_HANDLER(ID_POPUP_ENABLEEXTRAHOTKEYS, OnToggleExtraHotKeys)
		MESSAGE_HANDLER(WM_DRAWCLIPBOARD, OnDrawClipboard)
		MESSAGE_HANDLER(WM_CHANGECBCHAIN, OnChangeCbChain)
		MESSAGE_HANDLER(WM_MY_NOTIFYICON, OnIconNotification)
		MESSAGE_HANDLER(WM_TASKBARCREATED, OnTaskBarCreate)
		MESSAGE_HANDLER(WM_HOTKEY, OnHotkey)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
	END_MSG_MAP()

private:
	void OnFinalMessage(HWND /*hWnd*/)
	{
		::PostQuitMessage(0);
	}

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnFileExit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnActivate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnSuspend(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnChangeCbChain(UINT uMsg, WPARAM hWndRemove, LPARAM hWndAfter, BOOL& bHandled);
	LRESULT OnIconNotification(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnTaskBarCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnHotkey(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnEndSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDisplayApp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnDrawClipboard(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnClearHistory(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnDisplayMenu(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnAutopaste(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnSendinput(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	void OnMinimizeActiveApp();
	void OnMaximizeActiveApp();
	enum Direction { dirLeft, dirRight, dirUp, dirDown };
	void OnMoveActiveApp(Direction dir);

	BOOL SendTrayNotify(DWORD dwMessage, UINT uID, PTSTR pszTip);
	void ClipboardChain(bool add);
};

#endif //__TASKBARWND_H_
