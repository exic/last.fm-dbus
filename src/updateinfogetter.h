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

#ifndef UPDATEINFOGETTER_H
#define UPDATEINFOGETTER_H

#include "componentinfo.h"
#include "plugininfo.h"
#include "exceptions.h"

#include <QObject>

#include <vector>

class QDomNode;
class CachedHttp;


/*************************************************************************/ /**
    Downloads information about our apps and plugins from the last.fm
    server.
******************************************************************************/
class CUpdateInfoGetter : public QObject
{
    Q_OBJECT

public:

    /*********************************************************************/ /**
        Ctor/Dtor
    **************************************************************************/
    CUpdateInfoGetter();
    virtual ~CUpdateInfoGetter();

    /*********************************************************************/ /**
        Fill info with downloaded information about the Audioscrobbler
        application itself. Asynchronous, sends signal appInfoReceived when
        done.
    **************************************************************************/
    void
    GetUpdateInfo(
        CComponentInfo*           appInfo,
        std::vector<CPluginInfo>* vecPluginInfo,
        bool                      bForceRefresh = true);

    /*********************************************************************/ /**
        Stops the download.
    **************************************************************************/
    void
    Cancel();

signals:

    /*********************************************************************/ /**
        Application info downloaded.
    **************************************************************************/
    void
    updateInfoDone(
        bool error,
        QString errorMsg);

    /*********************************************************************/ /**
        Plugin info downloaded.
    **************************************************************************/
/*
    void
    pluginInfoDone(
        bool error,
        QString errorMsg);
*/

    /*********************************************************************/ /**
        Report a new progress percentage for the current operation.
    **************************************************************************/
    void
    progressMade(
        int percentage,
        int total);

    /*********************************************************************/ /**
        Report a new status message for the current operation.
    **************************************************************************/
    void
    statusChange(
        QString message);

private:

    /*********************************************************************/ /**
        Pulls the info down from server.
    **************************************************************************/
    void
    DownloadInfo();
    
    /*********************************************************************/ /**
        Populates a component info object.
    **************************************************************************/
    void
    PopulateComponent(
        const QDomNode& appNode,
        CComponentInfo& info);

    /*********************************************************************/ /**
        Populates a plugin info object.
    **************************************************************************/
    void
    PopulatePlugin(
        const QDomNode&     pluginNode,
        CPluginInfo&  info);

    CComponentInfo            mApp;
    std::vector<CPluginInfo>  mPlugins;

    CComponentInfo*           mExtApp;
    std::vector<CPluginInfo>* mExtPlugins;

    CachedHttp*               mHttp;
    int                       mGetId;

    bool                      mbCancelled;

private slots:

    /*********************************************************************/ /**
        Called by QHttp when done.
    **************************************************************************/
    void
    downloadFinished(
        int  requestId,
        bool error);

    /*********************************************************************/ /**
        Called by QHttp when its state changes.
    **************************************************************************/
    void
    statusChanged(
        int  state);

    void sendProgress(int /*p*/, int /*t*/) {
        //LOGL(3, "sendProgress made: " << p << "/" << t);
    }

    void readyRead(const class QHttpResponseHeader & /*resp*/) {
        //LOGL(3, "statusCode: " << resp.statusCode() );
        //LOGL(3, "reason: " << resp.reasonPhrase() );
    }

    void requestStarted(int /*id*/) {
        //LOGL(3, "id: " << id );
    }

    void responseHeaderReceived ( const QHttpResponseHeader & /*resp*/ ) {
        //LOGL(3, "statusCode: " << resp.statusCode() );
        //LOGL(3, "reason: " << resp.reasonPhrase() );
    }
};

#endif // UPDATEINFOGETTER_H
