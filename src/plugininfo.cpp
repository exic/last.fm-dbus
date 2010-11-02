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

#include "plugininfo.h"
#include "logger.h"
#include "LastFmSettings.h"
#include "versionnumber.h"
#include "UnicornCommon.h"

#ifdef WIN32
    #include "FileVersionInfo/FileVersionInfo.h"
    #include "KillProcess/KillProcess.h"
    #include <windows.h>
#endif

//#include <sstream>
//#include <algorithm> // for transform

using namespace std;

QString CPluginInfo::msDefaultPlayer = "-1";

/******************************************************************************
    GetPlayerDir
******************************************************************************/
QString
CPluginInfo::GetPlayerDir()
{
/*
    string::size_type nIdx = msPlayerPath.find_last_of("/");
    Q_ASSERT(nIdx != string::npos);

    string sDir = msPlayerPath.substr(0, nIdx + 1);

    return sDir;
*/

    int nIdx = msPlayerPath.lastIndexOf("/");
    if ( nIdx == -1 )
        nIdx = msPlayerPath.lastIndexOf("\\");
    
    Q_ASSERT(nIdx != -1);

    QString sDir = msPlayerPath.mid( 0, nIdx + 1 );

    return sDir;
}

/******************************************************************************
    GetPlayerFilename
******************************************************************************/
QString
CPluginInfo::GetPlayerFilename()
{
/*
    string::size_type nIdx = msPlayerPath.find_last_of("/");
    Q_ASSERT(nIdx != string::npos);

    string sFile = msPlayerPath.substr(nIdx + 1, msPlayerPath.size() - nIdx - 1);

    return sFile;
*/

    int nIdx = msPlayerPath.lastIndexOf("/");
    if ( nIdx == -1 )
        nIdx = msPlayerPath.lastIndexOf("\\");
    
    Q_ASSERT(nIdx != -1);

    QString sFile = msPlayerPath.mid( nIdx + 1, msPlayerPath.size() - nIdx - 1 );

    return sFile;
}

/******************************************************************************
    IsInstalled
******************************************************************************/
bool
CPluginInfo::IsInstalled()
{
    if (mInstalled == eUnknown)
    {
        mInstalled = IsPluginInstalled() ? eTrue : eFalse;
    }
    return mInstalled == eTrue;
}

/******************************************************************************
    IsVersionNewer
******************************************************************************/
bool
CPluginInfo::IsVersionNewer()
{
    if (mVersionNewer == eUnknown)
    {
        QString sSubKey = GetId();
        QString sInstalledVer = GetRegVersion(sSubKey);

        CVersionNumber verInstalled(sInstalledVer.toStdString());
        CVersionNumber verNew(GetVersion().toStdString());

        LOG(3, GetName() << " plugin installed ver: " << sInstalledVer <<
            ", server ver: " << GetVersion() << "\n");

        mVersionNewer = verNew > verInstalled ? eTrue : eFalse;
    }

    return mVersionNewer == eTrue;
}

/******************************************************************************
    IsPlayerInstalled
******************************************************************************/
bool
CPluginInfo::IsPlayerInstalled()
{
    if (mPlayerInstalled == eUnknown)
    {
        mPlayerInstalled = ProgramExists(msPlayerPath) && VersionMatch() ?
                           eTrue : eFalse;
    }
    return mPlayerInstalled == eTrue;
}

/******************************************************************************
    IsPlayerDefault
******************************************************************************/
bool
CPluginInfo::IsPlayerDefault()
{
    #ifdef WIN32
    if (msDefaultPlayer == "-1")
    {
        msDefaultPlayer = UnicornUtils::findDefaultPlayer().toLower();
    }
    #endif

    return GetPlayerFilename().toLower() == msDefaultPlayer;
}

/******************************************************************************
    IsRunning
******************************************************************************/
bool
CPluginInfo::IsRunning()
{
#ifdef WIN32

    CKillProcessHelper killer;
    DWORD dummy;
    return (killer.FindProcess(qPrintable(GetPlayerFilename()), dummy) != NULL);

#else // !WIN32

    Q_ASSERT_X(false, "CPluginInfo::IsRunning",
        "TODO: not yet implemented for non-Win platforms");

    return false;

#endif // WIN32
}

/******************************************************************************
    Clear
******************************************************************************/
void
CPluginInfo::Clear()
{
    CComponentInfo::Clear();

    msPlayerName = "";
    msPlayerPath = "";
    msInstallDir = "";
    msPlayerVerMin = "";
    msPlayerVerMax = "";
    
    mPlayerInstalled = eUnknown;
    mPlayerDefault = eUnknown;
}

/******************************************************************************
    KillProcess
******************************************************************************/
bool
CPluginInfo::KillProcess()
{
#ifdef WIN32

    CKillProcessHelper killer;
    return killer.KillProcess(qPrintable(GetPlayerFilename()));

#else // !WIN32

    Q_ASSERT_X(false, "CPluginInfo::KillProcess",
        "TODO: not yet implemented for non-Win platforms");
    
    return false;

#endif // WIN32
}

/******************************************************************************
    ExecuteProcess
******************************************************************************/
bool
CPluginInfo::ExecuteProcess()
{
#ifdef WIN32

    return QProcess::startDetached( QString( "\"" ) + msPlayerPath + "\"" );

#else // !WIN32

    Q_ASSERT_X(false, "CPluginInfo::KillProcess",
        "TODO: not yet implemented for non-Win platforms");

    return false;

#endif // WIN32
}


/******************************************************************************
    IsPluginInstalled
******************************************************************************/
bool
CPluginInfo::IsPluginInstalled()
{
	#ifdef Q_WS_MAC
		if( GetId() == "itm" )
			return true;
	#endif
	
    bool exists = The::settings().pluginVersion( GetId() ) != "";

    if (exists)
    {
        LOG(3, GetId() << " was found. Plugin present.\n");
        return true;
    }
    else
    {
        LOG(3, GetId() << " wasn't found. Plugin not present.\n");
        return false;
    }
}

/******************************************************************************
    VersionMatch
******************************************************************************/
bool
CPluginInfo::VersionMatch()
{
    // Let's try and read player exe version
    QString sExeVer = GetExeVersion(msPlayerPath);

    if (sExeVer == "")
    {
        // We couldn't read the version from the exe
        LOG(1, "Couldn't read version number for " << msPlayerName << "\n");

        // So... if the plugin info has no version restrictions, we let it through.
        // If it has, we will have to assume it doesn't match as installing the
        // wrong plugin could fuck things up.
        bool bVersionRestricted = msPlayerVerMin != "" || msPlayerVerMax != "";

        LOG(1, "Version restricted: " << (bVersionRestricted ? "yes" : "no") <<
            ", assuming match: " << (!bVersionRestricted ? "yes" : "no") << "\n");

        return !bVersionRestricted;
    }

    // Got some info
    LOG(3, "Player version to test: " << msPlayerVerMin << " - " << msPlayerVerMax << "\n");
    LOG(3, "Version of " << msPlayerName << ": " << sExeVer << "\n");

    // Make version objects
    CVersionNumber fileVer(sExeVer.toStdString());
    CVersionNumber minVer(msPlayerVerMin.toStdString());

    // If we don't have a max version restriction we need to set it to
    // a very high number before we compare.
    QString sMaxVer = msPlayerVerMax;
    if (sMaxVer == "")
    {
        sMaxVer = QString("%1.%2.%3.%4").arg(INT_MAX).arg(INT_MAX).arg(INT_MAX).arg(INT_MAX);
    }

    CVersionNumber maxVer(sMaxVer.toStdString());

    // Compare versions
    return (fileVer >= minVer && fileVer <= maxVer);
}

void CPluginInfo::SetBootstrapType(const QString &bootstrapType)
{
    if( bootstrapType == "Client" )
    {
        m_BootstrapType = BOOTSTRAP_CLIENT;
    }
    else if( bootstrapType == "Plugin" )
    {
        m_BootstrapType = BOOTSTRAP_PLUGIN;
    }
    else
    {
        m_BootstrapType = BOOTSTRAP_NONE;
    }
}

bool CPluginInfo::IsBootstrappable()
{
    return m_BootstrapType != BOOTSTRAP_NONE;
}
