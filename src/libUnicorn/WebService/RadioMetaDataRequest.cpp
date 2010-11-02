/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Max Howell, Last.fm Ltd <max@last.fm>                              *
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

#include "last.fm.h"
#include "Request.h"
#include "XmlRpc.h"


RadioMetaDataRequest::RadioMetaDataRequest()
        : Request( TypeRadioMetaData, "RadioMetaData" )
        , m_retry_timeout( 0 )
{
    m_retry_timer.setSingleShot( true );
    
    connect( &m_retry_timer, SIGNAL(timeout()), SLOT(start()) );
}

void
RadioMetaDataRequest::start()
{
    get( basePath() + "/np.php?session=" + session() );
}

void
RadioMetaDataRequest::success( QByteArray data )
{
    MetaData track;
    QString stationName;

    if ( data.size() <= 0 )
    {
        LOGL( 3, "Get metadata for radio failed" );
        m_track.populate( The::currentMetaData() );
    }
    else {
        QString result( data );
        //LOGL( 4, "result: " << result );

        track.setArtist( parameter( "artist", result ) );
        track.setAlbum( parameter( "album", result ) );
        track.setTrack( parameter( "track", result ) );
        track.setAlbumPicUrl( parameter( "albumcover_medium", result ) );
        track.setArtistPageUrl( parameter( "artist_url", result ) );
        track.setAlbumPageUrl( parameter( "album_url", result ) );
        track.setTrackPageUrl( parameter( "track_url", result ) );
        track.setDuration( parameter( "trackduration", result ).toInt() );
        track.setSource( MetaData::Radio );

        int errCode = parameter( "error", result ).toInt(); 
        bool discovery = parameter( "discovery", result ) != "-1";
        m_stationFeed = parameter( "stationfeed", result );
        
        stationName = parameter( "station", result );
        
        Q_UNUSED( errCode );
        Q_UNUSED( discovery );
    }

    if (track.sameAs( The::currentMetaData() ))
    {
        LOGL( 3, "Re-requesting metadata" );
        
        tryAgain();
    }
    else
    {
        LOGL(3, "got new track, populating The::webService()->currentTrack()" );
        
        m_stationName = stationName;
        m_track.populate( track );
    }
}
