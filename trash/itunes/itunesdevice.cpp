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

#include <QCoreApplication>
#include "itunesdevice.h"
#include "MooseCommon.h"

#ifdef Q_WS_MAC
    #include <sys/param.h>
    #include <sys/mount.h>
#endif

#ifdef WIN32
    #include "windows.h"
    #include "shfolder.h"
#endif

#include "logger.h"
#include "LastFmSettings.h"

#define ESC( token ) QString( token ).replace( "\'", "''" )
const QString ITEM( "dict" ); //so we don't construct these QStrings all the time
const QString KEY( "key" ); //so we don't construct these QStrings all the time


QString
ITunesDevice::LibraryPath()
{
    if ( !m_iTunesLibraryPath.isEmpty() )
        return m_iTunesLibraryPath;

    QString path;
    QString confPath;

#ifdef Q_WS_MAC
    QSettings ist( "apple.com", "iTunes" );
    path = ist.value( "AppleNavServices:ChooseObject:0:Path" ).toString();
    path = path.remove( "file://localhost" );
    qDebug() << "Found iTunes Library in:" << path;

    QFileInfo fi( path + "iTunes Music Library.xml" );
    if ( fi.exists() )
        m_iTunesLibraryPath = fi.absoluteFilePath();
    else
        m_iTunesLibraryPath = QFileInfo( QDir::homePath() + "/Music/iTunes/iTunes Music Library.xml" ).absoluteFilePath();

    return m_iTunesLibraryPath;
#endif

#ifdef WIN32
    {
        // Get path to My Music
        char acPath[MAX_PATH];
        HRESULT h = SHGetFolderPathA( NULL, CSIDL_MYMUSIC,
                                      NULL, 0, acPath );

        if ( h == S_OK )
            path = QString::fromLocal8Bit( acPath );
        else
            LOG( 1, "Couldn't get My Music path\n" );

        qDebug() << "CSIDL_MYMUSIC path: " << path;
    }

    {
        // Get path to Local App Data
        char acPath[MAX_PATH];
        HRESULT h = SHGetFolderPathA( NULL, CSIDL_LOCAL_APPDATA,
                                      NULL, 0, acPath );

        if ( h == S_OK )
            confPath = QString::fromLocal8Bit( acPath );
        else
            LOG( 1, "Couldn't get Local Application Data path\n" );

        qDebug() << "CSIDL_LOCAL_APPDATA path: " << confPath;
    }

    // Try reading iTunesPrefs.xml for custom library path
    QFile f( confPath + "/Apple Computer/iTunes/iTunesPrefs.xml" );
    if ( f.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        qDebug() << "Found iTunesPrefs.xml";

        QByteArray content = f.readAll();

        int tagStart = content.indexOf( "iTunes Library XML Location:1" );

        if ( tagStart != -1 )
        {
            // TODO: this could fail if the XML is broken
            int dataTagStart = content.indexOf( "<data>", tagStart );
            int dataTagEnd = dataTagStart + 6;
            int dataEndTagStart = content.indexOf( "</data>", dataTagStart );
            QByteArray lp = content.mid( dataTagEnd, dataEndTagStart - dataTagEnd );

            qDebug() << "lp before trim: " << lp;

            // The file contains whitespace and linebreaks in the middle of
            // the data so need to squeeze all that out
            lp = lp.simplified();
            lp = lp.replace( ' ', "" );

            qDebug() << "lp after simplified: " << lp;

            lp = QByteArray::fromBase64( lp );

            qDebug() << "lp after base64: " << lp;

            QString sp = QString::fromUtf16( (ushort*)lp.data() );

            qDebug() << "Found iTunes Library path (after conversion to QString):" << sp;

            QFileInfo fi( sp );
            if ( fi.exists() )
            {
                qDebug() << "file exists, returning: " << fi.absoluteFilePath();
                m_iTunesLibraryPath = fi.absoluteFilePath();
                return m_iTunesLibraryPath;
            }
        }
        else
        {
            qDebug() << "No custom library location found in iTunesPrefs.xml";
        }
    }

    // Fall back to default path otherwise
    m_iTunesLibraryPath = path + "/iTunes/iTunes Music Library.xml";

    qDebug() << "Will use default iTunes Library path: " << m_iTunesLibraryPath;

    return m_iTunesLibraryPath;

#endif

    // Fallback for testing
    m_iTunesLibraryPath = "/tmp/iTunes Music Library.xml";
    return m_iTunesLibraryPath;
}


ITunesDevice::ITunesDevice() :
    m_file( 0 ),
    m_ipodJustConnected( false ),
    m_fs( 0 )
{
    qDebug() << "Initialising iTunes MediaDevice";

    m_copyPath = MooseUtils::savePath( "iTunesLibraryCopy.xml" );
    m_diffPath = MooseUtils::savePath( "iTunesLibraryAfterIpod.xml" );
}


void
ITunesDevice::setupWatchers()
{
    QFileInfo fi( LibraryPath() );
    fi.setCaching( false );
    if ( !fi.exists() )
    {
        qDebug() << "The library path doesn't exist. Adding it to watcher failed.";
        return;
    }

    m_fs = new QFileSystemWatcher( this );
    m_fs->addPath( LibraryPath() );

    #ifdef Q_WS_MAC
    m_fs->addPath( "/Volumes" );

    if ( !QDir( "/Volumes" ).exists() )
        qDebug() << "The /Volumes path doesn't exist. Adding it to watcher failed.";
    #endif

    connect( m_fs, SIGNAL( fileChanged( QString ) ),
             this,   SLOT( libraryChanged() ) );
    connect( m_fs, SIGNAL( directoryChanged( QString ) ),
             this,   SLOT( checkDevices() ) );
}


void
ITunesDevice::initDatabase()
{
    m_db = QSqlDatabase::database( "mediadevice" );
    if ( !m_db.isValid() )
    {
        m_db = QSqlDatabase::addDatabase( "QSQLITE", "mediadevice" );
        m_db.setDatabaseName( MooseUtils::savePath( "mediadevice.db" ) );
    }
    m_db.open();

    qDebug() << "Opening DB" << ( m_db.isValid() ? "worked" : "failed" );
    if ( !m_db.isValid() )
        QCoreApplication::quit();

    if ( !m_db.tables().contains( "mediadevice" ) )
    {
        qDebug() << "Creating iTunes-mediadevice database!";

        QSqlQuery query( m_db );
        query.exec( "CREATE TABLE mediadevice ( "
                        "id         INTEGER PRIMARY KEY, "
                        "artist     VARCHAR( 255 ), "
                        "album      VARCHAR( 255 ), "
                        "track      VARCHAR( 255 ), "
                        "filename   VARCHAR( 512 ), "
                        "uniqueID   VARCHAR( 32 ), "
                        "duration   INTEGER, "
                        "timestamp  INTEGER, "
                        "playcount  INTEGER )" );

        query.exec( "CREATE INDEX id_idx ON mediadevice( id )" );
        query.exec( "CREATE INDEX filename_idx ON mediadevice( filename )" );
        query.exec( "CREATE INDEX uniqueID_idx ON mediadevice( uniqueID )" );
    }
}


void
ITunesDevice::importDatabase( const QString& file )
{
    qDebug() << "ITunesDevice::importDatabase start, " << QDateTime::currentDateTime().toUTC().time().toString();

    QFile::remove( MooseUtils::savePath( "mediadevice.db" ) );
    initDatabase();
    TrackInfo track = firstTrack( file );

    m_db.transaction();
    while ( ( m_file && !m_file->atEnd() ) || !track.isEmpty() )
    {
        if ( !track.isEmpty() )
            addTrack( track );

        track = nextTrack();
    }
    m_db.commit();

    qDebug() << "ITunesDevice::importDatabase end, " << QDateTime::currentDateTime().toUTC().time().toString();
}


void
ITunesDevice::diffDatabase( const QString& file, QHash<QString, TrackInfo> iTunesHistory )
{
    qDebug() << "ITunesDevice::diffDatabase start, " << QDateTime::currentDateTime().toUTC().time().toString();
    TrackInfo track = firstTrack( file );

//     m_db.transaction();
    while ( ( m_file && !m_file->atEnd() ) || !track.isEmpty() )
    {
        if ( !track.isEmpty() )
            updateTrack( track, iTunesHistory );

        track = nextTrack();
    }
//     m_db.commit();

    qDebug() << "ITunesDevice::diffDatabase end, " << QDateTime::currentDateTime().toUTC().time().toString();
}


TrackInfo
ITunesDevice::firstTrack( const QString& file )
{
    m_database = file;

    if ( !m_file )
    {
        m_file = new QFile( file );
        if ( !m_file->open( QIODevice::ReadOnly | QIODevice::Text ) )
        {
            qDebug() << "Could not open iTunes Library" << m_database;
            return TrackInfo();
        }

        m_totalSize = m_file->size();
        m_xmlReader = new QXmlSimpleReader();
        m_xmlInput = new QXmlInputSource();

        m_handler = new ITunesParser();
        m_xmlReader->setContentHandler( m_handler );

        m_xmlInput->setData( m_file->read( 32768 ) );
        if ( !m_xmlReader->parse( m_xmlInput, true ) )
        {
            qDebug() << "Couldn't read file: " << m_database;
            return TrackInfo();
        }
    }

    return nextTrack();
}


TrackInfo
ITunesDevice::nextTrack()
{
    if ( m_handler->trackCount() < 20 && !m_file->atEnd() )
    {
        m_xmlInput->setData( m_file->read( 32768 ) );
        m_xmlReader->parseContinue();
    }

    TrackInfo t = m_handler->takeTrack();
    if ( !t.isEmpty() )
    {
        emit progress( (float)( (float)m_file->pos() / (float)m_file->size() ) * 100.0, t );
        return t;
    }

    if ( m_file->atEnd() )
    {
        // Finished with the database, let's close our stuff
        qDebug() << "Finished reading";

        m_file->close();
        delete m_file;
        m_file = 0;
    }

    return TrackInfo();
}


void
ITunesDevice::addTrack( TrackInfo track )
{
    //qDebug() << "Adding track to database:" << track.artist() << track.album() << track.track() << track.playCount();
    QSqlQuery query( m_db );

    QDateTime dt;
    dt.fromTime_t( track.timeStamp() );

    QString q = QString( "INSERT INTO mediadevice ( artist, album, track, duration, timestamp, playcount, filename, uniqueID ) VALUES ("
                         "'" + ESC( track.artist() ) + "',"
                         "'" + ESC( track.album() ) + "',"
                         "'" + ESC( track.track() ) + "',"
                         "" + QString::number( track.duration() ) + ","
                         "'" + ESC( dt.toString( Qt::ISODate ) ) + "',"
                         "" + QString::number( track.playCount() ) + ","
                         "'" + ESC( track.fileName() ) + "',"
                         "'" + ESC( track.uniqueID() ) + "')" );

    query.exec( q );
    if ( !query.lastError().databaseText().isEmpty() )
        qDebug() << "SQL Error:" << query.lastError().text() << query.lastError().databaseText();
}


void
ITunesDevice::updateTrack( TrackInfo track, QHash<QString, TrackInfo> iTunesHistory )
{
    QSqlQuery query( m_db );
    query.exec( QString( "SELECT playcount FROM mediadevice WHERE "
                         "uniqueID = '%1' ORDER BY playcount DESC" )
                         .arg( ESC( track.uniqueID() ) ) );

    int dbPlayCount = !query.first() ? 0 : query.value( 0 ).toInt();

    if ( track.playCount() > dbPlayCount )
    {
        // Now we must check the iTunes history to see if we might have already
        // scrobbled this track through the normal channels. This is because
        // iTunes doesn't always update its library XML immediately so we might get
        // some normal iTunes plays included in the diff.
        TrackInfo alreadyScrobbled = iTunesHistory.value( track.toString(), TrackInfo() );
        int alreadyScrobbledCount = alreadyScrobbled.isEmpty() ? 0 : alreadyScrobbled.playCount();

        if ( alreadyScrobbledCount > 0 )
            qDebug() << "Already scrobbled iTunes count found: " << alreadyScrobbledCount;

        int iPodScrobbleCount = track.playCount() - dbPlayCount - alreadyScrobbledCount;

        track.setPlayCount( iPodScrobbleCount );

        emit trackChanged( track, iPodScrobbleCount );
    }

}


void
ITunesDevice::libraryChanged()
{
    QMutexLocker locker( &m_mutex );

    int sanity = 0;
    while ( !QFileInfo( LibraryPath() ).exists() )
    {
        #ifdef WIN32
            Sleep( 1000 );
        #else
            sleep( 10 );
        #endif

        if ( ++sanity == 20 )
        {
            qDebug() << "Sanity skip in libraryChanged()!";
            return;
        }
    }

    Q_ASSERT( m_fs != 0 );
    m_fs->addPath( LibraryPath() );

    // fake for linux
    //m_ipodJustConnected = true;

    if ( m_ipodJustConnected && QFile::exists( m_copyPath ) )
    {
        // Ok, library changed due to ipod interaction.
        qDebug() << "iPod just got connected, the updated iTunes database will be analyzed!";

        QFileInfo fileInfo( m_copyPath );
        m_lastItunesUpdateTime = fileInfo.lastModified();

        qDebug() << "Will diff against old copy with timestamp (UTC): " << m_lastItunesUpdateTime.toUTC();

        qDebug() << "Now: " << QDateTime::currentDateTime().toUTC();

        QString actualItunesLib = LibraryPath();
        QFileInfo fileInfo2( actualItunesLib );
        QDateTime newItunesUpdateTime = fileInfo2.lastModified();

        emit deviceChangeStart( m_uid, newItunesUpdateTime );
        QFile::remove( m_diffPath );
        QFile::copy( LibraryPath(), m_diffPath );
        QFile::copy( actualItunesLib, m_diffPath );

        QHash<QString, TrackInfo> iTunesHistory = readItunesScrobbleHistory( m_lastItunesUpdateTime );

        qDebug() << "iTunesHistory count: " << iTunesHistory.count();

        importDatabase( m_copyPath );
        diffDatabase( m_diffPath, iTunesHistory );

        QFile::remove( m_copyPath );
        QFile::copy( m_diffPath, m_copyPath );
        emit deviceChangeEnd( m_uid );

        m_lastItunesUpdateTime = QDateTime();
    }
    else
    {
        // Ok, normal change in itunes database, no ipod interaction apparently.
        qDebug() << "Normal iTunes database update, will create a copy for diffing now!";

        QFile::remove( m_copyPath );
        QFile::copy( LibraryPath(), m_copyPath );
    }

    m_ipodJustConnected = false;
}


// Checks the path given to see if the thing there is an iPod
void
ITunesDevice::checkDevices()
{
    QMutexLocker locker( &m_mutex );

    qDebug() << "Rescanning devices";
    QStringList availUIDs;

    // For Mac, go through everything in /Volumes
    #ifdef Q_WS_MAC
    m_path = QDir( "/Volumes" );
    QStringList l = m_path.entryList( QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks );

    qDebug() << "All volumes:" << l;

    for( int x = 0; x < l.count(); x++ )
    {
        QString cdir = "/" + m_path.dirName() + "/" + l.at( x );
        if ( QFileInfo( cdir ).isHidden() )
            continue;

        qDebug() << "Trying:" << cdir;
        qDebug() << "Permissions:" << QFileInfo( cdir ).permissions();
        qDebug() << "Hidden:" << QFileInfo( cdir ).isHidden();
        qDebug() << "Created:" << QFileInfo( cdir ).created();

        struct statfs stat_buf;
        const char rfs[5][8] = { "afp","ftp","nfs","smbfs","webdav" };

        if ( statfs( cdir.toUtf8(), &stat_buf ) )
        {
            qDebug() << "Couldn't stat:" << cdir;
            continue;
        }

        bool networkDrive = false;
        int i = 0;
        while ( *rfs[i] )
        {
            if ( !strncmp( stat_buf.f_fstypename, rfs[i], sizeof( rfs[i] ) ) )
            {
                networkDrive = true;
                break;
            }
            i++;
        }

        if ( networkDrive )
        {
            qDebug() << "This is a bloody network device, skipping:" << cdir;
            continue;
        }

        int sanity = 0;
        QDir qcd( cdir );
        while ( QFile::exists( cdir + "/.autodiskmounted" ) || qcd.entryList().count() < 2 )
        {
            qcd.refresh();
            qDebug() << "On disk:" << qcd.entryList();
            if ( ++sanity == 150 )
            {
                qDebug() << "Sanity skip!";
                break;
            }

            usleep( 50000 );
        }
        qDebug() << "On disk after sanity check:" << sanity << qcd.entryList();
        qDebug() << qcd.absolutePath() << cdir;

        QFileInfo sdi( cdir + "/iPod_Control/Device" );
        QFile file( cdir + "/iPod_Control/Device/SysInfo" );
        qDebug() << "Does control exist?" << QDir( cdir + "/iPod_Control" ).exists();
        qDebug() << "Does device exist?" << QDir( cdir + "/iPod_Control/Device" ).exists();
    #else
        QFileInfo sdi( m_path.absolutePath() + "/iPod_Control/Device" );
        QFile file( m_path.absolutePath() + "/iPod_Control/Device/SysInfo" );
    #endif

        qDebug() << "Try opening" << file.fileName();
        qDebug() << "Does exist?" << file.exists();

    #ifdef Q_WS_MAC
        sleep( 500000 );
    #endif

        qDebug() << "Does exist?" << file.exists();

        if( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
        {
            QString uid;
            qDebug() << "Opening worked" << file.fileName();

            while ( file.bytesAvailable() )
            {
                QString s = QString( file.readLine() );
//                qDebug() << "Read:" << s;
                if ( s.contains( "pszSerialNumber:" ) )
                {
                    QStringList tokens = s.split( " " );
                    uid = tokens.at( 1 ).trimmed();
                }
            }

            if ( uid.isEmpty() )
            {
                uid = sdi.created().toString( "yyMMdd_hhmmss" );
//                 uid = "serial";
            }

            availUIDs << uid;

            //HACK
        #ifdef WIN32
            QDir qcd = m_path;
        #endif
            QSettings().setValue( "devicePaths/" + uid + "/path", qcd.absolutePath() );
            //HACK
        }
        else
        {
//            qDebug() << file.error();
        }

    #ifdef Q_WS_MAC
    }
    #endif

    for ( int x = 0; x < availUIDs.count(); x++ )
    {
        if ( !QFile::exists( m_copyPath ) )
            QFile::copy( LibraryPath(), m_copyPath );

        m_uid = "iPod_" + availUIDs.at( x );
        qDebug() << "user:" << The::settings().mediaDeviceUser( m_uid );

        // we don't do auto sync stuff for manual ipods either
        if ( !The::settings().mediaDeviceUser( m_uid ).isEmpty() && !The::settings().isManualIpod( m_uid ) )
        {
            qDebug() << "A known iPod has been detected!";
            m_ipodJustConnected = true;
        }

        qDebug() << "Found device" << availUIDs.at( x );
        emit deviceAdded( m_uid );
    }

    qDebug() << "Rescanning devices finished";
}


// Called when a new device is plugged in
void
ITunesDevice::forceDetection( const QString& path )
{
    #ifdef WIN32
        m_path = QDir( path );
    #endif
    #ifdef Q_WS_MAC
        m_path = QDir( "/Volumes" );
    #endif

    Q_UNUSED( path )

    checkDevices();
}


QHash<QString, TrackInfo>
ITunesDevice::readItunesScrobbleHistory( QDateTime afterThisTime )
{
    // FIXME: code duplicated from media device watcher readQueue

    Q_ASSERT( !m_uid.isEmpty() );

    QString user = The::settings().mediaDeviceUser( m_uid );

    QString path = MooseUtils::savePath( "iTunesScrobbleHistory.xml" );

    QHash<QString, TrackInfo> queue;

    QFile file( path );
    if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        qDebug() << "Could not open iTunes history file: " << path <<
                    "\nDefinite risk for duplicate scrobbles!";
        return queue;
    }

    QTextStream stream( &file );
    stream.setCodec( "UTF-8" );

    QDomDocument d;
    QString contents = stream.readAll();

    if ( !d.setContent( contents ) )
    {
        qDebug() << "Couldn't parse file: " << path <<
                    "\nDefinite risk for duplicate scrobbles!";
        return queue;
    }

    const QString ITEM( "item" ); //so we don't construct these QStrings all the time

    time_t afterThisTime_t = afterThisTime.toTime_t();
    for( QDomNode n = d.namedItem( "submissions" ).firstChild(); !n.isNull() && n.nodeName() == ITEM; n = n.nextSibling() )
    {
        TrackInfo t( n.toElement() );
        if (t.timeStamp() > afterThisTime_t)
        {
            queue[t.toString()] = t;
        }
    }

    return queue;
}

Q_EXPORT_PLUGIN2( mediadevice, ITunesDevice )
