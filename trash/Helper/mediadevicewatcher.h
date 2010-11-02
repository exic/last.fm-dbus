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

#ifndef MEDIADEVICE_H
#define MEDIADEVICE_H

#include "interfaces/MediaDeviceInterface.h"

#include <QThread>
#include <QSqlDatabase>
#include <QFileSystemWatcher>
#include <QMutex>

#include "TrackInfo.h"

#ifdef Q_WS_MAC
    #include "CocoaWatcher.h"
    #include <CoreFoundation/CoreFoundation.h>
    #include <Carbon/Carbon.h>
#endif

class CocoaThread;


class MediaDeviceWatcher : public QObject
{
    Q_OBJECT

public:
    MediaDeviceWatcher();
    ~MediaDeviceWatcher();

public slots:
    void forceDetection( const QString& path );

  #ifdef Q_WS_MAC
    void startWithiTunes();
  #endif

private slots:
    void deviceAdded( const QString& uid );
    void deviceChangeStart( const QString& uid, QDateTime lastItunesUpdateTime );
    void deviceChangeEnd( const QString& uid );

    void trackChanged( const TrackInfo& track, int playCounter );

    void shutdownHelper( const QString& path );

private:
    MediaDeviceInterface* m_mediaDevice;
    QString m_savePath;
    QDomDocument m_newsubdoc;
    QDomElement m_submitQueue;
    QString m_runPath;

    // DRY function, builds m_subdoc domtree
    void buildDomTree( const QList<TrackInfo>& );

  #ifdef Q_WS_MAC
    CocoaThread* m_cthread;
  #endif

    bool updateTrack( TrackInfo track, bool autoScrobble = false );
    QList<TrackInfo> readQueue( const QString& uid );
};


#ifdef Q_WS_MAC

class CocoaThread : public QThread
{
    public:
    CocoaThread( MediaDeviceWatcher* parent ) : QThread( parent ), m_parent( parent ), m_itunesRunning( false )
    {
        start();
    }

    ~CocoaThread() { m_run = false; }

    protected:
        void run();

    private:
        CocoaWatcher* m_cocoa;
        MediaDeviceWatcher* m_parent;
        bool m_run;
        bool m_itunesRunning;

        bool isITunesRunning();
};


#endif // end Q_WS_MAC

#endif
