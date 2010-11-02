/********************************************************************
*
* Copyright (C) 1999-2000 Sven Wiegand
* Copyright (C) 2000-2001 ToolsCenter
* 
* This file is free software; you can redistribute it and/or
* modify, but leave the headers intact and do not remove any 
* copyrights from the source.
*
* If you have further questions, suggestions or bug fixes, visit 
* our homepage
*
*    http://www.ToolsCenter.org
*
********************************************************************/

#ifdef WIN32

// This file contains legacy code that doesn't work with Unicode.
#undef UNICODE
#undef _UNICODE

#include <windows.h>

#include "FileVersionInfo.h"

/*
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
*/

using namespace std;

//-------------------------------------------------------------------
// CFileVersionInfo
//-------------------------------------------------------------------

CFileVersionInfo::CFileVersionInfo()
{
	Reset();
}


CFileVersionInfo::~CFileVersionInfo()
{}


BOOL CFileVersionInfo::GetTranslationId(LPVOID lpData, UINT unBlockSize, WORD wLangId, DWORD &dwId, BOOL bPrimaryEnough/*= FALSE*/)
{
    LPWORD lpwData;
	for (lpwData = (LPWORD)lpData; (LPBYTE)lpwData < ((LPBYTE)lpData)+unBlockSize; lpwData+=2)
	{
		if (*lpwData == wLangId)
		{
			dwId = *((DWORD*)lpwData);
			return TRUE;
		}
	}

	if (!bPrimaryEnough)
		return FALSE;

	for (lpwData = (LPWORD)lpData; (LPBYTE)lpwData < ((LPBYTE)lpData)+unBlockSize; lpwData+=2)
	{
		if (((*lpwData)&0x00FF) == (wLangId&0x00FF))
		{
			dwId = *((DWORD*)lpwData);
			return TRUE;
		}
	}

	return FALSE;
}


BOOL CFileVersionInfo::Create(HMODULE hModule /*= NULL*/)
{
	TCHAR path[_MAX_PATH];

	GetModuleFileName(hModule, path, _MAX_PATH);

	return Create(path);
}


BOOL CFileVersionInfo::Create(LPCTSTR lpszFileName)
{
	Reset();

	DWORD	dwHandle;
	DWORD	dwFileVersionInfoSize = GetFileVersionInfoSize((LPTSTR)lpszFileName, &dwHandle);
	if (!dwFileVersionInfoSize)
		return FALSE;

	LPVOID	lpData = (LPVOID)new BYTE[dwFileVersionInfoSize];
	if (!lpData)
		return FALSE;

	try
	{
		if (!GetFileVersionInfo((LPTSTR)lpszFileName, dwHandle, dwFileVersionInfoSize, lpData))
			throw FALSE;

		// catch default information
		LPVOID	lpInfo;
		UINT		unInfoLen;
		if (VerQueryValue(lpData, ("\\"), &lpInfo, &unInfoLen))
		{
			if (unInfoLen == sizeof(m_FileInfo))
				memcpy(&m_FileInfo, lpInfo, unInfoLen);
		}

		// find best matching language and codepage
		VerQueryValue(lpData, ("\\VarFileInfo\\Translation"), &lpInfo, &unInfoLen);
		
		DWORD	dwLangCode = 0;
		if (!GetTranslationId(lpInfo, unInfoLen, GetUserDefaultLangID(), dwLangCode, FALSE))
		{
			if (!GetTranslationId(lpInfo, unInfoLen, GetUserDefaultLangID(), dwLangCode, TRUE))
			{
				if (!GetTranslationId(lpInfo, unInfoLen, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), dwLangCode, TRUE))
				{
					if (!GetTranslationId(lpInfo, unInfoLen, MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL), dwLangCode, TRUE))
						// use the first one we can get
						dwLangCode = *((DWORD*)lpInfo);
				}
			}
		}
		
		//strSubBlock.Format(_T("\\StringFileInfo\\%04X%04X\\"), dwLangCode&0x0000FFFF, (dwLangCode&0xFFFF0000)>>16);
		TCHAR temp[100];
		sprintf(temp, ("\\StringFileInfo\\%04X%04X\\"), dwLangCode&0x0000FFFF, (dwLangCode&0xFFFF0000)>>16);
		string strSubBlock(temp);
		
		// catch string table
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)((strSubBlock+"CompanyName").c_str()), &lpInfo, &unInfoLen))
			m_strCompanyName = (LPCTSTR)lpInfo;
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)((strSubBlock+("FileDescription")).c_str()), &lpInfo, &unInfoLen))
			m_strFileDescription = (LPCTSTR)lpInfo;
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)((strSubBlock+("FileVersion")).c_str()), &lpInfo, &unInfoLen))
			m_strFileVersion = (LPCTSTR)lpInfo;
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)((strSubBlock+("InternalName")).c_str()), &lpInfo, &unInfoLen))
			m_strInternalName = (LPCTSTR)lpInfo;
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)((strSubBlock+("LegalCopyright")).c_str()), &lpInfo, &unInfoLen))
			m_strLegalCopyright = (LPCTSTR)lpInfo;
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)((strSubBlock+("OriginalFileName")).c_str()), &lpInfo, &unInfoLen))
			m_strOriginalFileName = ((LPCTSTR)lpInfo);
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)((strSubBlock+("ProductName")).c_str()), &lpInfo, &unInfoLen))
			m_strProductName = ((LPCTSTR)lpInfo);
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)((strSubBlock+("ProductVersion")).c_str()), &lpInfo, &unInfoLen))
			m_strProductVersion = ((LPCTSTR)lpInfo);
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)((strSubBlock+("Comments")).c_str()), &lpInfo, &unInfoLen))
			m_strComments = ((LPCTSTR)lpInfo);
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)((strSubBlock+("LegalTrademarks")).c_str()), &lpInfo, &unInfoLen))
			m_strLegalTrademarks = ((LPCTSTR)lpInfo);
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)((strSubBlock+("PrivateBuild")).c_str()), &lpInfo, &unInfoLen))
			m_strPrivateBuild = ((LPCTSTR)lpInfo);
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)((strSubBlock+("SpecialBuild")).c_str()), &lpInfo, &unInfoLen))
			m_strSpecialBuild = ((LPCTSTR)lpInfo);

		delete[] lpData;
	}
	catch (BOOL)
	{
		delete[] lpData;
		return FALSE;
	}

	return TRUE;
}


WORD CFileVersionInfo::GetFileVersion(int nIndex) const
{
	if (nIndex == 0)
		return (WORD)(m_FileInfo.dwFileVersionLS & 0x0000FFFF);
	else if (nIndex == 1)
		return (WORD)((m_FileInfo.dwFileVersionLS & 0xFFFF0000) >> 16);
	else if (nIndex == 2)
		return (WORD)(m_FileInfo.dwFileVersionMS & 0x0000FFFF);
	else if (nIndex == 3)
		return (WORD)((m_FileInfo.dwFileVersionMS & 0xFFFF0000) >> 16);
	else
		return 0;
}


WORD CFileVersionInfo::GetProductVersion(int nIndex) const
{
	if (nIndex == 0)
		return (WORD)(m_FileInfo.dwProductVersionLS & 0x0000FFFF);
	else if (nIndex == 1)
		return (WORD)((m_FileInfo.dwProductVersionLS & 0xFFFF0000) >> 16);
	else if (nIndex == 2)
		return (WORD)(m_FileInfo.dwProductVersionMS & 0x0000FFFF);
	else if (nIndex == 3)
		return (WORD)((m_FileInfo.dwProductVersionMS & 0xFFFF0000) >> 16);
	else
		return 0;
}


DWORD CFileVersionInfo::GetFileFlagsMask() const
{
	return m_FileInfo.dwFileFlagsMask;
}


DWORD CFileVersionInfo::GetFileFlags() const
{
	return m_FileInfo.dwFileFlags;
}


DWORD CFileVersionInfo::GetFileOs() const
{
	return m_FileInfo.dwFileOS;
}


DWORD CFileVersionInfo::GetFileType() const
{
	return m_FileInfo.dwFileType;
}


DWORD CFileVersionInfo::GetFileSubtype() const
{
	return m_FileInfo.dwFileSubtype;
}


/*
CTime CFileVersionInfo::GetFileDate() const
{
	FILETIME	ft;
	ft.dwLowDateTime = m_FileInfo.dwFileDateLS;
	ft.dwHighDateTime = m_FileInfo.dwFileDateMS;
	return CTime(ft);
}
*/

string CFileVersionInfo::GetCompanyName() const
{
	return m_strCompanyName;
}


string CFileVersionInfo::GetFileDescription() const
{
	return m_strFileDescription;
}


string CFileVersionInfo::GetFileVersion() const
{
	return m_strFileVersion;
}


string CFileVersionInfo::GetInternalName() const
{
	return m_strInternalName;
}


string CFileVersionInfo::GetLegalCopyright() const
{
	return m_strLegalCopyright;
}


string CFileVersionInfo::GetOriginalFileName() const
{
	return m_strOriginalFileName;
}


string CFileVersionInfo::GetProductName() const
{
	return m_strProductName;
}


string CFileVersionInfo::GetProductVersion() const
{
	return m_strProductVersion;
}


string CFileVersionInfo::GetComments() const
{
	return m_strComments;
}


string CFileVersionInfo::GetLegalTrademarks() const
{
	return m_strLegalTrademarks;
}


string CFileVersionInfo::GetPrivateBuild() const
{
	return m_strPrivateBuild;
}


string CFileVersionInfo::GetSpecialBuild() const
{
	return m_strSpecialBuild;
}


void CFileVersionInfo::Reset()
{
	ZeroMemory(&m_FileInfo, sizeof(m_FileInfo));
	m_strCompanyName.clear();
	m_strFileDescription.clear();
	m_strFileVersion.clear();
	m_strInternalName.clear();
	m_strLegalCopyright.clear();
	m_strOriginalFileName.clear();
	m_strProductName.clear();
	m_strProductVersion.clear();
	m_strComments.clear();
	m_strLegalTrademarks.clear();
	m_strPrivateBuild.clear();
	m_strSpecialBuild.clear();
}


#endif // WIN32
