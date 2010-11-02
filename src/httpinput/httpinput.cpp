/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jalevik, Last.fm Ltd <erik@last.fm>                          *
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

#include "httpinput.h"
#include "logger.h"
#include "WebService.h"
#include "WebService/Request.h"
#include "RadioEnums.h"

#include "MooseCommon.h"

#include <QDebug>
#include <QtPlugin>


HttpInput::HttpInput() :
        m_state( State_Stopped ),
        m_http( this ),
        m_bufferCapacity( 16 * 1024 ),
        m_lastRequestId( -1 )
{
    LOGL( 3, "Initialising HTTP Input" );

    connect( &m_http, SIGNAL( readyRead( QHttpResponseHeader ) ),
                      SLOT  ( onHttpDataAvailable( QHttpResponseHeader ) ) );
    connect( &m_http, SIGNAL( responseHeaderReceived( const QHttpResponseHeader& ) ),
                      SLOT  ( onHttpResponseHeader( const QHttpResponseHeader& ) ) );
    connect( &m_http, SIGNAL( stateChanged( int ) ),
                      SLOT  ( onHttpStateChange( int ) ) );
    connect( &m_http, SIGNAL( requestFinished( int, bool ) ),
                      SLOT  ( onHttpRequestFinished( int, bool ) ) );

    m_timeoutTimer.setSingleShot( true );
    m_timeoutTimer.setInterval( 29 * 1000 );
    connect( &m_timeoutTimer, SIGNAL( timeout() ),
                              SLOT  ( onHttpTimeout() ) );

    m_genericAsOfYetUndiagnosedStreamerError =
        tr( "There was a problem contacting the radio streamer. "
            "Please try again later." );
}


bool
HttpInput::hasData()
{
    return !m_buffer.isEmpty() &&
           ( m_state == State_Streaming || m_state == State_Stopped );
}


void
HttpInput::data( QByteArray& fillMe, int numBytes )
{
    switch ( m_state )
    {
        case State_FetchingStream:
        case State_Buffering:
        {
            // Do nothing, the caller will have to wait until we're streaming again.
            Q_ASSERT( !"Caller shouldn't call us when we're not streaming." );
        }
        break;

        case State_Streaming:
        case State_Stopped:
        {
            // Even though we've stopped there might still be data left in the buffer
            fillMe = m_buffer.left( numBytes );
            m_buffer.remove( 0, numBytes );

            // Once the Http connection's closed, we've gone to Stopped
            if ( m_state != State_Stopped )
            {
                if ( m_buffer.size() == 0 )
                {
                    LOGL( 3, "Buffer empty, buffering..." );

                    m_timeoutTimer.start();

                    setState( State_Buffering );
                    emit buffering( 0, m_bufferCapacity );
                }
            }
        }
        break;

        default:
        {
            Q_ASSERT( !"Unhandled state" );
        }
        break;
    }

}


void
HttpInput::setBufferCapacity( int size )
{
    m_bufferCapacity = size;
}


void
HttpInput::startStreaming()
{
    // As we're driven by the AudioControllerThread, we can be sure that
    // we're always stopped before a new start request comes in. Anything
    // else is a bug.
    Q_ASSERT( m_state == State_Stopped );

    LOGL( 3, "Starting streaming from: " << m_streamUrl.toString() );

    m_http.setHost( m_streamUrl.host(), m_streamUrl.port() > 0 ? m_streamUrl.port() : 80 );

    QString path = m_streamUrl.path();
    if ( !m_streamUrl.encodedQuery().isEmpty() )
    {
        path += "?" + QString( m_streamUrl.encodedQuery() );
    }

    QHttpRequestHeader header( "GET", path );
    header.setValue( "Host", m_streamUrl.host() );
    if ( !m_session.isEmpty() )
        header.setValue( "Cookie", "Session=" + m_session );
    m_lastRequestId = m_http.request( header );

    m_timeoutTimer.start();

    setState( State_FetchingStream );
}


void
HttpInput::stopStreaming()
{
    // A stop however, can come in in any state so we must handle it properly.
    switch ( m_state )
    {
        case State_FetchingStream:
        case State_StreamFetched:
        case State_Buffering:
        case State_Streaming:
        {
            m_http.abort();
            m_buffer.clear();
            m_timeoutTimer.stop();
            setState( State_Stopped );
        }
        break;

        case State_Stopped:
        {
            // Even if we're stopped, we might have some unplayed buffer left
            m_buffer.clear();
        }
        break;

        default:
            Q_ASSERT( !"Unhandled case" );
    }
}


void
HttpInput::setSession( const QString& sesh )
{
    m_session = sesh;
}


void
HttpInput::load( const QString& url )
{
    m_streamUrl = url;
}


void
HttpInput::onHttpStateChange( int state )
{
    //NOTE! No longer used. Only left this in here to ease debugging,
    // should we ever need to.

    switch( state )
    {
        case QHttp::Reading:
        case QHttp::Closing:
        case QHttp::Unconnected:
        case QHttp::Connecting:
        case QHttp::Sending:
        case QHttp::Connected:
        case QHttp::HostLookup:
        default:
            break;
    }
}


void
HttpInput::onHttpResponseHeader( const QHttpResponseHeader& resp )
{
    m_timeoutTimer.stop();

    int respCode = resp.statusCode();
    QString respPhrase = resp.reasonPhrase();
    QString errorMsg = "\n\nStreamer error code: " + QString::number( respCode ) +
                       "\nReason: " + respPhrase;

    // Redirects handled by RedirectHttp class
    if ( respCode != 200 && respCode != 301 && respCode != 302 && respCode != 307 )
    {
        LOGL( 2, errorMsg )
    }

    switch( resp.statusCode() )
    {
        case 200:
        {
            // Success
        }
        break;

        case 403:
        {
            // INVALID_TICKET         = "HTTP/1.1 403 Invalid ticket";
            // INVALID_AUTH           = "HTTP/1.1 403 Invalid authorization";

            // TODO: better error messages
            if ( respPhrase == "Invalid ticket" )
                emit error( Radio_InvalidUrl,
                    m_genericAsOfYetUndiagnosedStreamerError + errorMsg );
            else
                emit error( Radio_InvalidAuth,
                    tr( "Invalid authorisation." ) + errorMsg );
        }
        break;

        case 404:
        {
            // Should try different URL if we have one.
            // NO_TRACK_FOUND         = "HTTP/1.1 404 Track not found";
            // NO_TRACK_AVAILABLE     = "HTTP/1.1 404 Track not available"
            //                        = "HTTP/1.1 404 Track invalid"
            emit error( Radio_TrackNotFound,
                tr( "This stream is currently not available. Please try again later." ) + errorMsg );
        }
        break;

        case 503:
        {
            // EXCEEDED_SKIP_LIMIT    = "HTTP/1.1 503 Skip limit exceeded"
            // UNEXPECTED_ERROR       = "HTTP/1.1 503 Unexpected Error";
            if ( resp.reasonPhrase() == "Skip limit exceeded" )
                emit error( Radio_SkipLimitExceeded, tr( "Skip limit exceeded." ) + errorMsg );
            else
                emit error( Radio_UnknownError, m_genericAsOfYetUndiagnosedStreamerError + errorMsg );
        }
        break;

        case 301:   //Moved Permanently
        case 302:
        case 307:   //Temporary Redirect
        {
            // This is now handled by the RedirectHttp class
        }
        break;

        default:
        {
            Q_ASSERT( !"Mischa didn't tell us about this one." );
            emit error( Radio_UnknownError, m_genericAsOfYetUndiagnosedStreamerError + errorMsg );
        }

    }
}


void
HttpInput::onHttpDataAvailable( const QHttpResponseHeader &resp )
{
    Q_UNUSED( resp );

    m_timeoutTimer.stop();

    m_buffer.append( m_http.readAll() );

    switch ( m_state )
    {
        case State_FetchingStream:
        {
            setState( State_StreamFetched );
            setState( State_Buffering );
        }
        // fall-through

        case State_Buffering:
        {
            if ( m_buffer.size() >= m_bufferCapacity )
            {
                setState( State_Streaming );
            }

            // We want both numbers to be the same when the buffering is finished,
            // client code might rely on it.
            emit buffering( qMin( m_buffer.size(), m_bufferCapacity ), m_bufferCapacity );
        }
        break;

        case State_Streaming:
        {
            // Do nothing
        }
        break;

        case State_Stopped:
        default:
            Q_ASSERT( !"Invalid state" );
    }
}


void
HttpInput::onHttpRequestFinished( int id, bool err )
{
    Q_UNUSED( id );

    m_timeoutTimer.stop();

    if ( err )
    {
        // QHttp error codes:
        //NoError,
        //UnknownError,
        //HostNotFound,
        //ConnectionRefused,
        //UnexpectedClose,
        //InvalidResponseHeader,
        //WrongContentLength,
        //Aborted

        if ( m_http.error() != QHttp::Aborted )
        {
            LOGL( 2, "HttpInput get failed. " << "\n" <<
                     "  Http response: " << m_http.lastResponse().statusCode() << "\n" <<
                     "  QHttp error code: " << m_http.error() << "\n" <<
                     "  QHttp error text: " << m_http.errorString() << "\n" <<
                     "  Request: " << m_http.currentRequest().path() << "\n" <<
                     "  Bytes returned: " << m_http.bytesAvailable() << "\n" );

            // Streamer not contactable
            emit error( Radio_ConnectionRefused, m_genericAsOfYetUndiagnosedStreamerError +
                "\n\nHttp error: " + m_http.errorString() );
        }
    }

    // If we do this here instead of in onHttpStateChanged, it works both for
    // proxied and non-proxied connections. For some reason, a proxy connection
    // emits a different sequence of state changes so listening out for Closing
    // wasn't reliable.
    if ( id == m_lastRequestId )
    {
        setState( State_Stopped );
    }
}


void
HttpInput::onHttpTimeout()
{
    // Streamer not contactable
    emit error( Radio_ConnectionRefused, m_genericAsOfYetUndiagnosedStreamerError +
        "\n\nError: The connection timed out." );
    stopStreaming();
}


void
HttpInput::setState( RadioState newState )
{
    // These are the only valid states
    Q_ASSERT( newState == State_FetchingStream ||
              newState == State_StreamFetched ||
              newState == State_Buffering ||
              newState == State_Streaming ||
              newState == State_Stopped );

    if ( newState != m_state )
    {
        LOGL( 4, "HttpInput state: " << radioState2String( newState ) );

        m_state = newState;

        emit stateChanged( newState );
    }
}


Q_EXPORT_PLUGIN2( input, HttpInput )
