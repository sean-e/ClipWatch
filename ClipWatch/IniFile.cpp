// IniFile.cpp: implementation of the CIniFile class.
// Written by: Adam Clauss
// http://www.codeproject.com/Articles/491/CIniFile
//
// Modified in 2001, 2009, 2013-2014, 2018 by Sean Echevarria
// http://www.creepingfog.com/sean/
// I think I started with the May 2000 version of the class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <winnt.h>
#include <sys/stat.h>
#include "IniFile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE [] = __FILE__;
#define new DEBUG_NEW
#endif

using std::wstring;

/////////////////////////////////////////////////////////////////////
// Construction/Destruction
/////////////////////////////////////////////////////////////////////

//default constructor
CIniFile::CIniFile()
{
}

//constructor, can specify pathname here instead of using SetPath later
CIniFile::CIniFile(LPCWSTR inipath)
{
	path = inipath;
}

CIniFile::CIniFile(wstring inipath)
{
	path = inipath;
}

//default destructor
CIniFile::~CIniFile()
{

}

/////////////////////////////////////////////////////////////////////
// Public Functions
/////////////////////////////////////////////////////////////////////

//sets path of ini file to read and write from
void CIniFile::SetPath(LPCWSTR newpath)
{
	path = newpath;
}

void CIniFile::SetPath(wstring newpath)
{
	path = newpath;
}

//reads ini file specified using CIniFile::SetPath()
//returns true if successful, false otherwise
bool CIniFile::ReadFile()
{
	std::wifstream inifile;
	try
	{
		inifile.open(path.c_str());
	}
	catch (const std::exception &)
	{
		return false;
	}

	if (inifile.fail())
		return false;

	wstring readinfo;
	wstring keyname, valuename, value;
	wstring temp;
	while (getline(inifile, readinfo))
	{
		if (!readinfo.empty())
		{
			int readLen = readinfo.length();
			if (readinfo[0] == L'[' && readinfo[readLen - 1] == L']') //if a section heading
			{
				keyname = readinfo.substr(1, readLen - 2);
			}
			else //if a value
			{
				int eq = readinfo.find(L"=");
				if (eq == wstring::npos)
				{
					// maybe a comment
					valuename = readinfo;
					value = L"";
				}
				else
				{
					valuename = readinfo.substr(0, eq);
					value = readinfo.substr(eq + 1, readLen - valuename.length() - 1);
				}
				SetValue(keyname.c_str(), valuename.c_str(), value.c_str());
			}
		}
	}
	inifile.close();
	return true;
}

//writes data stored in class to ini file
void CIniFile::WriteFile()
{
	std::wofstream inifile;
	inifile.open(path.c_str());
	for (unsigned int keynum = 0; keynum < names.size(); keynum++)
	{
		if (!keys[keynum].names.empty() && names[keynum].length())
		{
			inifile << L'[' << names[keynum] << L']' << std::endl;
			for (unsigned int valuenum = 0; valuenum < keys[keynum].names.size(); valuenum++)
			{
				if (keys[keynum].values[valuenum].length())
					inifile << keys[keynum].names[valuenum] << L"=" << keys[keynum].values[valuenum];
				else
					inifile << keys[keynum].names[valuenum];	// could be a comment
				if (valuenum != keys[keynum].names.size() - 1)
					inifile << std::endl;
				else if (keynum < names.size())
					inifile << std::endl;
			}
			if (keynum < names.size())
				inifile << std::endl;
		}
	}
	inifile.close();
}

//deletes all stored ini data
void CIniFile::Reset()
{
	keys.clear();
	names.clear();
}

//returns number of keys currently in the ini
int CIniFile::GetNumKeys()
{
	return keys.size();
}

//returns number of values stored for specified key, or -1 if key found
int CIniFile::GetNumValues(LPCWSTR keyname)
{
	int keynum = FindKey(keyname);
	if (keynum == -1)
		return -1;
	else
		return keys[keynum].names.size();
}

//gets value of [keyname] valuename = 
//overloaded to return LPCWSTR, int, and double
LPCWSTR CIniFile::GetValue(LPCWSTR keyname, LPCWSTR valuename, LPCWSTR defValue /* = NULL */)
{
	int keynum = FindKey(keyname), valuenum = FindValue(keynum, valuename);

	if (keynum == -1)
	{
		//		error = "Unable to locate specified key.";
		return defValue;
	}

	if (valuenum == -1)
	{
		//		error = "Unable to locate specified value.";
		return defValue;
	}

	LPCWSTR val = keys[keynum].values[valuenum].c_str();
	if (val && *val)
		return val;
	return defValue;
}

//gets value of [keyname] valuename = 
//overloaded to return LPCWSTR, int, and double
int CIniFile::GetValueInt(LPCWSTR keyname, LPCWSTR valuename, int defValue /* = 0 */)
{
	LPCWSTR val = GetValue(keyname, valuename);
	if (val && *val)
		return _wtoi(val);
	else
		return defValue;
}

//gets value of [keyname] valuename = 
//overloaded to return LPCWSTR, int, and double
double CIniFile::GetValueFloat(LPCWSTR keyname, LPCWSTR valuename, double defValue /* = 0 */)
{
	LPCWSTR val = GetValue(keyname, valuename);
	if (val && *val)
		return _wtof(val);
	else
		return defValue;
}

//sets value of [keyname] valuename =.
//specify the optional paramter as false (0) if you do not want it to create
//the key if it doesn't exist. Returns true if data entered, false otherwise
//overloaded to accept LPCWSTR, int, and double
bool CIniFile::SetValue(LPCWSTR keyname, LPCWSTR valuename, LPCWSTR value, bool create)
{
	int keynum = FindKey(keyname), valuenum = 0;
	//find key
	if (keynum == -1) //if key doesn't exist
	{
		if (!create) //and user does not want to create it,
			return false; //stop entering this key
		int nameLen = names.size();
		names.resize(nameLen + 1);
		keys.resize(keys.size() + 1);
		keynum = nameLen;
		names[keynum] = keyname;
	}

	//find value
	valuenum = FindValue(keynum, valuename);
	if (valuenum == -1)
	{
		if (!create)
			return false;
		int nameLen = keys[keynum].names.size();
		keys[keynum].names.resize(nameLen + 1);
		keys[keynum].values.resize(nameLen + 1);
		valuenum = nameLen;
		keys[keynum].names[valuenum] = valuename;
	}
	keys[keynum].values[valuenum] = value;
	return true;
}

//sets value of [keyname] valuename =.
//specify the optional paramter as false (0) if you do not want it to create
//the key if it doesn't exist. Returns true if data entered, false otherwise
//overloaded to accept LPCWSTR, int, and double
bool CIniFile::SetValueInt(LPCWSTR keyname, LPCWSTR valuename, int value, bool create)
{
	WCHAR temp[50];
	wsprintf(temp, L"%d", value);
	return SetValue(keyname, valuename, temp, create);
}

//sets value of [keyname] valuename =.
//specify the optional paramter as false (0) if you do not want it to create
//the key if it doesn't exist. Returns true if data entered, false otherwise
//overloaded to accept LPCWSTR, int, and double
bool CIniFile::SetValueFloat(LPCWSTR keyname, LPCWSTR valuename, double value, bool create)
{
	WCHAR temp[200];
	wsprintf(temp, L"%e", value);
	return SetValue(keyname, valuename, temp, create);
}

//deletes specified value
//returns true if value existed and deleted, false otherwise
bool CIniFile::DeleteValue(LPCWSTR keyname, LPCWSTR valuename)
{
	int keynum = FindKey(keyname), valuenum = FindValue(keynum, valuename);
	if (keynum == -1 || valuenum == -1)
		return false;

	// sean: substitutes for RemoveAt
	keys[keynum].names.at(valuenum) = L"";
	keys[keynum].values.at(valuenum) = L"";
	return true;
}

//deletes specified key and all values contained within
//returns true if key existed and deleted, false otherwise
bool CIniFile::DeleteKey(LPCWSTR keyname)
{
	int keynum = FindKey(keyname);
	if (keynum == -1)
		return false;

	// sean: substitutes for RemoveAt
	keys.at(keynum).values.clear();
	keys.at(keynum).names.clear();
	names.at(keynum) = L"";
	return true;
}

/////////////////////////////////////////////////////////////////////
// Private Functions
/////////////////////////////////////////////////////////////////////

//returns index of specified key, or -1 if not found
int CIniFile::FindKey(LPCWSTR keyname)
{
	int keynum = 0;
	while (keynum < (int) keys.size() && names[keynum] != keyname)
		keynum++;
	if (keynum == keys.size())
		return -1;
	return keynum;
}

//returns index of specified value, in the specified key, or -1 if not found
int CIniFile::FindValue(int keynum, LPCWSTR valuename)
{
	if (keynum == -1)
		return -1;
	int valuenum = 0;
	while (valuenum < (int) keys[keynum].names.size() && keys[keynum].names[valuenum] != valuename)
		valuenum++;
	if (valuenum == keys[keynum].names.size())
		return -1;
	return valuenum;
}

//overloaded from original getline to take string
std::wifstream & CIniFile::getline(std::wifstream & is, wstring & str)
{
	wchar_t buf[2048];
	is.getline(buf, 2048);
	str = buf;
	return is;
}
