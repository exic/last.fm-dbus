/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
 *      Jono Cole, Last.fm Ltd <jono@last.fm>                              *
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

#include "autoupdater.h"
#include "logger.h"
#include "exceptions.h"
#include "MooseCommon.h"
#include "UnicornCommon.h"

#ifdef WIN32
    #include <windows.h>
#endif

#include <QApplication>

#include <sstream>

using namespace std;

/******************************************************************************
    CAutoUpdater
******************************************************************************/
CAutoUpdater::CAutoUpdater( QObject* parent ) :
        QObject( parent ),
        mbRestartNeeded(false)
{
    connect( &mInfoGetter, SIGNAL( updateInfoDone( bool, QString ) ),
             this,         SLOT  ( updateInfoDone( bool, QString ) ), Qt::QueuedConnection );
}

/******************************************************************************
    CheckForUpdates
******************************************************************************/
void
CAutoUpdater::checkForUpdates()
{
    mnDownloadTasks = 1;
    mnCurrentDownload = 0;

    mVecUpdatables.clear();
    mbUpdates = false;
    mbErrorFlag = false;

    // These are now async
    mInfoGetter.GetUpdateInfo(&mApp, &mVecPlugins, true);
}

/******************************************************************************
    appInfoDone
******************************************************************************/
void
CAutoUpdater::updateInfoDone(
    bool    error,
    QString message)
{
    if (mbErrorFlag)
    {
        // Plugin info has already signalled an error
        return;
    }

    if (error)
    {
        mbErrorFlag = true;
        emit updateCheckDone(false, true, message);
    }
    else
    {
        if (mApp.IsVersionNewer())
        {
            LOG(3, "Newer app version found.\n");
            mVecUpdatables.push_back(&mApp);
            mbUpdates = true;
        }

        // We should only return true if there are true updates available,
        // not just if there is a plugin we haven't installed.
        for (int i = 0; i < static_cast<int>(mVecPlugins.size()); ++i)
        {
            CPluginInfo& plugin = mVecPlugins.at(i);
            if (plugin.IsInstalled() && plugin.IsVersionNewer())
            {
                LOG(3, "Newer plugin version found for " << plugin.GetName() << ".\n");
                mVecUpdatables.push_back(&plugin);
                mbUpdates = true;
            }
            else if (!plugin.IsInstalled() && plugin.IsPlayerInstalled())
            {
                LOG(3, "New plugin for installed player " << plugin.GetName() << " found.\n");

                // By pushing it back here, we include it in the list alongside
                // the updates without popping up a dialog box on each startup
                // if it's a plugin the user simply doesn't want.
                mVecUpdatables.push_back(&plugin);
            }
        }

        emit updateCheckDone(mbUpdates);
    }
}

/******************************************************************************
    downloadUpdates
******************************************************************************/
void
CAutoUpdater::downloadUpdates(
    std::vector<CComponentInfo*> vecInfo)
{
    mVecDownloads = vecInfo;
    mnDownloadTasks = static_cast<int>(mVecDownloads.size());

    LOG(3, "We have " << mnDownloadTasks << " download tasks\n");

    if (mnDownloadTasks > 0)
    {
        mbRestartNeeded = false;
        mnCurrentDownload = -1;
        downloadNext();
    }
    else
    {
        emit updateDownloadDone();
    }
}

/******************************************************************************
    downloadNext
******************************************************************************/
void
CAutoUpdater::downloadNext()
{
    mnCurrentDownload++;
    CComponentInfo& current = *mVecDownloads.at(mnCurrentDownload);

    QString sExt = current.GetURLFileExt();
    QString sFile = current.GetURLFilename();
    QString sInstallPath = current.GetInstallPath();
    QString sTempPath = QDir::tempPath() + "/";
    QString sDownloadPath;

    if (sExt == "exe")
    {
        sDownloadPath = sTempPath + sFile;
    }
    else if (sExt == "bz2")
    {
        sDownloadPath = sTempPath + sFile;
    }
    else if (sExt == "dll")
    {
        sDownloadPath = sInstallPath + sFile;
    }
    else
    {
        LOG(1, "Download '" << sFile << "' with ext: " << sExt << " not allowed" << "\n");
        Q_ASSERT(false);
        return;
    }

    LOG(3, "Will download " << current.GetDownloadURL() << " to '" <<
        sDownloadPath << "'\n");

    emit newFile(tr("Downloading %1").arg(current.GetURLFilename()));

    mFile.setFileName(sDownloadPath);
    if (!mFile.open(QIODevice::WriteOnly))
    {
        // Something went wrong
        QString msg = QString("Couldn't open file '%1' for writing.").arg(mFile.fileName());
        LOG(1, msg << "\n");

        mFile.close();
        emit updateDownloadDone(true, msg);
        return;
    }

    QUrl url(current.GetDownloadURL());

    mTransport = new CachedHttp(this);
    mTransport->setHost(url.host());

    connect(mTransport, SIGNAL(requestFinished(int, bool)),
            this,       SLOT  (downloadDone(int, bool)), Qt::QueuedConnection);
    connect(mTransport, SIGNAL(dataReadProgress(int, int)),
            this,       SLOT  (httpProgressMade(int, int)));
    connect(mTransport, SIGNAL(stateChanged(int)),
            this,       SLOT  (statusChanged(int)));

    mHttpId = mTransport->get( url.path(), &mFile );

    mCancelled = false;
}


/******************************************************************************
    downloadDone
******************************************************************************/
void
CAutoUpdater::downloadDone(
    int  requestId,
    bool error)
{
    if (requestId != mHttpId) { return; }

    Q_ASSERT(mTransport);

    QHttpResponseHeader header = mTransport->lastResponse();
    int statusCode = header.statusCode();

    //Successfull response is 2xx
    if (statusCode < 200 || statusCode >= 300 )
        error = true;

    if (mCancelled)
    {
        LOG(3, "Download of update cancelled. Not emitting any signals.\n");
        mFile.close();
        mTransport->deleteLater();
        return;
    }

    if (error)
    {
        QString errorString = header.reasonPhrase();
        // Something went wrong
        LOG(2, "Download of component failed. Error: " <<
            statusCode << " - " << errorString << "\n");

        mFile.close();
        mTransport->deleteLater();

        emit updateDownloadDone(true, QString(tr("Download failed ( %1 - %2 )")).
                                                    arg(statusCode).
                                                    arg(errorString));
    }
    else
    {
        // Everything's fine
        CComponentInfo& current = *mVecDownloads.at(mnCurrentDownload);
        QString installerPath = mFile.fileName();
        mFile.close();
        mTransport->deleteLater();

        if ( current.GetSize() > 0 && ( current.GetSize() != mFile.size() ) )
        {
            // Something went wrong
            LOG(2, "Download of component failed. Expected size: " <<
                current.GetSize() << " - size of downloaded file: " << mFile.size() << "\n");

            emit updateDownloadDone(true, QString(tr("The update servers are busy. Please try again later.")));

            return;
        }

      #ifndef WIN32
        if ( current.GetURLFileExt() == "bz2" )
        {
            QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );

            QStringList args;
            args << "xfj";
            args << installerPath;

            QProcess proc( this );
          #ifdef Q_WS_MAC
            QDir d = QDir( QCoreApplication::applicationDirPath() );
            d.cdUp();
            proc.setWorkingDirectory( d.absolutePath() );
          #else
            proc.setWorkingDirectory( QCoreApplication::applicationDirPath() );
          #endif

            proc.start( "tar", args );
            proc.waitForFinished( 60000 );

            QFile( installerPath ).remove();
            QApplication::restoreOverrideCursor();

            if ( mnCurrentDownload < ( mnDownloadTasks - 1 ) )
            {
                downloadNext();
            }
            else
            {
                emit updateDownloadDone( false );
            }
        }
      #endif

      #ifdef WIN32
        if (current.GetURLFileExt() == "exe")
        {
            // Sometimes you've got to be pragmatic
            if (current.IsApp())
            {
                // Install this separately
                msAppDLPath = installerPath;
                mbRestartNeeded = true;

                if (mnCurrentDownload < (mnDownloadTasks - 1))
                {
                    downloadNext();
                }
                else
                {
                    emit updateDownloadDone(false);
                }

                return;
            }

            QString installDir = QDir::toNativeSeparators( current.GetInstallPath() );
            QDir dir( installDir );
            if ( !dir.exists() )
            {
                LOGL( 3, "Install dir '" << installDir << "' didn't exist. Creating it." );
                bool success = dir.mkdir( installDir );

                if ( !success )
                {
                    LOGL( 3, "Folder creation failed" );
                }
            }

            WCHAR buf[MAX_PATH];
            DWORD res = GetShortPathNameW(
                reinterpret_cast<LPCWSTR>( installDir.utf16() ), buf, MAX_PATH );
            if ( res == 0 )
            {
                DWORD err = GetLastError();
                LOGL( 2, "Couldn't convert path to short version, using long. System error: " << err );
            }
            else
            {
                installDir = QString::fromUtf16(
                    reinterpret_cast<const ushort*>( buf ) );
            }

            installDir = "\"" + installDir + "\"";

            // HACK! Turning into a bit of a hackfest this. Seeing as all the
            // exes we use so far are Inno Setup ones, we can tack on
            // a "/DIR" param with the correct plugin path (which we can't
            // know at installer compile time) onto the end of the args string.
            // This will most probably die horribly if run on a non-Inno installer.
            QString installArgs = current.GetInstallArgs() + " /DIR=" + installDir;

            LOG(3, "Running '" << installerPath << " " << installArgs << "'\n");

            // Run installer
            // Must use ShellExecute because otherwise the elevation dialog
            // doesn't appear when launching the installer on Vista.
            SHELLEXECUTEINFOW sei;
            memset(&sei, 0, sizeof(sei));

            sei.cbSize = sizeof(sei);
            sei.fMask  = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_DDEWAIT;
            sei.hwnd   = GetForegroundWindow();
            sei.lpVerb = L"open";
            sei.lpFile = reinterpret_cast<LPCWSTR>(installerPath.utf16());
            sei.lpParameters = reinterpret_cast<LPCWSTR>(installArgs.utf16());
            sei.nShow  = SW_SHOWNORMAL;

            BOOL bOK = ShellExecuteExW(&sei);
            if (bOK)
            {
                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                WaitForSingleObject(sei.hProcess, 5000);
                CloseHandle(sei.hProcess);
                QApplication::restoreOverrideCursor();
            }
            else
            {
                LOG(1, "Couldn't ShellExecuteEx " << installerPath << " " <<
                    installArgs << ". GetLastError: " << GetLastError() << "\n");

                QString msg = tr("Failed to install '") + installerPath + tr("'.");
                emit updateDownloadDone(true, msg);
                return;
            }

            if (mnCurrentDownload < (mnDownloadTasks - 1))
            {
                downloadNext();
            }
            else
            {
                emit updateDownloadDone(false);
            }
        }
        else if (current.GetURLFileExt() == "dll")
        {
            // Do something to enable uninstalling dlls here
            Q_ASSERT_X(false, "CAutoUpdate::downloadDone", "Not implemented.");
        }
      #endif // WIN32

    } // end if error

}

/******************************************************************************
    Cancel
******************************************************************************/
void
CAutoUpdater::Cancel()
{
    mCancelled = true;

    mInfoGetter.Cancel();

    if (mTransport)
    {
        mTransport->abort();
    }
}

/******************************************************************************
    LaunchAppInstaller
******************************************************************************/
bool
CAutoUpdater::LaunchAppInstaller()
{
#ifdef WIN32

    LOG(3, "Shutting down to update app.\n");

    // The Updater.exe will wait for this mutex to become invalid, i.e.
    // AS having shut down, before launching the installer.
    string sMutex("AudioscrobblerUpdate-BBFA5A2D-4B6F-4d2c-8B2F-02F733C5B02C");
    CreateMutexA(NULL, TRUE, sMutex.c_str());

	QDir appDir = QDir( QCoreApplication::applicationDirPath() );

	QString updaterPath = appDir.absoluteFilePath( "Updater.exe" );
	#ifndef QT_NO_DEBUG
		updaterPath = appDir.absoluteFilePath( "Updaterd.exe" );
	#endif

	// Synchronous
    QString tempUpdaterPath = MooseUtils::savePath( "Updaterd.exe" );
    QFile::remove( tempUpdaterPath );
    if ( QFile::exists( tempUpdaterPath ) )
    {
        LOG(1, "Removal of old Updater.exe failed. System error code: " <<
            GetLastError() << "\n");
    }
    
    bool ok = QFile::copy( updaterPath, tempUpdaterPath );
    if (!ok)
    {
        LOG(1, "Copying of Updater.exe failed. System error code: " <<
            GetLastError() << "\n");
        return false;
    }

    QStringList args;
    args << "R" << QString::fromStdString(sMutex) << msAppDLPath;

    LOG(3, "Will execute: " << tempUpdaterPath << " " << args.join(" ") << "\n");

    QProcess proc( this );
    proc.setWorkingDirectory( QCoreApplication::applicationDirPath() );
    bool started = proc.startDetached( tempUpdaterPath, args );

/*
    HINSTANCE h = ShellExecuteW(
        NULL, L"open", L"UpTemp.exe", sArgs.utf16(), NULL, SW_HIDE);
*/
    LOG(3, "Executed updater, instance: " << proc.pid() << "\n");

    if (!started)
    {
        // Something went wrong
        LOG(1, "Couldn't launch Updater. Error code: " << proc.error() << "\n" );
        return false;
    }

	return true;

#elif defined Q_WS_MAC
	QProcess::startDetached( "/bin/bash", QStringList() << QApplication::applicationDirPath() + "/updater-autorestart.sh" << MooseUtils::savePath( "lastfm.pid" ) );
		
	return true;
#else
    return false;
#endif // WIN32
}

/******************************************************************************
    CheckIfRunning
******************************************************************************/
bool
CAutoUpdater::CheckIfRunning(
    vector<CComponentInfo*>& vecInfo,
    vector<QString>&         vecNames)
{
    mVecRunning.clear();
    for (size_t i = 0; i < vecInfo.size(); ++i)
    {
        CComponentInfo& current = *vecInfo.at(i);

        if (current.IsApp())
        {
            continue;
        }

        if (current.IsRunning())
        {
            LOG(3, "Found running process: " << current.GetName() << "\n");

            // Found running process
            mVecRunning.push_back(&current);
            vecNames.push_back(current.GetName());
        }
    }

    return mVecRunning.size() > 0;
}

/******************************************************************************
    KillRunning
******************************************************************************/
bool
CAutoUpdater::KillRunning()
{
    bool bSuccess = true;
    for (size_t i = 0; i < mVecRunning.size(); ++i)
    {
        CComponentInfo& current = *mVecRunning.at(i);
        if (!current.KillProcess())
        {
            bSuccess = false;
            LOG(1, "Couldn't kill process: " << current.GetName() << "\n");
        }
    }

    return bSuccess;
}

/******************************************************************************
    statusChanged (slot)
******************************************************************************/
void
CAutoUpdater::statusChanged(
    int state)
{
    QString msg = UnicornUtils::QHttpStateToString(state);
    emit statusChange(msg);
}

/******************************************************************************
    httpProgressMade (slot)
******************************************************************************/
void
CAutoUpdater::httpProgressMade(
    int percentage,
    int total)
{
    // Calc a float between 0 and 1 representing overall progress
    float base = (float)mnCurrentDownload / (float)mnDownloadTasks;
    float thisProgress = (float)percentage / (float)total;
    float totalProgress = base + (thisProgress / mnDownloadTasks);

    // Convert to a percentage out of a 100
    int totalPercentage = int(totalProgress * 100);

    emit progressMade(totalPercentage, 100);

    int kbDownloaded = percentage / 1024;
    int kbTotal = total / 1024;
    QString msg = tr("Downloaded %L1kB of %L2kB.")
        .arg(kbDownloaded)
        .arg(kbTotal);

    emit statusChange(msg);

}
