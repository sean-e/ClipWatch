#pragma once

#include "ATLControls.h"
#include <atlwin.h>

class SearchEditCtrl : public ATLControls::CEdit
// class SearchEditCtrl : public ATLControls::CEditT<CWindowImpl<SearchEditCtrl>> // would prefer this but message map doesn't seem to work, so need to subclass
{
public:
	SearchEditCtrl() { }
	~SearchEditCtrl() { }

	void ForwardReturn() { mForwardReturn = true; }

	void SetForwardingTarget(HWND wnd)
	{
		mForwardingTarget = wnd;
		ForwardReturn();

		// support for ctrl+backspace
		_ASSERTE(m_hWnd);
		::SHAutoComplete(m_hWnd, SHACF_AUTOAPPEND_FORCE_OFF | SHACF_AUTOSUGGEST_FORCE_OFF);

		// store this in GWL_USERDATA idx for retrieval in wnd proc -- not
		// recommended in general, but ok for this application
		SetWindowLong(GWL_USERDATA, (LONG)(void*)this);
		oldWndProc = (WNDPROC)::SetWindowLongPtrW(m_hWnd, GWLP_WNDPROC, (INT_PTR)DefWindowProc);
	}

	WNDPROC oldWndProc;

	BEGIN_MSG_MAP(SearchEditCtrl)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
	END_MSG_MAP()

protected:

	static LRESULT DefWindowProc(
		_In_ HWND hwnd,
		_In_ UINT uMsg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam)
	{
		SearchEditCtrl * _this = (SearchEditCtrl*) ::GetWindowLong(hwnd, GWL_USERDATA);
		if (_this)
			return _this->WindowProc(uMsg, wParam, lParam);
		return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	LRESULT WindowProc(
		_In_ UINT uMsg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam)
	{
		BOOL bHandled = FALSE;
		if (WM_KEYDOWN == uMsg)
			OnKeyDown(uMsg, wParam, lParam, bHandled);
		else if (WM_MOUSEWHEEL == uMsg)
			OnForwardMessage(uMsg, wParam, lParam, bHandled);
		else if (WM_NCDESTROY == uMsg)
			SetWindowLong(GWL_USERDATA, (LONG)nullptr);

		if (bHandled)
			return S_OK;

		return ::CallWindowProc(oldWndProc, m_hWnd, uMsg, wParam, lParam);
	}

	LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		switch (wParam) {
		case VK_UP:
		case VK_DOWN:
		case VK_NEXT:
		case VK_PRIOR:
			OnForwardMessage(uMsg, wParam, lParam, bHandled);
			break;
		case VK_RETURN:
			if (mForwardReturn)
				OnForwardMessage(uMsg, wParam, lParam, bHandled);
			break;
		case VK_LEFT:
		case VK_RIGHT:
			{
				CString curText;
				GetWindowText(curText);
				if (curText.IsEmpty())
				{
					// forward left and right to target if no text entered yet
					OnForwardMessage(uMsg, wParam, lParam, bHandled);
				}
			}
			break;
		case VK_HOME:
		case VK_END:
		case VK_SHIFT:
			{
				CString curText;
				GetWindowText(curText);
				if (curText.IsEmpty())
				{
					// forward home and end to target if no text entered yet
					OnForwardMessage(uMsg, wParam, lParam, bHandled);
				}
				else if (GetKeyState(VK_CONTROL) & 0x1000)
				{
					// forward ctrl+home and ctrl+end to target
					OnForwardMessage(uMsg, wParam, lParam, bHandled);
				}
			}
			break;
		}

		return S_OK;
	}

	LRESULT OnForwardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (mForwardingTarget)
		{
			::SendMessage(mForwardingTarget, uMsg, wParam, lParam);
			bHandled = TRUE;
		}

		return S_OK;
	}

private:
	HWND mForwardingTarget = nullptr;
	bool mForwardReturn = false;
};
