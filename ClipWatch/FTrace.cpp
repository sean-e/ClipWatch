/*
* ClipWatch clipboard extender/history/utility
* Copyright (C) 2001-2003, 2013-2014 Sean Echevarria
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

#include "StdAfx.h"
#include <windows.h>
#include <stdio.h>
#include "FTrace.h"

#if defined(ENABLE_FTRACE)

FTrace::FTrace(WCHAR msg[])
{
	DoTrace(msg);
}

FTrace::FTrace(const WCHAR * lpszFormat, ...)
{
	WCHAR buffer[1030];
	va_list argList;
	va_start(argList, lpszFormat);
	const int len = wvsprintf(buffer, lpszFormat, argList);
	va_end(argList);

	if (len > 1022)
		DoTrace(L"[truncation]\n");

	DoTrace(buffer);
}

void
FTrace::DoTrace(LPCWSTR msg)
{
	OutputDebugString(msg);
}

#endif // ENABLE_FTRACE
