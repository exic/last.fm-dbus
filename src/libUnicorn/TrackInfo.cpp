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

#include "TrackInfo.h"
#include "Settings.h"
#include "StopWatch.h"
#include "logger.h"

#include <QDir>
#include <QMimeData>


TrackInfo::TrackInfo() :
        m_trackNr( 0 ),
        m_playCount( 0 ),
        m_duration( 0 ),
        m_timeStamp( 0 ),
        m_source( Unknown ),
        m_nextPath( 0 ),
        m_stopWatch( 0 ),
        m_ratingFlags( 0 )
{}


TrackInfo::TrackInfo( const Track& that )
{
    *this = TrackInfo();

    setArtist( that.artist() );
    setTrack( that.title() );
    setAlbum( that.album() );
}


TrackInfo::TrackInfo( const QDomElement& e )
{
    setArtist( e.namedItem( "artist" ).toElement().text() );
    setAlbum( e.namedItem( "album" ).toElement().text() );
    setTrack( e.namedItem( "track" ).toElement().text() );
    setTrackNr( 0 );
    setDuration( e.namedItem( "duration" ).toElement().text() );
    setPlayCount( e.namedItem( "playcount" ).toElement().text().toInt() );
    setFileName( e.namedItem( "filename" ).toElement().text() );
    setUniqueID( e.namedItem( "uniqueID" ).toElement().text() );
    setSource( (Source)e.namedItem( "source" ).toElement().text().toInt() );
    setAuthCode( e.namedItem( "authorisationKey" ).toElement().text() );
    m_ratingFlags = e.namedItem( "userActionFlags" ).toElement().text().toUInt();
    m_mediaDeviceId = e.namedItem( "mediaDeviceId" ).toElement().text();

    // this is necessary because the default return for toInt() is 0, and that
    // corresponds to Radio not Unknown :( oops.
    QString const source = e.namedItem( "source" ).toElement().text();
    if (source.isEmpty())
        setSource( Unknown );
    else
        setSource( (Source)source.toInt() );

    // support 1.1.3 stringed timestamps, and 1.3.0 Unix Timestamps
    QString const timestring = e.namedItem( "timestamp" ).toElement().text();
    QDateTime const timestamp = QDateTime::fromString( timestring, "yyyy-MM-dd hh:mm:ss" );
    if (timestamp.isValid())
        setTimeStamp( timestamp.toTime_t() );
    else
        setTimeStamp( timestring.toUInt() );

    setPath( e.namedItem( "path" ).toElement().text() );
    setFpId( e.namedItem( "fpId" ).toElement().text() );
    setMbId( e.namedItem( "mbId" ).toElement().text() );
    setPlayerId( e.namedItem( "playerId" ).toElement().text() );
}


QDomElement
TrackInfo::toDomElement( QDomDocument& document ) const
{
    QDomElement item = document.createElement( "item" );

    #define makeElement( tagname, getter ) \
    { \
        QDomElement e = document.createElement( tagname ); \
        e.appendChild( document.createTextNode( getter ) ); \
        item.appendChild( e ); \
    }

    makeElement( "artist", m_artist );
    makeElement( "album", m_album );
    makeElement( "track", m_track );
    makeElement( "duration", QString::number( m_duration ) );
    makeElement( "timestamp", QString::number( m_timeStamp ) );
    makeElement( "playcount", QString::number( m_playCount ) );
    makeElement( "filename", m_fileName );
    makeElement( "uniqueID", m_uniqueID );
    makeElement( "source", QString::number( m_source ) );
    makeElement( "authorisationKey", m_authCode );
    makeElement( "userActionFlags", QString::number(m_ratingFlags) );
    makeElement( "path", path() );
    makeElement( "fpId", fpId() );
    makeElement( "mbId", mbId() );
    makeElement( "playerId", playerId() );
    makeElement( "mediaDeviceId", m_mediaDeviceId );

    return item;
}


void
TrackInfo::merge( const TrackInfo& that )
{
    m_ratingFlags |= that.m_ratingFlags;

    if ( m_artist.isEmpty() ) setArtist( that.artist() );
    if ( m_track.isEmpty() ) setTrack( that.track() );
    if ( m_trackNr == 0 ) setTrackNr( that.trackNr() );
    // can't do this, we don't know
    //if ( m_playCount == 0 ) setPlayCount( that.playCount() );
    if ( m_duration == 0 ) setDuration( that.duration() );
    if ( m_fileName.isEmpty() ) setFileName( that.fileName() );
    if ( m_mbId.isEmpty() ) setMbId( that.mbId() );
    if ( m_timeStamp == 0 ) setTimeStamp( that.timeStamp() );
    if ( m_source == Unknown ) setSource( that.source() );
    if ( m_authCode.isEmpty() ) setAuthCode( that.authCode() );
    if ( m_uniqueID.isEmpty() ) setUniqueID( that.uniqueID() );
    if ( m_playerId.isEmpty() ) setPlayerId( that.playerId() );
    if ( m_powerPlayLabel.isEmpty() ) setPowerPlayLabel( that.powerPlayLabel() );
    if ( m_paths.isEmpty() ) setPaths( that.m_paths );
    // can't do this
    //if ( m_nextPath.isEmpty() )
    if ( m_stopWatch == 0 ) setStopWatch( that.stopWatch() );
    if ( m_username.isEmpty() ) setUsername( that.username() );
    if ( m_fpId.isEmpty() ) setFpId( that.fpId() );
    if ( m_mediaDeviceId.isEmpty() ) setMediaDeviceId( that.mediaDeviceId() );
}


QString
TrackInfo::toString() const
{
    if ( m_artist.isEmpty() )
    {
        if ( m_track.isEmpty() )
            return QFileInfo( m_fileName ).fileName();
        else
            return m_track;
    }

    if ( m_track.isEmpty() )
        return m_artist;

    return m_artist + ' ' + QChar(8211) /*en dash*/ + ' ' + m_track;
}


QString
TrackInfo::ratingCharacter() const
{
    if (isBanned()) return "B";
    if (isLoved()) return "L";
    if (isScrobbled()) return "";
    if (isSkipped()) return "S";

    return "";
}


bool
TrackInfo::sameAs( const TrackInfo& that ) const
{
    // This isn't an ideal check, but it's the best we can do until we introduce
    // unique IDs for tracks. Since the artist/track info of a track can change
    // after we receive metadata, just comparing on metadata isn't reliable. So
    // we use the paths instead if we have them. And if not, fall back on metadata.

    if ( !this->path().isEmpty() && !that.path().isEmpty() )
        return this->path() == that.path();

    if ( this->artist() != that.artist() )
        return false;

    if ( this->track() != that.track() )
        return false;

    return true;
}


void
TrackInfo::timeStampMe()
{
    setTimeStamp( QDateTime::currentDateTime().toTime_t() );
}


const QString
TrackInfo::path() const
{
    return m_paths.isEmpty() ? "" : m_paths.at( 0 );
}


void
TrackInfo::setPath( QString path )
{
    m_paths.clear();
    m_paths.append( path );
}


const QString
TrackInfo::nextPath() const
{
    if ( m_nextPath < m_paths.size() )
    {
        return m_paths.at( m_nextPath++ );
    }
    else
    {
        return "";
    }
}


void
TrackInfo::setPaths( QStringList paths )
{
    m_paths = paths;
}


QString
TrackInfo::sourceString() const
{
    switch (m_source)
    {
        case Radio: return "L" + authCode();
        case Player: return "P" + playerId();
        case MediaDevice: return "P" + mediaDeviceId();
        default: return "U";
    }
}


QString
TrackInfo::durationString() const
{
    QTime t = QTime().addSecs( m_duration );
    if (m_duration < 60*60)
        return t.toString( "m:ss" );
    else
        return t.toString( "hh:mm:ss" );
}
