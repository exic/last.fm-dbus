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

#ifndef AUTOUPDATER_H
#define AUTOUPDATER_H

#include "updateinfogetter.h"
#include "appinfo.h"
#include "plugininfo.h"
#include "CachedHttp.h"

#include <QtCore>

#include <vector>
#include <string>

/*************************************************************************/ /**
    Manages the process of checking for and downloading automatic updates.
    Observer of FileDownloader, Observable by WizardProgressPage.
******************************************************************************/
class CAutoUpdater : public QObject
{
    Q_OBJECT

public:

    /*********************************************************************/ /**
        Ctor/Dtor
    **************************************************************************/
    CAutoUpdater( QObject* parent = 0 );

    /*********************************************************************/ /**
        Contacts the web server to find out if updates are available.
        Asynchronous, sends signal updateCheckDone when done. Call
        GetUpdatables to get hold of them.
    **************************************************************************/
    void
    checkForUpdates();

    /*********************************************************************/ /**
        Returns vector of pointers to the updatable components.
    **************************************************************************/
    std::vector<CComponentInfo*>&
    GetUpdatables() { return mVecUpdatables; }

    /*********************************************************************/ /**
        Return true if any of the apps in the vector are running. Stores
        which apps are running internally.

        @param[in]  vecInfo - components to check
        @param[out] vecNames - receives the names of the running processes
    **************************************************************************/
    bool
    CheckIfRunning(
        std::vector<CComponentInfo*>& vecInfo,
        std::vector<QString>&         vecNames);

    /*********************************************************************/ /**
        Kills the processes that were found running by calling CheckIfRunning.
        Does nothing if CheckIfRunning hasn't been called first.
    **************************************************************************/
    bool
    KillRunning();

    /*********************************************************************/ /**
        Returns true if the app itself is one of the components that have
        been selected for update.
    **************************************************************************/
    bool
    IsRestartNeeded() { return mbRestartNeeded; }

    /*********************************************************************/ /**
        Launches the separate updater.exe to install a new version of the app.
    **************************************************************************/
    bool
    LaunchAppInstaller();

    /*********************************************************************/ /**
        Updates passed in components. Asynchronous, sends signal
        updateDownloadDone when done.
    **************************************************************************/
    void
    downloadUpdates(
        std::vector<CComponentInfo*> vecInfo);

    /*********************************************************************/ /**
        Cancel
    **************************************************************************/
    void
    Cancel();

    std::vector<CPluginInfo>&
        getPluginList(){ return mVecPlugins; }

signals:

    /*********************************************************************/ /**
        Emitted when an update check is finished.
    **************************************************************************/
    void
    updateCheckDone(
        bool updatesAvailable,
        bool error = false,
        QString errorMsg = "");

    /*********************************************************************/ /**
        Emitted when the updates are downloaded and installed.
    **************************************************************************/
    void
    updateDownloadDone(
        bool error = false,
        QString errorMsg = "" );

    /*********************************************************************/ /**
        Report a new progress percentage for the current operation.
    **************************************************************************/
    void
    progressMade(
        int percentage,
        int total);

    /*********************************************************************/ /**
        Reports when a new file download starts.
    **************************************************************************/
    void
    newFile(
        QString message);

    /*********************************************************************/ /**
        Report a new status message for the current operation.
    **************************************************************************/
    void
    statusChange(
        QString message);

private:

    /*********************************************************************/ /**
        Call this to advance to next queued download.
    **************************************************************************/
    void
    downloadNext();

    CAppInfo                        mApp;
    std::vector<CPluginInfo>        mVecPlugins;
    std::vector<CComponentInfo*>    mVecUpdatables;
    std::vector<CComponentInfo*>    mVecRunning;
    std::vector<CComponentInfo*>    mVecDownloads;

    CUpdateInfoGetter               mInfoGetter;

    // Using pointer because using the same QHttp object repeatedly
    // doesn't seem to be reliable.
    QPointer<CachedHttp>            mTransport;

    QFile                           mFile;

    int                             mHttpId;
    bool                            mbUpdates;
    bool                            mbErrorFlag;

    QString                         msAppDLPath;

    int                             mnCurrentDownload;
    int                             mnDownloadTasks;
    bool                            mbRestartNeeded;

    bool                            mCancelled;

private slots:

    /*********************************************************************/ /**
        Return slot for checkForUpdates.
    **************************************************************************/
    void
    updateInfoDone(
        bool    error,
        QString message);

    /*********************************************************************/ /**
        Called by QHttp when its state changes.
    **************************************************************************/
    void
    statusChanged(
        int  state);

    /*********************************************************************/ /**
        Called by QHttp to report download progress.
    **************************************************************************/
    void
    httpProgressMade(
        int percentage,
        int total);

    /*********************************************************************/ /**
        Return slot for each file download.
    **************************************************************************/
    void
    downloadDone(
        int  requestId,
        bool error);

};

#endif // AUTOUPDATER_H
