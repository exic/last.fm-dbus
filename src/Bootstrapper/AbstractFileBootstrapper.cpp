/***************************************************************************
*   Copyright (C) 2005 - 2007 by                                          *
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

#include "AbstractFileBootstrapper.h"
#include "logger.h"
#include "MooseCommon.h"

#include <QFile>

static const int k_maxPlaysPerTrack = 10000;
static const int k_maxTotalPlays = 300000;

static const QString XML_VERSION = "1.0";


AbstractFileBootstrapper::AbstractFileBootstrapper( QString product, QObject* parent )
                         : AbstractBootstrapper( parent ),
                           m_runningPlayCount( 0 )
{
    m_bootstrapElement = m_xmlDoc.createElement( "bootstrap" );
    m_xmlDoc.appendChild( m_bootstrapElement );
    m_bootstrapElement.setAttribute( "product", product );
    m_bootstrapElement.setAttribute( "version", XML_VERSION );

    m_savePath = MooseUtils::savePath( product + "_bootstrap.xml" );
}


AbstractFileBootstrapper::~AbstractFileBootstrapper(void)
{
}


bool
AbstractFileBootstrapper::appendTrack(TrackInfo& track)
{
    m_runningPlayCount += track.playCount();
    if ( track.playCount() > k_maxPlaysPerTrack ||
        m_runningPlayCount > k_maxTotalPlays )
    {
        LOGL( 2, "Playcount for bootstrap exceeded maximum allowed. Track: " <<
            track.playCount() << ", total: " << m_runningPlayCount );

        emit done( Bootstrap_Spam );
        return false;
    }

    QDomElement i = track.toDomElement( m_xmlDoc );
    m_bootstrapElement.appendChild( i );
    return true;
}


void
AbstractFileBootstrapper::zipAndSend()
{
    QFile file( m_savePath );
    file.open( QIODevice::WriteOnly | QIODevice::Text );
    QTextStream stream( &file );

    stream.setCodec( "UTF-8" );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << m_xmlDoc.toString();

    file.close();

    QString zipPath = m_savePath + ".gz";
    zipFiles( m_savePath, zipPath );

    sendZip( zipPath );
}
