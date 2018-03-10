// IniFile.h: interface for the CIniFile class.
// Written by: Adam Clauss
// Email: pandcc3@comwerx.net
// You may use this class/code as you wish in your programs.  Feel free to distribute it, and
// email suggested changes to me.
// http://www.codeproject.com/Articles/491/CIniFile
//
// Modified in 2001, 2013-2014, 2018 by Sean Echevarria
// http://www.creepingfog.com/sean/
// I think I started with the May 2000 version of the class.
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_INIFILE_H__D6BE0D97_13A8_11D4_A5D2_002078B03530__INCLUDED_)
#define AFX_INIFILE_H__D6BE0D97_13A8_11D4_A5D2_002078B03530__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <fstream>
#include <string>
#include <vector>

class CIniFile
{
	//all private variables
private:

	//stores pathname of ini file to read/write
	std::wstring path;

	//all keys are of this time
	struct key
	{
		//list of values in key
		std::vector<std::wstring> values;

		//corresponding list of value names
		std::vector<std::wstring> names;
	};

	//list of keys in ini
	std::vector<key> keys;

	//corresponding list of keynames
	std::vector<std::wstring> names;


	//all private functions
private:
	//overloaded to take string
	std::wifstream & getline(std::wifstream & is, std::wstring & str);

	//returns index of specified value, in the specified key, or -1 if not found
	int FindValue(int keynum, LPCWSTR valuename);

	//returns index of specified key, or -1 if not found
	int FindKey(LPCWSTR keyname);


	//public variables
public:

	//public functions
public:
	//default constructor
	CIniFile();

	//constructor, can specify pathname here instead of using SetPath later
	CIniFile(LPCWSTR inipath);
	CIniFile(std::wstring inipath);

	//default destructor
	virtual ~CIniFile();

	//sets path of ini file to read and write from
	void SetPath(LPCWSTR newpath);
	void SetPath(std::wstring newpath);

	//reads ini file specified using CIniFile::SetPath()
	//returns true if successful, false otherwise
	bool ReadFile();

	//writes data stored in class to ini file
	void WriteFile();

	//deletes all stored ini data
	void Reset();

	//returns number of keys currently in the ini
	int GetNumKeys();

	//returns number of values stored for specified key
	int GetNumValues(LPCWSTR keyname);

	//gets value of [keyname] valuename = 
	//overloaded to return LPCWSTR, int, and double,
	//returns "", or 0 if key/value not found.  Sets error member to show problem
	LPCWSTR GetValue(LPCWSTR keyname, LPCWSTR valuename, LPCWSTR defValue = nullptr);
	int GetValueInt(LPCWSTR keyname, LPCWSTR valuename, int defValue = 0);
	double GetValueFloat(LPCWSTR keyname, LPCWSTR valuename, double defValue = 0.0);

	//sets value of [keyname] valuename =.
	//specify the optional paramter as false (0) if you do not want it to create
	//the key if it doesn't exist. Returns true if data entered, false otherwise
	//overloaded to accept LPCWSTR, int, and double
	bool SetValue(LPCWSTR key, LPCWSTR valuename, LPCWSTR value, bool create = true);
	bool SetValueInt(LPCWSTR key, LPCWSTR valuename, int value, bool create = true);
	bool SetValueFloat(LPCWSTR key, LPCWSTR valuename, double value, bool create = true);

	//deletes specified value
	//returns true if value existed and deleted, false otherwise
	bool DeleteValue(LPCWSTR keyname, LPCWSTR valuename);

	//deletes specified key and all values contained within
	//returns true if key existed and deleted, false otherwise
	bool DeleteKey(LPCWSTR keyname);
};

#endif // !defined(AFX_INIFILE_H__D6BE0D97_13A8_11D4_A5D2_002078B03530__INCLUDED_)
