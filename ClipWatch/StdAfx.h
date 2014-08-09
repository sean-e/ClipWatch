/*
* ClipWatch clipboard extender/history/utility
* Copyright (C) 2001-2003, 2009, 2013 Sean Echevarria
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

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__C8E117D7_4FA4_11D4_B57F_00C04F8CF986__INCLUDED_)
#define AFX_STDAFX_H__C8E117D7_4FA4_11D4_B57F_00C04F8CF986__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#define NTDDI_VERSION NTDDI_WINXP
#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WINXP
#define WINVER  0x0500      /* version 5.0 */
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module

extern CComModule _Module;

#include <atlcom.h>
#include <atlhost.h>
#include <atlwin.h>
#include <atlctl.h>
#include <afxres.h>
#include <atlstr.h>
#include <memory>

#include "IniFile.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

typedef std::shared_ptr<CIniFile> AppSettings;
extern CString GetAppDataDir();

#endif // !defined(AFX_STDAFX_H__C8E117D7_4FA4_11D4_B57F_00C04F8CF986__INCLUDED)
