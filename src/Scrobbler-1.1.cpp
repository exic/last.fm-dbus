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

#include <QtXml>
#include <QDateTime>

#include <containerutils.h>
#include "Scrobbler-1.1.h"
#include "logger.h"

QString ScrobblerSubmitter::XML_VERSION = "1.0";
QString ScrobblerSubmitter::PROTOCOL_VERSION = "1.1";
QString ScrobblerSubmitter::CLIENT_ID = "ass";
QString ScrobblerSubmitter::CLIENT_VERSION; // will be set in code
QString ScrobblerSubmitter::HANDSHAKE_HOST = "post.audioscrobbler.com";

ScrobblerSubmitter::ScrobblerSubmitter( QObject *parent )
        : QObject( parent ),
		  m_submitID( -1 )
{
    connect( &m_http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( handshakeFinished( int, bool ) ) );
    connect( &m_submitHttp, SIGNAL( requestFinished( int, bool ) ), this, SLOT( submitFinished( int, bool ) ) );
    connect( &m_timer, SIGNAL(timeout()), this, SLOT(scheduledTimeReached()) );
    connect( this, SIGNAL( submitThreadHandOver() ), this, SLOT( submitItemInThread() ), Qt::QueuedConnection );
}


void
ScrobblerSubmitter::init( const QString& username, const QString& password, const QString& version )
{
    qDebug() << "scrobbler::init";

    m_submitQueue.clear();
    m_progressQueue.clear();

    LOGL( 3, "Initialising scrobbler for " << username );

    m_savePath = MooseUtils::savePath( username + "_submissions.xml" );
    m_username = username;
    m_password = password;
    m_challenge = "";
    m_submitUrl = 0;
    m_inProgress = false;
    m_needHandshake = true;
    m_prevSubmitTime = 0;
    m_lastSubmissionFinishTime = 0;
    m_interval = 0;

    CLIENT_VERSION = version;

    m_timer.setSingleShot( true );
    
	readSubmitQueue( m_savePath );
    readSubmitQueue( MooseUtils::savePath( username + "_mediadevice.xml" ), TrackInfo::MediaDevice );
    schedule( false );
}


ScrobblerSubmitter::~ScrobblerSubmitter()
{
    m_http.abort();
    m_submitHttp.abort();

    if ( !m_savePath.isEmpty() )
    {
        saveSubmitQueue();
    }
    else
    {
        // We have not been initialised
    }
}


/**
 * Sets item for submission to Audioscrobbler. Actual submission
 * depends on things like (is scrobbling enabled, are Audioscrobbler
 * profile details filled in etc).
 */
void
ScrobblerSubmitter::submitItem( TrackInfo item )
{
	Q_DEBUG_BLOCK
    
	m_submitQueue << item;

    // Thread handover
    emit submitThreadHandOver();
}


void
ScrobblerSubmitter::submitItemInThread()
{
	Q_DEBUG_BLOCK

    saveSubmitQueue();
    schedule( false );
}


/**
 * Performs handshake with Audioscrobbler.
 */
void
ScrobblerSubmitter::handshake()
{
	Q_DEBUG_BLOCK

    QString handshakeUrl = QString::null;

    if ( PROTOCOL_VERSION == "1.1" )
    {
        // Audioscrobbler protocol 1.1 (current)
        // http://post.audioscrobbler.com/?hs=true
        // &p=1.1
        // &c=<clientid>
        // &v=<clientver>
        // &u=<user>
        handshakeUrl = QString( "/?hs=true&p=%1&c=%2&v=%3&u=%4" )
                          .arg( PROTOCOL_VERSION )
                          .arg( CLIENT_ID )
                          .arg( CLIENT_VERSION )
                          .arg( QString( QUrl::toPercentEncoding( m_username ) ) );
    }

    LOG(3, "Handshaking with scrobbler service. URL: " << handshakeUrl << "\n");

    m_inProgress = true;
    m_http.setHost( HANDSHAKE_HOST, 80 );
    m_handshakeId = m_http.get( handshakeUrl );

    emit statusChanged( Info, tr("Connecting to last.fm...") );
}


/**
 * Called when handshake TransferJob has finished and data is received.
 */
void
ScrobblerSubmitter::handshakeFinished( int id, bool error )
{
    QString result( m_http.readAll() );
    QString failureStatus = tr("Couldn't connect to submissions server. Will cache scrobbled tracks.");

    if ( id != m_handshakeId )
    {
        return; // setHost() or old handshake returning
    }

    m_inProgress = false;

    if ( error || result.size() == 0 )
    {
        if ( m_http.error() == QHttp::Aborted ) { return; }

        LOG( 1, "Scrobbler handshake QHttp error: " << m_http.errorString() << "\n" );
        emit statusChanged( Info, failureStatus );
        schedule( true );
        return;
    }

    m_prevSubmitTime = QDateTime::currentDateTime().toUTC().toTime_t();

    // UPTODATE
    // <md5 challenge>
    // <url to submit script>
    // INTERVAL n (protocol 1.1)
    if ( result.startsWith( "UPTODATE" ) || result.startsWith( "UPDATE" ) )
    {
        m_challenge = result.section( "\n", 1, 1 );
        m_submitUrl = QUrl( result.section( "\n", 2, 2 ) );
        QString interval = result.section( "\n", 3, 3 );

        if ( m_challenge == "" || m_submitUrl.isEmpty() )
        {
            LOG( 1, "Scrobbler handshake response was missing challenge or submit URL\n" );
        }

        if ( interval.startsWith( "INTERVAL" ) )
            m_interval = interval.mid( 9 ).toUInt();
    }

    // FAILED <reason (optional)>
    // INTERVAL n (protocol 1.1)
    else if ( result.startsWith( "FAILED" ) )
    {
        QString reason = result.mid( 0, result.indexOf( "\n" ) );
        if ( reason.length() > 6 )
            reason = reason.mid( 7 ).trimmed();

        QString interval = result.section( "\n", 1, 1 );
        if ( interval.startsWith( "INTERVAL" ) )
            m_interval = interval.mid( 9 ).toUInt();

        LOG( 1, "Scrobbler handshake returned FAILED. Error: " << reason << "\n" );
        emit statusChanged( Info, failureStatus );
    }

    // BADUSER (protocol 1.1) or BADAUTH (protocol 1.2)
    // INTERVAL n (protocol 1.1)
    else if ( result.startsWith( "BADUSER" ) ||
              result.startsWith( "BADAUTH" ) )
    {
        QString interval = result.section( "\n", 1, 1 );
        if ( interval.startsWith( "INTERVAL" ) )
            m_interval = interval.mid( 9 ).toUInt();

        LOG( 1, "Scrobbler handshake returned BADUSER or BADAUTH.\n" );
        emit statusChanged( BadAuth, tr("Incorrect username or password") );
    }
    else
    {
        LOG( 1, "Couldn't parse response to Scrobbler handshake.\n" );
        emit statusChanged( Info, failureStatus );
    }

    if ( !m_challenge.isEmpty() && !m_submitUrl.isEmpty() )
    {
        LOG( 3, "Scrobbler handshake successful. Challenge: " << m_challenge <<
            ", submitUrl: " << m_submitUrl.toString() << "\n" );

        m_submitHttp.setHost( m_submitUrl.host(), m_submitUrl.port() );
        emit statusChanged( Info, tr("Submission system up and running.") );
    }

    schedule( m_challenge.isEmpty() );
}


/**
 * Flushes the submit queues
 */
void
ScrobblerSubmitter::submit()
{
	Q_DEBUG_BLOCK

    
	if ( m_inProgress || !m_submitQueue.size() )
        return;

    QString data;
    TrackInfo* items[10];
    bool portable = false;

    // Audioscrobbler accepts max 10 tracks on one submit.
    for ( m_submitCounter = 0; m_submitCounter < 10; m_submitCounter++ )
        items[ m_submitCounter ] = 0;

    if ( PROTOCOL_VERSION == "1.1" )
    {
        // Audioscrobbler protocol 1.1 (current)
        // http://post.audioscrobbler.com/v1.1-lite.php
        // u=<user>
        // &s=<MD5 response>&
        // a[0]=<artist 0>&t[0]=<track 0>&b[0]=<album 0>&
        // m[0]=<mbid 0>&l[0]=<length 0>&i[0]=<time 0>&
        // a[1]=<artist 1>&t[1]=<track 1>&b[1]=<album 1>&
        // m[1]=<mbid 1>&l[1]=<length 1>&i[1]=<time 1>&
        // ...
        // a[n]=<artist n>&t[n]=<track n>&b[n]=<album n>&
        // m[n]=<mbid n>&l[n]=<length n>&i[n]=<time n>&

        data =  "u="  + QString( QString( QUrl::toPercentEncoding( m_username ) ) ) +
                "&s=" + UnicornUtils::md5Digest( QString( m_password + m_challenge ).toUtf8() );

        m_submitQueue.first();
        for ( m_submitCounter = 0; m_submitCounter < 10; m_submitCounter++ )
        {
            if ( m_submitQueue.size() < 1 )
                break;

            TrackInfo itemFromQueue = m_submitQueue.takeFirst();
            if ( itemFromQueue.source() == TrackInfo::MediaDevice )
                portable = true;

            m_progressQueue << itemFromQueue;

            const QString count = QString::number( m_submitCounter );
            data += "&a[" + count + "]=" + QUrl::toPercentEncoding( itemFromQueue.artist() ) +
                    "&t[" + count + "]=" + QUrl::toPercentEncoding( itemFromQueue.track() ) +
                    "&b[" + count + "]=" + QUrl::toPercentEncoding( itemFromQueue.album() ) +
                    "&m[" + count + "]=" + QUrl::toPercentEncoding( itemFromQueue.mbId() ) +
                    "&l[" + count + "]=" + QString::number( itemFromQueue.duration() ) +
                    "&i[" + count + "]=" + QUrl::toPercentEncoding( itemFromQueue.timeStamp() );
        }
    }
    else
    {
        LOG( 2, "Submit not implemented for protocol version:" << PROTOCOL_VERSION << "\n" );
        return;
    }

    LOG( 3, "Submitting: " << data << " to " << m_submitUrl.path() << " on " << m_submitUrl.host() << "\n" );
    m_inProgress = true;

    //Http *http = new Http( m_submitUrl.host(), 80, this );
    //Http *http = new Http( "bla.audioscrob.com", 80, this );
    //connect( http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( submitFinished( int, bool ) ) );

    QString submitUrl = m_submitUrl.path();
    if ( portable )
        submitUrl += "?portable=1";

    QHttpRequestHeader header( "POST", submitUrl );
    header.setValue( "Host", m_submitUrl.host() );
    header.setContentType( "application/x-www-form-urlencoded" );

    // setHost is called in handshakeFinished
    m_submitID = m_submitHttp.request( header, data.toUtf8() );

    emit statusChanged( InfoTrack, tr("Scrobbling %n tracks...", "", m_submitCounter ) );
}


/**
 * Called when submit has finished and data is received.
 */
void
ScrobblerSubmitter::submitFinished( int id, bool error )
{
	Q_DEBUG_BLOCK

    
	QString m_submitResultBuffer( m_submitHttp.readAll() );
    if ( id != m_submitID )
        return; // setHost() finished

    m_prevSubmitTime = QDateTime::currentDateTime().toUTC().toTime_t();
    m_inProgress = false;

    // OK
    // INTERVAL n (protocol 1.1)
    bool failed = error || !m_submitResultBuffer.startsWith( "OK" );
    if ( !failed )
    {
        LOG( 3, "Submit successful\n" );

        QString interval = m_submitResultBuffer.section( "\n", 1, 1 );
        if ( interval.startsWith( "INTERVAL" ) )
            m_interval = interval.mid( 9 ).toUInt();

        emit scrobbled();
        emit statusChanged( ScrobbledOK, QString( tr("%1 scrobbled.") )
           .arg( m_submitCounter > 1 ? tr("Tracks") : tr("Track") ) );
    }
    else
    {
        if ( m_submitHttp.error() == QHttp::Aborted ) { saveSubmitQueue(); return; }

        // FAILED <reason (optional)>
        // INTERVAL n (protocol 1.1)
        if ( m_submitResultBuffer.startsWith( "FAILED" ) )
        {
            QString reason = m_submitResultBuffer.mid( 0, m_submitResultBuffer.indexOf( "\n" ) );
            if ( reason.length() > 6 )
                reason = reason.mid( 7 ).trimmed();

            LOG( 2, "Submit failed. Reason: " << reason << "\n" );

            QString interval = m_submitResultBuffer.section( "\n", 1, 1 );
            if ( interval.startsWith( "INTERVAL" ) )
                m_interval = interval.mid( 9 ).toUInt();

            emit statusChanged( ScrobbleFailed, tr("Couldn't submit, will try again later") );

        }
        // BADAUTH
        // INTERVAL n (protocol 1.1)
        else if ( m_submitResultBuffer.startsWith( "BADAUTH" ) )
        {
            LOG( 2, "Submit returned BADAUTH, setting needHandshake true\n" );

            QString interval = m_submitResultBuffer.section( "\n", 1, 1 );
            if ( interval.startsWith( "INTERVAL" ) )
                m_interval = interval.mid( 9 ).toUInt();

            m_challenge = QString::null;
            m_needHandshake = true;

            emit statusChanged( BadAuth, tr("Couldn't submit, the username or password is incorrect") );
        }
        else
        {
            LOG( 2, "Couldn't parse submission response: " << m_submitResultBuffer << "\n" );
            emit statusChanged( ScrobbleFailed, tr("Couldn't contact server, will try again later") );
        }

        int i = 0;
        TrackInfo item;
        foreach( item, m_progressQueue )
            m_submitQueue.insert( i++, item );

    }

    m_progressQueue.clear();
    saveSubmitQueue();

    schedule( failed );
}


void
ScrobblerSubmitter::saveSubmitQueue()
{
	Q_DEBUG_BLOCK

	QFile file( m_savePath );
    if( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )
    {
        LOG( 1, "Could not open cache file to write submit queue: " << m_savePath << "\n" );
        return;
    }

    if ( m_lastSubmissionFinishTime == 0 )
        m_lastSubmissionFinishTime = QDateTime::currentDateTime().toUTC().toTime_t();

    QDomDocument newdoc;
    QDomElement submitQueue = newdoc.createElement( "submissions" );
    submitQueue.setAttribute( "product", "Audioscrobbler" );
    submitQueue.setAttribute( "version", XML_VERSION );
    submitQueue.setAttribute( "lastSubmission", m_lastSubmissionFinishTime );

    if ( m_progressQueue.size() )
    {
        for ( int idx = 0; idx < m_progressQueue.count(); idx++ )
        {
            TrackInfo item = m_progressQueue.at( idx );
            QDomElement i = item.toDomElement( newdoc );
            submitQueue.appendChild( i );
        }
    }
    if ( m_submitQueue.size() )
    {
        for ( int idx = 0; idx < m_submitQueue.count(); idx++ )
        {
            TrackInfo item = m_submitQueue.at( idx );
            QDomElement i = item.toDomElement( newdoc );
            submitQueue.appendChild( i );
        }
    }

    QDomNode submitNode = newdoc.importNode( submitQueue, true );
    newdoc.appendChild( submitNode );

    QTextStream stream( &file );
    stream.setCodec( "UTF-8" );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << newdoc.toString();

    LOG( 3, "Wrote cache file: " << m_savePath << "\n" );
    file.close();
}


void
ScrobblerSubmitter::readSubmitQueue( QString path, TrackInfo::Source source )
{
    QFile file( path );
    if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        LOG( 1, "Could not open media device file to read submit queue: " << path << "\n" );
        return;
    }

    QTextStream stream( &file );
    stream.setCodec( "UTF-8" );

    QDomDocument d;
    if ( !d.setContent( stream.readAll() ) )
    {
        LOG( 1, "Couldn't read file: " << path << "\n" );
        return;
    }

    uint last = 0;
    if ( d.namedItem( "submissions" ).isElement() )
        last = d.namedItem( "submissions" ).toElement().attribute( "lastSubmission" ).toUInt();

    if ( last && last > m_lastSubmissionFinishTime )
        m_lastSubmissionFinishTime = last;

    const QString ITEM( "item" ); //so we don't construct these QStrings all the time

    for( QDomNode n = d.namedItem( "submissions" ).firstChild(); !n.isNull() && n.nodeName() == ITEM; n = n.nextSibling() )
    {
        TrackInfo t( n.toElement() );
        t.setSource( source );

        m_submitQueue << t;
    }

    file.close();

    // TODO: this won't work if someone has the file open but unfortunately
    // resize(0) doesn't work either...
    file.remove();
}


/**
 * Checks if it is possible to try to submit the data to Audioscrobbler.
 */
bool ScrobblerSubmitter::canSubmit() const
{
    if ( m_username.isEmpty() || m_password.isEmpty() )
    {
        LOG( 3, "Unable to submit - no uname/pass\n" );
        return false;
    }

    return true;
}


/**
 * Schedules an Audioscrobbler handshake or submit as required.
 * Returns true if an immediate submit was possible
 */
 bool ScrobblerSubmitter::schedule( bool failure )
 {
	 Q_DEBUG_BLOCK

     
	 m_timer.stop();
     if ( m_inProgress || !canSubmit() )
         return false;

     uint when, currentTime = QDateTime::currentDateTime().toUTC().toTime_t();
     if ( ( currentTime - m_prevSubmitTime ) > m_interval )
         when = 0;
     else
         when = m_interval - ( currentTime - m_prevSubmitTime );

     if ( failure )
     {
         m_backoff = qMin( qMax( m_backoff * 2, unsigned( MIN_BACKOFF ) ), unsigned( MAX_BACKOFF ) );
         when = qMax( m_backoff, m_interval );
     }
     else
         m_backoff = 0;

     if ( m_needHandshake || m_challenge.isEmpty() )
     {
         m_challenge = QString::null;
         m_needHandshake = false;

         if ( when == 0 )
         {
             LOG( 4, "Performing immediate handshake\n" );
             handshake();
         }
         else
         {
             LOG( 3, "Performing handshake in " << when << " seconds\n" );
             m_timer.start( when * 1000 );
         }
     }
     else if ( !m_submitQueue.isEmpty() )
     {
         if ( when == 0 )
         {
             LOG( 4, "Performing immediate submit\n" );
             submit();
             return true;
         }
         else
         {
             LOG( 3, "Performing submit in " << when << " seconds\n" );
             m_timer.start( when * 1000 );
         }
     } else {
         LOG( 4, "Nothing to schedule\n" );
     }

     return false;
 }


/**
 * Called when timer set up in the schedule function goes off.
 */
void ScrobblerSubmitter::scheduledTimeReached()
{
    if ( m_needHandshake || m_challenge.isEmpty() )
        handshake();
    else
        submit();
}

