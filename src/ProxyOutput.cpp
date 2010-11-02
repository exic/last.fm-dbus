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

#include "ProxyOutput.h"
#include "logger.h"
#include "LastFmSettings.h"


ProxyOutput::ProxyOutput() :
        m_active( false ),
        m_socket( new QTcpServer( this ) )
{
    Q_DEBUG_BLOCK;

    connect( m_socket, SIGNAL( newConnection() ),
             this,       SLOT( onClientConnected() ) );

    if ( The::settings().musicProxyPort() )
        m_socket->listen( QHostAddress::LocalHost, The::settings().musicProxyPort() );
}


ProxyOutput::~ProxyOutput()
{
    Q_DEBUG_BLOCK;
}


void
ProxyOutput::startPlayback()
{
    qDebug() << "Playback started";
}


void
ProxyOutput::stopPlayback()
{
    qDebug() << "Playback stopped";

    m_active = false;
    foreach( QTcpSocket* socket, m_clients )
    {
        m_clients.removeAll( socket );

        socket->close();
        delete socket;
    }

    m_buffer.clear();
}


void
ProxyOutput::processData( const QByteArray &buffer )
{
    m_active = ( m_clients.count() > 0 );
    if ( m_active )
    {
        m_buffer.append( buffer );
        if ( needsData() )
            return;

        QByteArray avail = buffer.left( 8192 );
        foreach( QTcpSocket* socket, m_clients )
        {
            if ( socket->state() == QTcpSocket::UnconnectedState )
            {
                m_clients.removeAll( socket );
                delete socket;
            }
            else
            {
                socket->write( avail, avail.size() );
                socket->flush();
            }
        }

        m_buffer.remove( 0, avail.size() );
    }
}


void
ProxyOutput::clearBuffers()
{
}


void
ProxyOutput::onClientConnected()
{
    QTcpSocket* socket = m_socket->nextPendingConnection();
    m_clients << socket;

    QByteArray token( "HTTP/1.1 200 OK\nContent-Type: audio/mpeg\n\n" );
    socket->write( token, token.length() );

    m_active = ( m_clients.count() > 0 );
}
