/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
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

#include "MooseCommon.h"

#include "logger.h"
#include "UnicornCommon.h"
#include "LastFmSettings.h"

#include <QApplication>
#include <QDir>
#include <QHostAddress>
#include <QLibrary>
#include <QMap>
#include <QMessageBox>
#include <QPluginLoader>
#include <QProcess>
#include <QTcpSocket>

#ifdef WIN32
    #include <windows.h>
    #include <shlobj.h>
#endif


namespace MooseUtils
{


QString
dataPath( QString file )
{
    return QApplication::applicationDirPath() + "/data/" + file;
}


QString
savePath( QString file )
{
    QString path;

    #ifdef WIN32
        path = UnicornUtils::appDataPath();
        if ( path.isEmpty() )
            path = QApplication::applicationDirPath();
        else
            path += "/Last.fm/Client";
    #else
        path = UnicornUtils::appDataPath() + "/Last.fm";
    #endif

    QDir d( path );
    d.mkpath( path );

    return d.filePath( file );
}


QString
logPath( QString file )
{
    #ifndef Q_WS_MAC
        return savePath( file );
    #else
        return QDir( QDir::homePath() + "/Library/Logs" ).filePath( file );
    #endif

}


QString
cachePath()
{
    #ifdef Q_WS_MAC
        QString appSupportFolder = UnicornUtils::applicationSupportFolderPath();
        QDir cacheDir( appSupportFolder + "/Last.fm/Cache" );

        if (!cacheDir.exists())
        {
            cacheDir.mkpath( appSupportFolder + "/Last.fm/Cache" );
        }

        return cacheDir.path() + "/";
    #else
        return savePath( "cache/" );
    #endif
}


QString
servicePath( QString name )
{
    QString dirPath;
    #ifdef WIN32
        // Hack to get it working with VS2005
        dirPath = qApp->applicationDirPath();
    #else
        dirPath = qApp->applicationDirPath() + "/services";
    #endif

    #ifndef QT_NO_DEBUG
        dirPath += "/debug";
    #endif

    if ( name.isEmpty() )
    {
        return dirPath;
    }

    #ifndef QT_NO_DEBUG
        name += DEBUG_SUFFIX;
    #endif

    QDir servicesDir( dirPath );
    QString fileName = SERVICE_PREFIX + name + LIB_EXTENSION;

    return servicesDir.absoluteFilePath( fileName );
}


// This is the kind of beastly function that Max will throw a hissy fit if he can't have.
static void
loadServiceError( QString name )
{
    QMessageBox::critical( 0,
        QCoreApplication::translate( "Container", "Error" ),
        QCoreApplication::translate( "Container", "Couldn't load service: %1. The application won't be able to start." ).arg( name ) );

    QCoreApplication::exit( 1 );
}


QObject*
loadService( QString name )
{
    QString path = servicePath( name );

    qDebug() << "Loading service: " << name << "at" << path;

    QObject* plugin = QPluginLoader( path ).instance();

    if ( plugin == NULL )
    {
        loadServiceError( name );
        return NULL;
    }

    return plugin;
}


QIcon
icon( const char *name )
{
    return QIcon( MooseUtils::dataPath( QString("icons/") + name + ".png" ) );
}


MooseEnums::ScrobblableStatus
scrobblableStatus( TrackInfo& track )
{
    using namespace MooseEnums;

    // Check duration
    if ( track.duration() < MooseConstants::kScrobbleMinLength )
    {
        LOG( 3, "Track length is " << track.duration() << " s which is too short, will not submit.\n" );
        return TooShort;
    }

    // Radio tracks above preview length always scrobble
    if ( track.source() == TrackInfo::Radio )
    {
        return OkToScrobble;
    }

    // Check timestamp
    if ( track.timeStamp() == 0 )
    {
        LOG( 3, "Track has no timestamp, will not submit.\n" );
        return NoTimeStamp;
    }
    
    // actual spam prevention is something like 12 hours, but we are only
    // trying to weed out obviously bad data, server side criteria for
    // "the future" may change, so we should let the server decide, not us
    if ( track.timeStamp() > QDateTime::currentDateTime().addMonths( 1 ).toTime_t() )
    {
        LOGL( 3, "Track is more than a month in the future, will not submit" );
        return FromTheFuture;
    }
    
    if ( track.timeStamp() < QDateTime::fromString( "2003-01-01", Qt::ISODate ).toTime_t() )
    {
        LOGL( 3, "Track was played before the Audioscrobbler project was founded! Will not submit" );
        return FromTheDistantPast;
    }

    // Check if any required fields are empty
    if ( track.artist().isEmpty() )
    {
        LOG( 3, "Artist was missing, will not submit.\n" );
        return ArtistNameMissing;
    }
    if ( track.track().isEmpty() )
    {
        LOG( 3, "Artist, track or duration was missing, will not submit.\n" );
        return TrackNameMissing;
    }

    // Check if dir excluded
    if ( isDirExcluded( track.path() ) )
    {
        LOG( 3, "Track is in excluded directory `" << track.path() << "', " << "will not submit.\n" );
        return ExcludedDir;
    }

    QStringList invalidList;
    invalidList << "unknown artist"
                << "unknown"
                << "[unknown]"
                << "[unknown artist]";

    // Check if artist name is an invalid one like "unknown"
    foreach( QString invalid, invalidList )
    {
        if ( track.artist().toLower() == invalid )
        {
            LOG( 3, "Artist '" << track.artist() << "' is an invalid artist name, will not submit.\n" );
            return ArtistInvalid;
        }
    }

    // All tests passed!
    return OkToScrobble;
}


bool
isDirExcluded( const QString& path )
{
    QString pathToTest = QDir( path ).absolutePath();
    #ifdef WIN32
        pathToTest = pathToTest.toLower();
    #endif

    if (pathToTest.isEmpty())
        return false;

    foreach ( QString bannedPath, The::currentUser().excludedDirs() )
    {
        bannedPath = QDir( bannedPath ).absolutePath();
        #ifdef WIN32
            bannedPath = bannedPath.toLower();
        #endif

        // Try and match start of given path with banned dir
        if ( pathToTest.startsWith( bannedPath ) )
        {
            // Found, this path is from a banned dir
            return true;
        }
    }

    // The path wasn't found in exclusions list
    return false;
}


int
scrobbleTime( TrackInfo& track )
{
    // If we don't have a length or it's less than the minimum, return the
    // threshold
    if ( track.duration() <= 0 || track.duration() < MooseConstants::kScrobbleMinLength )
        return MooseConstants::kScrobbleTimeMax;

    float scrobPoint = qBound( MooseConstants::kScrobblePointMin,
                               The::currentUser().scrobblePoint(),
                               MooseConstants::kScrobblePointMax );
    scrobPoint /= 100.0f;

    return qMin( MooseConstants::kScrobbleTimeMax, int( track.duration() * scrobPoint ) );
}


bool
isAlreadyRunning()
{
  #ifdef WIN32
    QString id( "Lastfm-F396D8C8-9595-4f48-A319-48DCB827AD8F" );
    ::CreateMutexA( NULL, false, id.toAscii() );

    // The call fails with ERROR_ACCESS_DENIED if the Mutex was 
    // created in a different users session because of passing
    // NULL for the SECURITY_ATTRIBUTES on Mutex creation);
    return ::GetLastError() == ERROR_ALREADY_EXISTS || ::GetLastError() == ERROR_ACCESS_DENIED;
  #else
    return sendToInstance( "", MooseEnums::DontStartNewInstance );
  #endif
}


bool
sendToInstance( const QString& data, MooseEnums::StartNewInstanceBehaviour behaviour )
{
    LOGL( 3, "sendToInstance (new instance): " << data );

    QTcpSocket socket;
    socket.connectToHost( QHostAddress::LocalHost, The::settings().controlPort() );

    if ( socket.waitForConnected( 500 ) )
    {   
        if ( data.length() > 0 )
        {
            QByteArray utf8Data = data.toUtf8();
            socket.write( utf8Data, utf8Data.length() );
            socket.flush();
        }

        socket.close();
        return true;
    }
    else if ( behaviour == MooseEnums::StartNewInstance )
    {
        qDebug() << "Starting instance" << The::settings().path();
        //FIXME doesn't work for some reason
        return QProcess::startDetached( The::settings().path(), QStringList() << data );
    }

    return false;
}


QStringList
extensionPaths()
{
    QStringList paths;
    
  #ifdef WIN32
    // Hack to get it working with VS2005
    QString path = qApp->applicationDirPath();
  #else
    QString path = qApp->applicationDirPath() + "/extensions";
  #endif

  #ifndef QT_NO_DEBUG
    path += "/debug";
  #endif

    QDir d( path );
    foreach( QString fileName, d.entryList( QDir::Files ) )
    {
        if (!fileName.startsWith( EXTENSION_PREFIX ))
            continue;
        QString const path = d.absoluteFilePath( fileName );
        if (!QLibrary::isLibrary( path ))
            continue;
        paths += path;
    }

    return paths;
}


#ifdef WIN32

static QString
helperStartupShortcutPath()
{
    wchar_t acPath[MAX_PATH];
    HRESULT h = SHGetFolderPathW( NULL, CSIDL_STARTUP | CSIDL_FLAG_CREATE,
                                  NULL, 0, acPath );

    if ( h == S_OK )
    {
        QString shortcutPath;
        shortcutPath = QString::fromUtf16( reinterpret_cast<const ushort*>( acPath ) );
        if ( !shortcutPath.endsWith( "\\" ) && !shortcutPath.endsWith( "/" ) )
            shortcutPath += "\\";
        shortcutPath += "Last.fm Helper.lnk";
        return shortcutPath;
    }
    else
    {
        LOGL( 1, "Uh oh, Windows returned no STARTUP path, not going to be able to disable helper" );
        return "";
    }
}


void
disableHelperApp()
{
    // Delete the shortcut from startup and kill process
    QString shortcutPath = helperStartupShortcutPath();
    if ( !shortcutPath.isEmpty() && QFile::exists( shortcutPath ) )
    {
        bool fine = QFile::remove( shortcutPath );
        if ( !fine )
        {
            LOGL( 1, "Deletion of shortcut failed, helper will still autolaunch" );
        }
        else
        {
            LOGL( 3, "Helper shortcut removed from Startup" );
        }
    }
}

#endif // WIN32


#ifdef Q_OS_MAC
QString bundleDirPath()
{
    return QDir( qApp->applicationDirPath() ).filePath( "../.." );
}
#endif

} // namespace MooseUtils
