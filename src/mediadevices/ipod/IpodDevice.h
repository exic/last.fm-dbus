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

#ifndef IPOD_DEVICE_H
#define IPOD_DEVICE_H

#include "TrackInfo.h"
#include <QSqlDatabase>

typedef struct _Itdb_iTunesDB Itdb_iTunesDB;
typedef struct _Itdb_Track Itdb_Track;
typedef struct _Itdb_Playlist Itdb_Playlist;


/**
 * Currently all MediaDevice plugins are instantiated on startup.
 * In the future it would be better that we only create them as needed
 */
class MyMediaDeviceInterface : public QObject
{
    Q_OBJECT

protected:
    QString m_error;
    QString m_uid;
    QString m_mountPath;

public:
    /** if you need a persistent sqlite store, use this db
      * ensure your table name is unique eg ClassNameTracks */
    QSqlDatabase database() const;

    /** this must be set or you're going to get database table conflicts among
      * other things */
    QString const uniqueId() const { return m_uid; }
    // because we suck currently, can't be in ctor due to QPlugin restriction but must be set
    void setUniqueId( const QString& uid ) { m_uid = uid; }

    /** should be controlled by higher object */
    void setMountPath( const QString& path ) { m_mountPath = path; }

    /** return tracks that should be scrobbled, next time this is called you
      * should return tracks played since then, so you are expected to commit
      * to the database (or whatever storage solution you choose) before you
      * return
      * set m_error if there was an unrecoverable error */
    virtual QList<TrackInfo> tracksToScrobble() = 0;

    /** a suggested unique table name */
    QString tableName() const { return "a" + m_uid; }

    /** the controlling code will check this if you return an empty track list
      * above. If the track list is empty and there is no error it will say
      * "nothing to scrobble", else it will show the error message */
    QString error() const { return m_error; }
};

Q_DECLARE_INTERFACE( MyMediaDeviceInterface, "fm.last.MyMediaDevice/1.0" );


/** NOTE this means a manually synced ipod */
class IpodDevice : public MyMediaDeviceInterface
{
    Q_OBJECT
    Q_INTERFACES( MyMediaDeviceInterface );

public:
    IpodDevice();
    ~IpodDevice();

    virtual QList<TrackInfo> tracksToScrobble();

private:
    QDateTime previousPlayTime( Itdb_Track* ) const;
    uint previousPlayCount( Itdb_Track* ) const;

    /** to the database */
    void commit( const TrackInfo& );

    /** opens the db, allocates a LOT of memory, call after setting m_mountPath */
    void open();

private:
    Itdb_iTunesDB* m_itdb;
    Itdb_Playlist* m_mpl;
};

#endif
