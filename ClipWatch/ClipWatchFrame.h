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

// ClipWatchFrame.h : Declaration of the ClipWatchFrame

#ifndef __CLIPWATCHFRAME_H_
#define __CLIPWATCHFRAME_H_

#include "resource.h"       // main symbols
#include "ATLControls.h"
#include "SearchEditCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// ClipWatchFrame

class TaskBarWnd;
class ClipboardHistory;

enum 
{ 
	kClassNameLength = 256
};

#define CLIP_DISPLAY_WND_CLASS_NAME		TEXT("Fester_ClipboardDisplayFrameClass")
#define CLIP_DISPLAY_WND_TITLE			TEXT("Clipboard Monitor")

class ClipWatchFrame : 
	public CWindowImpl<ClipWatchFrame, CWindow, CWinTraits<WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE> >
{
	AppSettings	mSettings;
	ATLControls::CListViewCtrl mListview;
	SearchEditCtrl mEditCtrl;
	HMENU		mOptionsMenu;
	HMENU		mMainMenu;
	HACCEL		mAccelerators;
	HWND		mTargetWnd, mForegroundWnd;
	BOOL		mAttemptAutoPaste;
	WCHAR		mTargetWndClassName[kClassNameLength];
	TaskBarWnd	*mTaskWnd;
	std::shared_ptr<ClipboardHistory> mClipHist;
	using ClipItemIndex = std::vector<int>;
	ClipItemIndex	mClipIndexes;
	CString		mFilterText;
	UINT		mFilterTimerId = 0;
	bool		mInSearchMode = false;
	bool		mCaseSensitive = false;
	bool		mPendingFilter = false;

public:
	ClipWatchFrame(TaskBarWnd *tbWnd, AppSettings settings, std::shared_ptr<ClipboardHistory> clipHistory, BOOL show = TRUE);
	~ClipWatchFrame();

	DECLARE_WND_CLASS_EX(CLIP_DISPLAY_WND_CLASS_NAME, 0, 0)

	void UpdateData(CString filter = CString());
	void RedisplayWindow();
	bool TranslateAccelerator(MSG* msg);

protected:
	BEGIN_MSG_MAP(ClipWatchFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(ID_FILE_CLOSEWINDOW, OnWindowClose)
		COMMAND_ID_HANDLER(ID_FILE_SAVE, OnFileSave)
		COMMAND_ID_HANDLER(ID_FILE_EXITAPPLICATION, OnAppExit)
		COMMAND_ID_HANDLER(IDC_ACTIVE, OnActivate)
		COMMAND_ID_HANDLER(IDC_SUSPEND, OnSuspend)
		COMMAND_ID_HANDLER(IDC_CLEARHISTORY, OnClearHistory)
		COMMAND_ID_HANDLER(IDM_AUTOPASTE, OnAutopaste)
		COMMAND_ID_HANDLER(IDM_SENDINPUT, OnSendinput)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, OnCopyItemToClipboard)
		COMMAND_ID_HANDLER(ID_EDIT_PASTE, OnPasteItemExternal)
		COMMAND_ID_HANDLER(ID_EDIT_SELECT_ALL, OnSelectAll)
		COMMAND_ID_HANDLER(ID_EDIT_DELETE, OnDeleteItem)
		COMMAND_ID_HANDLER(ID_NEXT_PANE, OnControlNavigate)
		COMMAND_ID_HANDLER(ID_EDIT_PIN, OnPinItem)
		COMMAND_ID_HANDLER(ID_EDIT_SEARCH, OnToggleSearch)
		COMMAND_ID_HANDLER(ID_EDIT_EXECUTE, OnExecuteItem)
		COMMAND_ID_HANDLER(ID_POPUP_ENABLEEXTRAHOTKEYS, OnToggleHotKeys)
		COMMAND_ID_HANDLER(ID_HELP, OnHelp)
		COMMAND_ID_HANDLER(ID_ABOUT, OnAbout)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_INITMENU, OnInitMenu)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		NOTIFY_HANDLER(IDC_LISTVIEW, NM_CLICK, OnListViewSelect)
		NOTIFY_HANDLER(IDC_LISTVIEW, NM_RCLICK, OnListViewRightClick)
		NOTIFY_HANDLER(IDC_LISTVIEW, LVN_KEYDOWN, OnListViewKeyDown)
		COMMAND_HANDLER(IDC_FILTER_EDIT, EN_CHANGE, OnFilterEditTextChange)
	END_MSG_MAP()

private:
	void OnFinalMessage(HWND /*hWnd*/);
	LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnWindowClose(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnListViewSelect(int /*idCtrl*/, LPNMHDR /*notifyCode*/, BOOL& bHandled);
	LRESULT OnSize(UINT /* uMsg */, WPARAM /* wParam */, LPARAM /* lParam */, BOOL& lResult);
	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnInitMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnAppExit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnListViewKeyDown(int idCtrl, LPNMHDR notifyCode, BOOL& bHandled);
	LRESULT OnActivate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnSuspend(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnClearHistory(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnFileSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnListViewRightClick(int /*idCtrl*/, LPNMHDR /*notifyCode*/, BOOL& bHandled);
	LRESULT OnAutopaste(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnSendinput(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCopyItemToClipboard(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnPasteItemExternal(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnSelectAll(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnDeleteItem(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnControlNavigate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnPinItem(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnToggleSearch(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnExecuteItem(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnToggleHotKeys(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnFilterEditTextChange(int code, int idCtrl, HWND hwndCtl, BOOL& bHandled);
	LRESULT OnHelp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnAbout(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	bool SelectListViewItem(bool doPaste = true);
	void SaveDimensions();
	bool DoAutopaste();
	void SetTargetWindow();
	int GetClipItemHistIndex(int uiItemIdx) const;
	void SetFilterText(const CString &filterTxt);
	CString GetTextFromFilterEdit();
	bool ShouldDisplayItem(CString item) const;
	bool IsTargetWndListed(LPCWSTR iniSectionName, LPCWSTR iniItemName, LPCWSTR defaultVal = nullptr) const;
	bool IsTargetWndExcluded(LPCWSTR iniItemName, LPCWSTR defaultVal = nullptr) const;
	bool IsTargetWndIncluded(LPCWSTR iniItemName, LPCWSTR defaultVal = nullptr) const;
	bool TargetWndSupportsPasteMsg() const;
	bool TargetWndSupportsStandardPasteAccel() const;
	bool TargetWndSupportsPasteCmd() const;
	int ExecuteItem(int itemIdx);
};

#endif //__CLIPWATCHFRAME_H_
