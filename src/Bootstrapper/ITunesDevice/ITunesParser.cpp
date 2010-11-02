/***************************************************************************
 *   Copyright 2005 - 2008 Last.fm Ltd.                                    *
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

#include <QXmlContentHandler>
#include <QXmlSimpleReader>


class ITunesParser : public QXmlContentHandler
{
    public:
        ITunesParser()
        {
            m_nextIsKey = false;
        }

        TrackInfo peekTrack()
        {
            if ( m_tracks.size() )
                return m_tracks.at( 0 );
            else
                return TrackInfo();
        }

        TrackInfo takeTrack()
        {
            if ( m_tracks.size() )
                return m_tracks.takeAt( 0 );
            else
                return TrackInfo();
        }

        int trackCount()
        {
            return m_tracks.size();
        }

        bool startElement( const QString & /*namespaceURI*/, const QString & localName, const QString & /*qName*/, const QXmlAttributes & /*atts*/ )
        {
//             qDebug() << "stEl" << localName;

            if ( localName == "key" )
                m_nextIsKey = true;
            else
                m_nextIsKey = false;


            return true;
        }

        bool characters ( const QString & ch )
        {
//             qDebug() << "strings" << ch;

            if ( ch.trimmed().isEmpty() )
                return true;

            if ( m_nextIsKey )
            {
//                 qDebug() << "New Key:" << ch.trimmed();

                m_lastKey = ch.trimmed();
                m_nextIsKey = false;
            }
            else
            {
//                 qDebug() << "New Value:" << ch.trimmed();

                if ( m_lastKey == "Name" )
                    m_track.setTrack( ch.trimmed() );

                if ( m_lastKey == "Artist" )
                    m_track.setArtist( ch.trimmed() );

                if ( m_lastKey == "Album" )
                    m_track.setAlbum( ch.trimmed() );

                if ( m_lastKey == "Total Time" )
                    m_track.setDuration( ch.trimmed().toInt() / 1000 );

                if ( m_lastKey == "Play Count" )
                    m_track.setPlayCount( ch.trimmed().toInt() );

                if ( m_lastKey == "Location" )
                    m_track.setFileName( ch.trimmed() );

                if ( m_lastKey == "Persistent ID" )
                    m_track.setUniqueID( ch.trimmed() );

                if ( m_lastKey == "Play Date UTC" )
                {
                    QDateTime dt = QDateTime::fromString( ch.trimmed(), Qt::ISODate );
                    m_track.setTimeStamp( dt.toTime_t() );
                }
            }

            return true;
        }

        bool endElement ( const QString & /*namespaceURI*/, const QString & localName, const QString & /*qName*/ )
        {
//             qDebug() << "enEl" << localName;

            if ( localName == "dict" )
            {
                if ( !m_track.isEmpty() && m_track.playCount() > 0 )
                {
                    m_track.setSource( TrackInfo::MediaDevice );
                    m_tracks << m_track;
                }

                m_track = TrackInfo();
            }

            return true;
        }

        bool endDocument () { return true; }
        bool endPrefixMapping ( const QString & /*prefix*/ ) { return true; }
        QString errorString () const { return QString(); }
        bool ignorableWhitespace ( const QString & /*ch*/ ) { return true; }
        bool processingInstruction ( const QString & /*target*/, const QString & /*data*/ ) { return true; }
        void setDocumentLocator ( QXmlLocator * /*locator*/ ) {}
        bool skippedEntity ( const QString & /*name*/ ) { return true; }
        bool startDocument () { return true; }
        bool startPrefixMapping ( const QString & /*prefix*/, const QString & /*uri*/ ) { return true; }

    private:
        QString m_lastKey;
        bool m_nextIsKey;

        TrackInfo m_track;
        QList<TrackInfo> m_tracks;
};
