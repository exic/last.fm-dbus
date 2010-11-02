/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "componentinfo.h"
#include "logger.h"
#include "versionnumber.h"
#include "LastFmSettings.h"

#ifdef WIN32
    #include <windows.h>
    #include "FileVersionInfo/FileVersionInfo.h"
    #include "KillProcess/KillProcess.h"
#endif

#include <sys/stat.h>
#include <cctype>
#include <algorithm> // for transform

using namespace std;

/******************************************************************************
    GetPathFilename
******************************************************************************/
QString
CComponentInfo::GetPathFilename()
{
/*
    string::size_type nIdx = msPath.find_last_of("/");
    Q_ASSERT(nIdx != string::npos);

    string sFile = msPath.substr(
        nIdx + 1, msPath.size() - (nIdx + 1));

    return sFile;
*/
    int nIdx = msPath.lastIndexOf("/");
    Q_ASSERT(nIdx != -1);

    QString sFile = msPath.mid( nIdx + 1, msPath.size() - (nIdx + 1) );

    return sFile;
}

/******************************************************************************
    GetPathFileExt
******************************************************************************/
QString
CComponentInfo::GetPathFileExt()
{
/*
    string::size_type nIdx = msPath.find_last_of(".");
    Q_ASSERT(nIdx != string::npos);

    string sExt = msPath.substr(
        nIdx + 1, msPath.size() - (nIdx + 1));

    transform(sExt.begin(), sExt.end(), sExt.begin(), (int(*)(int))tolower);

    return sExt;
*/

    int nIdx = msPath.lastIndexOf(".");
    Q_ASSERT(nIdx != -1);

    QString sExt = msPath.mid( nIdx + 1, msPath.size() - (nIdx + 1) );

    return sExt.toLower();
}

/******************************************************************************
    GetURLFilename
******************************************************************************/
QString
CComponentInfo::GetURLFilename()
{
/*
    string::size_type nIdx = msDownloadURL.find_last_of("/");
    Q_ASSERT(nIdx != string::npos);

    string sFile = msDownloadURL.substr(
        nIdx + 1, msDownloadURL.size() - (nIdx + 1));

    return sFile;
*/

    int nIdx = msDownloadURL.lastIndexOf("/");
    Q_ASSERT(nIdx != -1);

    QString sFile = msDownloadURL.mid( nIdx + 1, msDownloadURL.size() - (nIdx + 1) );

    return sFile;
}

/******************************************************************************
    GetURLFileExt
******************************************************************************/
QString
CComponentInfo::GetURLFileExt()
{
/*
    string::size_type nIdx = msDownloadURL.find_last_of(".");
    Q_ASSERT(nIdx != string::npos);

    string sExt = msDownloadURL.substr(
        nIdx + 1, msDownloadURL.size() - (nIdx + 1));

    transform(sExt.begin(), sExt.end(), sExt.begin(), (int(*)(int))tolower);

    return sExt;
*/

    int nIdx = msDownloadURL.lastIndexOf(".");
    Q_ASSERT(nIdx != -1);

    QString sExt = msDownloadURL.mid( nIdx + 1, msDownloadURL.size() - (nIdx + 1) );

    return sExt.toLower();
}

/******************************************************************************
    GetInstallPath
******************************************************************************/
QString
CComponentInfo::GetInstallPath()
{
/*
    // Just remove the filename from the path
    string::size_type nIdx = msPath.find_last_of("/");
    Q_ASSERT(nIdx != string::npos);

    string sPath = msPath.substr(0, nIdx + 1);

    return sPath;
*/

    int nIdx = msPath.lastIndexOf("/");
    Q_ASSERT(nIdx != -1);

    QString sPath = msPath.mid( 0, nIdx + 1 );

    return sPath;

}

/******************************************************************************
    IsInstalled
******************************************************************************/
bool
CComponentInfo::IsInstalled()
{
    if (mInstalled == eUnknown)
    {
        mInstalled = ProgramExists(msPath) ? eTrue : eFalse;
    }
    return mInstalled == eTrue;
}

/******************************************************************************
    IsVersionNewer
******************************************************************************/
bool
CComponentInfo::IsVersionNewer()
{
    if (mVersionNewer == eUnknown)
    {
        QString sExeVer = GetExeVersion(msPath);

        CVersionNumber verExe(sExeVer.toStdString());
        CVersionNumber verLatest(msVersion.toStdString());

        mVersionNewer = verLatest > verExe ? eTrue : eFalse;
    }

    return mVersionNewer == eTrue;
}

/******************************************************************************
    IsRunning
******************************************************************************/
bool
CComponentInfo::IsRunning()
{
#ifdef WIN32

    CKillProcessHelper killer;
    DWORD dummy;
    return (killer.FindProcess(qPrintable(GetPathFilename()), dummy) != NULL);

#else // !WIN32

    Q_ASSERT_X(false, "CComponentInfo::IsRunning",
        "TODO: not yet implemented for non-Win platforms");

    return false;

#endif // WIN32
}

/******************************************************************************
    IsApp
******************************************************************************/
bool
CComponentInfo::IsApp()
{
    // This fucker is the result of this class hierarchy breaking the
    // Liskov Substitution Principle. Extremely ugly hack of an
    // implementation it is too.
    return (GetName() == "Last.fm");
}

/******************************************************************************
    Clear
******************************************************************************/
void
CComponentInfo::Clear()
{
    msName = "";
    msPath = "";
    msDownloadURL = "";
    msVersion = "";
    msInstallArgs = "";
    mInstalled = eUnknown;
    mVersionNewer = eUnknown;
    mSize = 0;
    mMajorUpgrade = false;
    mDescription = "";
    mImage = QUrl();
}

/******************************************************************************
    KillProcess
******************************************************************************/
bool
CComponentInfo::KillProcess()
{
#ifdef WIN32

    CKillProcessHelper killer;
    return killer.KillProcess(qPrintable(GetPathFilename()));

#else // !WIN32

    Q_ASSERT_X(false, "CComponentInfo::KillProcess",
        "TODO: not yet implemented for non-Win platforms");

    return false;

#endif // WIN32
}

/******************************************************************************
    ProgramExists

    Simple check for default installation path.
******************************************************************************/
bool
CComponentInfo::ProgramExists(
    const QString& sPath)
{
    // _stat returns 0 if it was possible to find the file
    struct stat dummy;
    int nSuccess = stat(qPrintable(sPath), &dummy);

    return nSuccess == 0;
}

/******************************************************************************
    GetExeVersion
******************************************************************************/
QString
CComponentInfo::GetExeVersion(
    const QString& sPath)
{
#ifdef WIN32

    CFileVersionInfo fvi;
    if (!fvi.Create(qPrintable(sPath)))
    {
        return "";
    }

    return QString::fromStdString(fvi.GetFileVersion());

#else // !WIN32

    Q_UNUSED( sPath )

    LOG(2, "GetExeVersion called on non-Windows, returning empty string\n");
    return "";

#endif // WIN32
}

/******************************************************************************
    GetRegVersion

    Subkey should be of the form "wmp" e.g.
******************************************************************************/
QString
CComponentInfo::GetRegVersion( const QString& sSubKey )
{
    QString version = The::settings().pluginVersion(sSubKey);

    if (version == "")
        LOG(1, "Couldn't read version string for " << sSubKey << "\n");

    return version;
}
