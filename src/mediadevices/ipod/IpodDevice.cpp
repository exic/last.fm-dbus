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

#include "IpodDevice.h"

#include "MooseCommon.h"

#include "logger.h"

#include <QApplication>
#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QtPlugin>
#include <QFileDialog>
#include <QSettings>

extern "C"
{
    #include <gpod/itdb.h>
    #include <glib/glist.h>
}

#define TABLE_NAME "IpodDeviceTracks"


IpodDevice::IpodDevice()
    : m_itdb( 0 )
    , m_mpl( 0 )
{}


IpodDevice::~IpodDevice()
{
    if ( m_mpl )
        itdb_playlist_free( m_mpl );

    if ( m_itdb )
        itdb_free( m_itdb );

    database().close();
}


void
IpodDevice::open()
{
    QByteArray _mountpath = QFile::encodeName( m_mountPath ); //must be on stack during use
    const char* mountpath = _mountpath.data();

    m_itdb = itdb_new();
    itdb_set_mountpoint( m_itdb, mountpath );
    m_mpl = itdb_playlist_new( "iPod", false );
    itdb_playlist_set_mpl( m_mpl );

    GError* err = 0;
    m_itdb = itdb_parse( mountpath, &err );

    if ( err )
        throw tr( "The iPod database could not be opened." );

    if ( m_uid.isEmpty() )
    {
        QFileInfo f( m_mountPath + "/iPod_Control/Device" );
        m_uid = f.created().toString( "yyMMdd_hhmmss" );

        qDebug() << "uid" << m_uid;
    }
}


QList<TrackInfo>
IpodDevice::tracksToScrobble()
{
    Q_DEBUG_BLOCK;

    QList<TrackInfo> tracks;

    try
    {
        open();
    }
    catch ( QString& error )
    {
        m_error = error;
        return tracks;
    }

    if ( !m_itdb )
        return tracks;

    GList *cur;
    for ( cur = m_itdb->tracks; cur; cur = cur->next )
    {
        Itdb_Track *track = (Itdb_Track *)cur->data;
        if (!track)
            continue;

        // This requires some recent libgpod!
        if ( track->mediatype != 0 && // old ipods
             track->mediatype != ITDB_MEDIATYPE_AUDIO && track->mediatype != ITDB_MEDIATYPE_MUSICVIDEO )
        {
//             qDebug() << "Skipping, not a music track:" << track->artist << "-" << track->title << "- type:" << track->mediatype;
            continue;
        }

        QDateTime time;
        time.setTime_t( itdb_time_mac_to_host( track->time_played ) );

        if (time.toString() == "Thu Jan 1 01:00:00 1970")
            // the above is prolly same check as isNull() but I don't know
            // why it isn't just checking isNull() so I left it as is --mxcl
            continue;

        //NOTE should we ignore playtime and just go on playcounts?
        QDateTime previousPlayTime = this->previousPlayTime( track );
        if (previousPlayTime >= time)
            continue;

        TrackInfo t;
        t.setArtist( QString::fromUtf8( track->artist ) );
        t.setAlbum( QString::fromUtf8( track->album ) );
        t.setTrack( QString::fromUtf8( track->title ) );
        t.setPath( QString::fromUtf8( track->ipod_path ) );
        t.setTimeStamp( time.toTime_t() );
        t.setDuration( track->tracklen / 1000 );
        t.setPlayCount( track->playcount );
        t.setUniqueID( QString::number( track->id ) );
        t.setSource( TrackInfo::MediaDevice );

        //TODO don't scrobble on first sync, this would be effectively
        // bootstrapping. well maybe we should then?
        t.setPlayCount( t.playCount() - previousPlayCount( track ) );

        if ( t.playCount() < 1 )
        {
            qDebug() << "Warning: iPod's last-played timestamp changed, but play-count did not increase. "
                        "Ignoring track " << t.artist() << "-" << t.track() << "for now, the iPod will "
                        "eventually update its play-counter.";
        }
        else
        {
            tracks += t;
            commit( t );
        }
    }

    return tracks;
}


QDateTime
IpodDevice::previousPlayTime( Itdb_Track* track ) const
{
    QSqlDatabase db = database();
    QSqlQuery query( db );
    QString sql = "SELECT lastplaytime FROM " + tableName() + " WHERE id=" + QString::number( track->id );

    query.exec( sql );
    if ( query.next() )
        return QDateTime::fromTime_t( query.value( 0 ).toUInt() );

    return QDateTime::fromTime_t( 0 );
}


uint
IpodDevice::previousPlayCount( Itdb_Track* track ) const
{
    QSqlDatabase db = database();
    QSqlQuery query( db );
    QString sql = "SELECT playcount FROM " + tableName() + " WHERE id=" + QString::number( track->id );

    query.exec( sql );
    if ( query.next() )
        return query.value( 0 ).toUInt();

    return 0;
}


void
IpodDevice::commit( const TrackInfo& track )
{
    QSqlDatabase db = database();
    QSqlQuery query( db );
    QString sql = "REPLACE INTO " + tableName() + " ( playcount, lastplaytime, id ) VALUES( %1, %2, %3 )";

    if ( !query.exec( sql.arg( track.playCount() ).arg( track.timeStamp() ).arg( track.uniqueID() ) ) )
        qDebug() << query.lastError().text();
}


///////////////////////////////////////////////////////////////////////////////>
QSqlDatabase
MyMediaDeviceInterface::database() const
{
    QString const name = "TrackContents";
    QSqlDatabase db = QSqlDatabase::database( name );

    if (!db.isValid())
    {
        db = QSqlDatabase::addDatabase( "QSQLITE", name );
        db.setDatabaseName( MooseUtils::savePath( QString(metaObject()->className()) + ".db" ) );

        db.open();

        if (!db.tables().contains( tableName() ) )
        {
            QSqlQuery q( db );
            bool b = q.exec( "CREATE TABLE " + tableName() + " ( "
                             "id           INTEGER PRIMARY KEY, "
                             "playcount    INTEGER, "
                             "lastplaytime INTEGER )" );

            if ( !b )
                qWarning() << q.lastError().text();
        }
    }

    return db;
}
///////////////////////////////////////////////////////////////////////////////>


Q_EXPORT_PLUGIN2( mediadevice, IpodDevice );
