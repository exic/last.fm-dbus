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

#ifndef COMPONENTINFO_H
#define COMPONENTINFO_H

#include <QString>
#include <QUrl>

#include <vector>

/*************************************************************************/ /**
    Contains downloaded info about an upgradable component.
******************************************************************************/
class CComponentInfo
{
public:

    /*********************************************************************/ /**
        Ctor/Dtor
    **************************************************************************/
    CComponentInfo() : mInstalled(eUnknown), mVersionNewer(eUnknown), mMajorUpgrade( false ) { }
    virtual ~CComponentInfo() { }

    /*********************************************************************/ /**
        Getters/Setters
    **************************************************************************/
    QString GetName() { return msName; }
    QString GetPath() { return msPath; }
    QString GetPathFilename();
    QString GetPathFileExt();
    QString GetDownloadURL() { return msDownloadURL; }
    unsigned int GetSize() { return mSize; }
    QString GetURLFilename();
    QString GetURLFileExt();
    QString GetVersion() { return msVersion; }
    virtual QString GetInstallPath();
    QString GetInstallArgs() { return msInstallArgs; }
    QString GetDescription() const { return mDescription; }
    QUrl GetImage() const { return mImage; }

    void SetMajorUpgrade( bool major ) { mMajorUpgrade = major; }
    void SetImage( const QUrl& url ) { mImage = url; }
    void SetDescription( const QString& desc ) { mDescription = desc; }

    void SetName(
        const QString& sName) { msName = sName; }
    void SetPath(
        const QString& sPath) { msPath = sPath; }
    void SetDownloadURL(
        const QString& sDownloadURL) { msDownloadURL = sDownloadURL; }
    void SetSize(
        unsigned int size) { mSize = size; }
    void SetVersion(
        const QString& sVersion) { msVersion = sVersion; }
    void SetInstallArgs(
        const QString& sInstallArgs) { msInstallArgs = sInstallArgs; }

    /*********************************************************************/ /**
        Isers
    **************************************************************************/
    bool IsMajorUpgrade() const { return mMajorUpgrade; }
    virtual bool IsInstalled();
    virtual bool IsVersionNewer();
    virtual bool IsRunning();
    virtual bool IsApp();
    
    /*********************************************************************/ /**
        Clear
    **************************************************************************/
    virtual void
    Clear();

    /*********************************************************************/ /**
        Shuts down this component if it's running.
    **************************************************************************/
    virtual bool
    KillProcess();

    virtual bool
    ExecuteProcess(){ return false; };

protected:

    // Tri-state bool
    typedef signed char trool;
    enum ETroolValues
    {
        eUnknown = -1,
        eFalse   =  0,
        eTrue    =  1
    };

    /*********************************************************************/ /**
        Works out if this component is installed on the system by checking
        whether Program Files\ + sPath exists. Subclass and override to provide
        more specific checks.
    **************************************************************************/
    bool
    ProgramExists(
        const QString& sProgram);

    /*********************************************************************/ /**
        Reads version number from exe. Returns "" if no version info was found.
    **************************************************************************/
    QString
    GetExeVersion(
        const QString& sPath);

    /*********************************************************************/ /**
        Reads a "Version" value from sKey. Returns "" if no version info was
        found.
    **************************************************************************/
    QString
    GetRegVersion(
        const QString& sRegKey);

    trool       mInstalled;
    trool       mVersionNewer;

private:

    QString msName;
    QString msPath;
    QString msDownloadURL;
    QString msVersion;
    QString msInstallArgs;
    QString mDescription;
    QUrl mImage;

    bool mMajorUpgrade;
    unsigned int mSize;

};

#endif // COMPONENTINFO_H
