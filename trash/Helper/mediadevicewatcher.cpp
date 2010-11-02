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

#include <QtCore>
#include <QSqlQuery>
#include <QSqlResult>
#include <QSqlError>
#include <QFileSystemWatcher>

#include "mediadevicewatcher.h"
#include "MooseCommon.h"
#include "logger.h"
#include "LastFmSettings.h"

#ifdef Q_WS_MAC
    #include <sys/sysctl.h>
    #include <errno.h>
    #include <stdlib.h>
    #include <stdio.h>
#endif

#ifdef WIN32
    #include "windows.h"
    #include "shfolder.h"
#endif

#define ESC( token ) QString( token ).replace( "\'", "''" )

QString XML_VERSION = "1.0";

#ifdef Q_WS_MAC
ComponentInstance theComponent = NULL;
OSAID m_OSA_CheckITunes = kOSANullScript;
#endif


void
sendToInstance( const QString& data )
{
    if ( The::settings().isFirstRun() )
        return;

    #ifdef QT_NO_DEBUG
        #ifdef WIN32
            QString appExe = "LastFM.exe";
        #endif
        #ifdef Q_WS_MAC
            QString appExe = "Last.fm";
        #endif
    #else
        #ifdef WIN32
            QString appExe = "LastFMd.exe";
        #endif
        #ifdef Q_WS_MAC
            QString appExe = "Last.fm_debug";
        #endif
    #endif

    QString app = QCoreApplication::applicationDirPath() + "/" + appExe;
    QStringList params( "-tray" );
    params << data;

    qDebug() << "Starting new instance of" << app << params;

    QProcess::startDetached( app, params );
}


MediaDeviceWatcher::MediaDeviceWatcher()
{
    Q_DEBUG_BLOCK;

    #ifdef Q_WS_MAC
        #ifndef QT_NO_DEBUG
        m_runPath = QDir( QCoreApplication::applicationDirPath() ).canonicalPath() + "/LastFmHelper_debug";
        #else
        m_runPath = QDir( QCoreApplication::applicationDirPath() ).canonicalPath() + "/LastFmHelper";
        #endif
    #endif

    //FIXME: should this really be here?
    QObject* obj = MooseUtils::loadService( "itunesdevice" );
    m_mediaDevice = qobject_cast<MediaDeviceInterface*>( obj );

    if ( !m_mediaDevice )
        qDebug() << "Loading mediadevice plugin failed";
    else
    {
        qDebug() << "Loading mediadevice succeeded";
        m_mediaDevice->setupWatchers();

        connect( m_mediaDevice, SIGNAL( deviceAdded( QString ) ),
                 this,     SLOT( deviceAdded( QString ) ) );
        connect( m_mediaDevice, SIGNAL( deviceChangeStart( QString, QDateTime ) ),
                 this,     SLOT( deviceChangeStart( QString, QDateTime ) ) );
        connect( m_mediaDevice, SIGNAL( deviceChangeEnd( QString ) ),
                 this,     SLOT( deviceChangeEnd( QString ) ) );
        connect( m_mediaDevice, SIGNAL( trackChanged( TrackInfo, int ) ),
                 this,     SLOT( trackChanged( TrackInfo, int ) ) );
    }

    #ifdef Q_WS_MAC
        QFileSystemWatcher* fs = new QFileSystemWatcher( this );
        QDir dir = QDir( QCoreApplication::applicationDirPath() );
        dir.cdUp();
        dir.cdUp();

        fs->addPath( dir.canonicalPath() );
        fs->addPath( QCoreApplication::applicationDirPath() );

        #ifndef QT_NO_DEBUG
            fs->addPath( QCoreApplication::applicationDirPath() + "/Last.fm_debug" );
        #else
            fs->addPath( QCoreApplication::applicationDirPath() + "/Last.fm" );
        #endif

        connect( fs, SIGNAL( fileChanged( QString ) ), SLOT( shutdownHelper( QString ) ) );
        connect( fs, SIGNAL( directoryChanged( QString ) ), SLOT( shutdownHelper( QString ) ) );

        m_cthread = new CocoaThread( this );
    #endif
}


MediaDeviceWatcher::~MediaDeviceWatcher()
{
    #ifdef Q_WS_MAC
        delete m_cthread;
    #endif
}


#include "mediadevices/ipod/IpodDevice.h"

QList<TrackInfo>
MediaDeviceWatcher::readQueue( const QString& uid )
{
    QString user = The::settings().mediaDeviceUser( uid );

    QList<TrackInfo> queue;
    QString path = MooseUtils::savePath( user + "_mediadevice.xml" );

    QFile file( path );
    if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        qDebug() << "Could not open cache file to read submit queue: " << path;
        return queue;
    }

    QTextStream stream( &file );
    stream.setCodec( "UTF-8" );

    QDomDocument d;
    QString contents = stream.readAll();

    if ( !d.setContent( contents ) )
    {
        qDebug() << "Couldn't parse file: " << path;
        return queue;
    }

    const QString ITEM( "item" ); //so we don't construct these QStrings all the time

    for( QDomNode n = d.namedItem( "submissions" ).firstChild(); !n.isNull() && n.nodeName() == ITEM; n = n.nextSibling() )
        queue << TrackInfo( n.toElement() );

    return queue;
}


void
MediaDeviceWatcher::trackChanged( const TrackInfo& track, int playCounter )
{
    // since we subtract playcounts in iTunesDevice due to iTunes weirdness
    // this is indeed a necessary check. DO NOT REMOVE! --mxcl
    if ( track.playCount() > 0 )
    {
        qDebug() << "Scrobbling" << playCounter << "plays for" << track.artist() << "-" << track.track();

        QDomElement i = track.toDomElement( m_newsubdoc );
        m_submitQueue.appendChild( i );
    }
}


void
MediaDeviceWatcher::buildDomTree( const QList<TrackInfo>& tl )
{
    m_newsubdoc = QDomDocument();
    m_submitQueue = m_newsubdoc.createElement( "submissions" );
    m_submitQueue.setAttribute( "product", "Audioscrobbler" );
    m_submitQueue.setAttribute( "version", XML_VERSION );
    m_submitQueue.setAttribute( "lastSubmission", QDateTime::currentDateTime().toTime_t() );

    foreach( TrackInfo track, tl )
    {
        QDomElement i = track.toDomElement( m_newsubdoc );
        m_submitQueue.appendChild( i );
    }
}


void
MediaDeviceWatcher::deviceAdded( const QString& uid )
{
    Q_DEBUG_BLOCK;

    QString user = The::settings().mediaDeviceUser( uid );
    qDebug() << "A new mediadevice found" << uid << "for user" << user;

    // If there was no user found, tell the main app.
    if ( user.isEmpty() )
        sendToInstance( "container://addMediaDevice/" + uid );

    else if ( user != "<disabled>" && The::settings().isManualIpod( uid ) ) 
    {
        Q_DEBUG_BLOCK;

        QObject* o = QPluginLoader( MooseUtils::servicePath( "Ipod_device" ) ).instance();
        MyMediaDeviceInterface* plugin = qobject_cast<MyMediaDeviceInterface*>(o);

        if (plugin)
        {
            plugin->setUniqueId( uid );

            //HACK
            plugin->setMountPath( QSettings().value( "devicePaths/" + uid + "/path" ).toString() );

            buildDomTree( plugin->tracksToScrobble() );
            deviceChangeEnd( uid );
        }

        delete o;
    }
}


void
MediaDeviceWatcher::deviceChangeStart( const QString& uid, QDateTime lastItunesUpdateTime )
{
    QString user = The::settings().mediaDeviceUser( uid );

    if ( !user.isEmpty() && user != "<disabled>" )
    {
        buildDomTree( readQueue( uid ) );
        m_submitQueue.setAttribute( "lastItunesUpdate", lastItunesUpdateTime.toTime_t() );
    }
}


void
MediaDeviceWatcher::deviceChangeEnd( const QString& uid )
{
    QString user = The::settings().mediaDeviceUser( uid );

    if ( !user.isEmpty() && user != "<disabled>" )
    {
        QFile subfile( MooseUtils::savePath( user + "_mediadevice.xml" ) );
        if( !subfile.open( QIODevice::WriteOnly | QIODevice::Text ) )
        {
            qDebug() << "Could not open cache file to write submit queue";
            return;
        }

        QDomNode submitNode = m_newsubdoc.importNode( m_submitQueue, true );
        m_newsubdoc.appendChild( submitNode );

        QTextStream stream( &subfile );
        stream.setCodec( "UTF-8" );
        stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
        stream << m_newsubdoc.toString();
        subfile.close();

        // send it even if empty so client can show a nothing to scrobble message
        sendToInstance( "container://checkScrobblerCache/" + user );
    }
}


void
MediaDeviceWatcher::forceDetection( const QString& path )
{
    m_mediaDevice->forceDetection( path );
}


void
MediaDeviceWatcher::shutdownHelper( const QString& path )
{
    // Get the path again to check if we've been moved to the trash
    #ifndef QT_NO_DEBUG
        QString mypath = QDir( QCoreApplication::applicationDirPath() ).canonicalPath() + "/LastFmHelper_debug";
    #else
        QString mypath = QDir( QCoreApplication::applicationDirPath() ).canonicalPath() + "/LastFmHelper";
    #endif

    if ( !QFile::exists( m_runPath ) || m_runPath != mypath )
    {
        qDebug() << "Shutting down cause of:" << path;
        qDebug() << "Path to me:" << mypath;
        qDebug() << "Runpath:" << m_runPath;
        qDebug() << "Do I still exist?" << QFile::exists( m_runPath );

        QCoreApplication::quit();
    }
}


#ifdef Q_WS_MAC

void
MediaDeviceWatcher::startWithiTunes()
{
    if ( LastFmUserSettings( The::settings().currentUsername() ).launchWithMediaPlayer() )
        sendToInstance( "" );
}


typedef struct kinfo_proc kinfo_proc;

// Returns a list of all BSD processes on the system.  This routine
// allocates the list and puts it in *procList and a count of the
// number of entries in *procCount.  You are responsible for freeing
// this list (use "free" from System framework).
// On success, the function returns 0.
// On error, the function returns a BSD errno value.
static int GetBSDProcessList( kinfo_proc **procList, size_t *procCount )
{
    int                 err;
    kinfo_proc *        result;
    bool                done;
    static const int    name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
    // Declaring name as const requires us to cast it when passing it to
    // sysctl because the prototype doesn't include the const modifier.
    size_t              length;

    // We start by calling sysctl with result == NULL and length == 0.
    // That will succeed, and set length to the appropriate length.
    // We then allocate a buffer of that size and call sysctl again
    // with that buffer.  If that succeeds, we're done.  If that fails
    // with ENOMEM, we have to throw away our buffer and loop.  Note
    // that the loop causes use to call sysctl with NULL again; this
    // is necessary because the ENOMEM failure case sets length to
    // the amount of data returned, not the amount of data that
    // could have been returned.
    result = NULL;
    done = false;
    *procCount = 0;

    do
    {
        // Call sysctl with a NULL buffer.
        length = 0;
        err = sysctl( (int *) name, ( sizeof( name ) / sizeof( *name ) ) - 1, NULL, &length, NULL, 0 );
        if (err == -1)
        {
            err = errno;
        }

        // Allocate an appropriately sized buffer based on the results
        // from the previous call.
        if ( err == 0 )
        {
            result = (kinfo_proc*)malloc( length );
            if ( result == NULL )
            {
                err = ENOMEM;
            }
        }

        // Call sysctl again with the new buffer.  If we get an ENOMEM
        // error, toss away our buffer and start again.
        if ( err == 0 )
        {
            err = sysctl( (int *) name, ( sizeof( name ) / sizeof( *name ) ) - 1, result, &length, NULL, 0 );
            if ( err == -1 )
            {
                err = errno;
            }
            if (err == 0)
            {
                done = true;
            } else if ( err == ENOMEM )
            {
                free( result );
                result = NULL;
                err = 0;
            }
        }
    } while ( err == 0 && !done );

    // Clean up and establish post conditions.
    if ( err != 0 && result != NULL )
    {
        free( result );
        result = NULL;
    }

    *procList = result;
    if ( err == 0 )
    {
        *procCount = length / sizeof( kinfo_proc );
    }

    return err;
}


bool
CocoaThread::isITunesRunning()
{
    const char* processName = "iTunes";
    kinfo_proc* processList = NULL;
    size_t processCount = 0;

    if ( GetBSDProcessList( &processList, &processCount ) )
    {
        qDebug() << "Error getting process list!";
        return false;
    }

    bool found = false;
    for ( size_t processIndex = 0; processIndex < processCount; processIndex++ )
    {
        if ( strcmp( processList[processIndex].kp_proc.p_comm, processName ) == 0 )
        {
//            qDebug() << "Found iTunes:" << processList[processIndex].kp_proc.p_comm;
            found = true;
            break;
        }
    }

    if ( processList != NULL )
        free( processList );

    return found;
}


void
CocoaThread::run()
{
    m_run = true;
    while ( m_run )
    {
        bool ir = isITunesRunning();

        if ( !m_itunesRunning && ir )
        {
            qDebug() << "Detect iTunes starting up!";
            m_itunesRunning = true;
            m_parent->startWithiTunes();
        }
        else if ( m_itunesRunning && !ir )
        {
            qDebug() << "Detect iTunes shutdown!";
            m_itunesRunning = false;
        }

        usleep( 1500000 );
    }
}


#endif
