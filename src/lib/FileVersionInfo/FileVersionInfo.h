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

#if !defined(AFX_FILEVERSION_H__F828004C_7680_40FE_A08D_7BB4FF05B4CC__INCLUDED_)
#define AFX_FILEVERSION_H__F828004C_7680_40FE_A08D_7BB4FF05B4CC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <winver.h>

#include <string>

class /*AFX_EXT_CLASS*/ CFileVersionInfo
{
// construction/destruction
public:
	CFileVersionInfo();
	virtual ~CFileVersionInfo();

// operations
public:
	BOOL Create(HMODULE hModule = NULL);
	BOOL Create(LPCSTR lpszFileName);

// attribute operations
public:
	WORD GetFileVersion(int nIndex) const;
	WORD GetProductVersion(int nIndex) const;
	DWORD GetFileFlagsMask() const;
	DWORD GetFileFlags() const;
	DWORD GetFileOs() const;
	DWORD GetFileType() const;
	DWORD GetFileSubtype() const;
	//CTime GetFileDate() const;

	std::string GetCompanyName() const;
	std::string GetFileDescription() const;
	std::string GetFileVersion() const;
	std::string GetInternalName() const;
	std::string GetLegalCopyright() const;
	std::string GetOriginalFileName() const;
	std::string GetProductName() const;
	std::string GetProductVersion() const;
	std::string GetComments() const;
	std::string GetLegalTrademarks() const;
	std::string GetPrivateBuild() const;
	std::string GetSpecialBuild() const;

// implementation helpers
protected:
	virtual void Reset();
	BOOL GetTranslationId(LPVOID lpData, UINT unBlockSize, WORD wLangId, DWORD &dwId, BOOL bPrimaryEnough = FALSE);

// attributes
private:
	VS_FIXEDFILEINFO m_FileInfo;

	std::string m_strCompanyName;
	std::string m_strFileDescription;
	std::string m_strFileVersion;
	std::string m_strInternalName;
	std::string m_strLegalCopyright;
	std::string m_strOriginalFileName;
	std::string m_strProductName;
	std::string m_strProductVersion;
	std::string m_strComments;
	std::string m_strLegalTrademarks;
	std::string m_strPrivateBuild;
	std::string m_strSpecialBuild;
};

#endif // !defined(AFX_FILEVERSION_H__F828004C_7680_40FE_A08D_7BB4FF05B4CC__INCLUDED_)

#endif // WIN32