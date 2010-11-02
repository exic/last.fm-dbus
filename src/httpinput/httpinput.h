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

#ifndef HTTPINPUT_H
#define HTTPINPUT_H

#include "interfaces/InputInterface.h"
#include "metadata.h"
#include "WebService/fwd.h"
#include "RadioEnums.h"
#include "CachedHttp.h"

#include <QMutex>
#include <QObject>
#include <QUrl>
#include <QTimer>
#include <QString>

/*************************************************************************/ /**
    Responsible for HTTP streaming.

    The streamer sends at full speed for the first 10 seconds and then goes
    down to 16kb/s.

    This class is not thread-safe since it's used as part of a separate
    audio thread managed by the AudioController.
******************************************************************************/
class HttpInput : public InputInterface
{
    Q_OBJECT
    Q_INTERFACES( InputInterface )

public:

    HttpInput();

    virtual bool hasData();
    virtual void data( QByteArray& fillMe, int numBytes );

    virtual void setBufferCapacity( int size );
    virtual int bufferCapacity() { return m_bufferCapacity; }

    /*********************************************************************/ /**
        Returns the size of the buffer right now.
    **************************************************************************/
    int bufferSize() { return m_buffer.size(); }

    RadioState state() { return m_state; }

public slots:

    /*********************************************************************/ /**
        Starts streaming. This method will lead to the state going to
        FetchingStream, StreamFetched, Buffering and Streaming in that order.
    **************************************************************************/
    virtual void startStreaming();

    /*********************************************************************/ /**
        Stops streaming. This method is atomic and the object's state will go
        to Stopped immediately.
    **************************************************************************/
    virtual void stopStreaming();

    /*********************************************************************/ /**
        Set the session key to use for streaming.

        Now have to pass session key through as well as Mischa wants it on
        the server side.

        @param session - session key
    **************************************************************************/
    virtual void setSession( const QString& session = "" );

    /*********************************************************************/ /**
        Set the URL to use for streaming.

        @param url - the location of a streamable track (http://...mp3)
    **************************************************************************/
    virtual void load( const QString& url );

signals:
    virtual void stateChanged( RadioState newState );

    virtual void error( int errorCode, const QString& reason );

    /*********************************************************************/ /**
        Emitted if the stream has to rebuffer. Buffering finishes when
        size == total.

        @param size - current buffer size
        @param total - total buffer size
    **************************************************************************/
    virtual void
    buffering( int size, int total );

private:

    RadioState m_state;

    QUrl m_streamUrl;
    QString m_session;

    CachedHttp m_http;

    QByteArray m_buffer;
    int m_bufferCapacity;
    int m_lastRequestId;

    QString m_genericAsOfYetUndiagnosedStreamerError;

    QTimer m_timeoutTimer;

private slots:

    void
    onHttpDataAvailable( const QHttpResponseHeader &resp );

    void
    onHttpResponseHeader( const QHttpResponseHeader &resp );

    void
    onHttpStateChange( int state );

    void
    onHttpRequestFinished( int id, bool error );

    void
    onHttpTimeout();

    void
    setState( RadioState newState );

};

#endif
