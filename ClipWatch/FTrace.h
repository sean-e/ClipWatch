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

#ifndef FTrace_H__
#define FTrace_H__

#ifndef NDEBUG
#	ifndef ENABLE_FTRACE
#	define ENABLE_FTRACE
#	endif
#endif

#ifdef ENABLE_FTRACE
#include "TChar.h"

class FTrace
{
public:
	FTrace(WCHAR msg[]);
	FTrace(const WCHAR *msg, ...);

private:
	void DoTrace(LPCWSTR msg);
};

#define FTRACE	FTrace
#define FTRACE1	FTrace
#define FTRACE2	FTrace
#define FTRACE3	FTrace
#define FTRACE4	FTrace

#else

#define FTRACE(str)
#define FTRACE1(str, x)
#define FTRACE2(str, x, y)
#define FTRACE3(str, x, y, z)
#define FTRACE4(str, w, x, y, z)

#endif // !ENABLE_FTRACE
#endif // FTrace_H__
