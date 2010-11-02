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

#ifndef PLUGININFO_H
#define PLUGININFO_H

#include "componentinfo.h"

#include <QString>
#include <vector>

/*************************************************************************/ /**
    Contains downloaded info about an upgradable plugin.
******************************************************************************/
class CPluginInfo : public CComponentInfo
{
public:

    /*********************************************************************/ /**
        Ctor/Dtor
    **************************************************************************/
    CPluginInfo() :
        mPlayerInstalled(eUnknown),
        mPlayerDefault(eUnknown){ }
    virtual ~CPluginInfo() { }


    enum BootStrapType
    {
        BOOTSTRAP_NONE = 0,
        BOOTSTRAP_CLIENT,
        BOOTSTRAP_PLUGIN
    };

    /*********************************************************************/ /**
        Getters/Setters
    **************************************************************************/
    QString GetId() { return msId; }
    QString GetPlayerName() { return msPlayerName; }
    QString GetPlayerPath() { return msPlayerPath; }
    QString GetPlayerDir(); // including slash
    QString GetPlayerFilename(); // including slash
    virtual QString GetInstallPath() { return GetPlayerDir() + msInstallDir + "/"; }
    QString GetPlayerVerMin() { return msPlayerVerMin; }
    QString GetPlayerVerMax() { return msPlayerVerMax; }
    BootStrapType GetBootstrapType() { return m_BootstrapType; }

    void SetId(
        const QString& sId) { msId = sId; }
    void SetPlayerName(
        const QString& sPlayerName) { msPlayerName = sPlayerName; }
    void SetPlayerPath(
        const QString& sPlayerPath) { msPlayerPath = sPlayerPath;
                                      mPlayerInstalled = eUnknown; }
    void SetInstallDir(
        const QString& sInstallDir) { msInstallDir = sInstallDir; }
    void SetPlayerVerMin(
        const QString& sPlayerVerMin) { msPlayerVerMin = sPlayerVerMin; }
    void SetPlayerVerMax(
        const QString& sPlayerVerMax) { msPlayerVerMax = sPlayerVerMax; }
    void SetBootstrapType(
        const QString& bootstrapType);
    
    /*********************************************************************/ /**
        Isers
    **************************************************************************/
    virtual bool IsInstalled();
    virtual bool IsVersionNewer();
    virtual bool IsRunning();
    bool IsPlayerInstalled();
    bool IsPlayerDefault();
    bool IsBootstrappable();

    /*********************************************************************/ /**
        Clear
    **************************************************************************/
    virtual void
    Clear();

    /*********************************************************************/ /**
        Shuts down the player this plugin belongs to if it's running.
        Returns true if it was shut down successfully.
    **************************************************************************/
    virtual bool
    KillProcess();

    virtual bool
    ExecuteProcess();


    /*********************************************************************/ /**
    Works out if plugin is installed by looking in the registry.
    **************************************************************************/
    bool
        IsPluginInstalled();

private:

    /*********************************************************************/ /**
        Does the installed player version match that of this plugin?
    **************************************************************************/
    bool
    VersionMatch();

    /*********************************************************************/ /**
        Reads the version from the registry.
    **************************************************************************/
    QString
    GetInstalledPluginVersion();

    QString msId;
    QString msPlayerName;
    QString msPlayerPath;
    QString msInstallDir;  // plugin install dir relative to player dir
    QString msPlayerVerMin;
    QString msPlayerVerMax;

    BootStrapType m_BootstrapType;
    
    static QString msDefaultPlayer;

    trool       mPlayerInstalled;
    trool       mPlayerDefault;
    
};

#endif // PLUGININFO_H
