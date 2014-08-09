/*
* ClipWatch clipboard extender/history/utility
* Copyright (C) 2001-2004, 2013-2014 Sean Echevarria
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

// ClipWatch.cpp : Implementation of WinMain

#include "stdafx.h"
#include <shlobj.h>
#include <sys/stat.h>
#include "resource.h"
#include <initguid.h>
#include "ClipWatch.h"
#include "TaskBarWnd.h"
#include "CommCtrl.h"

#pragma comment(lib, "comctl32.lib")

/*
[clipwatch todo]
list should have mouse over tooltips to display item text
add options dlg to clipwatch
	optional mouse follow
		if disabled save position at close
exclude app list
investigate alternative to clipboard chain
	http://msdn.microsoft.com/en-us/library/ms649033%28v=VS.85%29.aspx
	http://msdn.microsoft.com/en-us/library/ms649021%28v=vs.85%29.aspx
	http://blogs.msdn.com/b/oldnewthing/archive/2011/09/19/10213224.aspx
	http://blogs.msdn.com/b/oldnewthing/archive/2011/09/19/10213224.aspx#10215774
*/

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

CString
GetAppDataDir()
{
	static CString appFolder;
	if (appFolder.IsEmpty())
	{
		WCHAR path[MAX_PATH+1] = L"";
		if (!::SHGetSpecialFolderPath(::GetDesktopWindow(), path, CSIDL_APPDATA, 0))
		{
		}

		appFolder = path;
		appFolder += L"\\ClipWatch";
		::CreateDirectory(appFolder, nullptr);
	}

	return appFolder;
}

CString
GetIniFilePath()
{
	CString iniPath;
	iniPath = GetAppDataDir();
	iniPath += L"\\cliphist.ini";
	return iniPath;
}

void
RunApp()
{
	AppSettings appSettings = std::make_shared<CIniFile>();
	appSettings->SetPath(GetIniFilePath());
	appSettings->ReadFile();
	TaskBarWnd taskBarWnd(appSettings);

	MSG msg;
	BOOL bRet;
	while ((bRet = GetMessage(&msg, nullptr, 0, 0)) != 0)
	{
		if (bRet == -1) 
		{
		}
		else
		{ 
			if (!taskBarWnd.TranslateAccelerator(&msg))
			{
				TranslateMessage(&msg); 
				DispatchMessage(&msg); 
			} 
		} 
	}

	appSettings->WriteFile();
}


extern "C" int WINAPI 
wWinMain(HINSTANCE hInstance,
		 HINSTANCE /*hPrevInstance*/,
		 LPWSTR lpCmdLine,
		 int /*nShowCmd*/)
{
//	lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT

    _Module.Init(ObjectMap, hInstance);
    int nRet = 0;

	::InitCommonControls();

	HWND hPrevInst = FindWindow(WND_CLASS_NAME, WND_NAME);
	if (hPrevInst)
		SendMessage(hPrevInst, WM_COMMAND, ID_APP_EXIT, 0);

	RunApp();

	_Module.Term();
    return nRet;
}
