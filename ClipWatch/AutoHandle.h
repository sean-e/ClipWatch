/*
* ClipWatch clipboard extender/history/utility
* Copyright (C) 2013 Sean Echevarria
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

#pragma once

#include <windows.h>


class AutoHandle
{
public:
	AutoHandle() : mHandle(nullptr)
	{ }

	AutoHandle(const HANDLE &rhs) : mHandle(rhs)
	{ }

	~AutoHandle()
	{
		if (mHandle && mHandle != INVALID_HANDLE_VALUE)
			::CloseHandle(mHandle);
	}

	AutoHandle& operator=(const HANDLE &rhs)
	{
		mHandle = rhs;
		return *this;
	}

	bool operator!() const
	{
		return !mHandle || mHandle == INVALID_HANDLE_VALUE;
	}

	operator HANDLE() const { return mHandle; }

	bool operator==(const HANDLE &rhs) const
	{
		return mHandle == rhs;
	}

	bool operator!=(const HANDLE &rhs) const
	{
		return !(*this == rhs);
	}

private:
	HANDLE mHandle;
};
