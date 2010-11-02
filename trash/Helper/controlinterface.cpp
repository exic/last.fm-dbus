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

#include "controlinterface.h"

#include "MooseCommon.h"
#include "logger.h"
#include "LastFmSettings.h"

#include <QCoreApplication>
#include <QWaitCondition>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>

ControlInterface::ControlInterface( QCoreApplication* parent )
		: m_parent( parent ),
		  m_clientSocket( 0 )
{
    m_serverSocket = new QTcpServer( this );

    connect( m_serverSocket, SIGNAL( newConnection() ), this, SLOT( clientConnect() ) );

    if ( !m_serverSocket->listen( QHostAddress::LocalHost, The::settings().helperControlPort() ) )
        m_serverSocket->listen( QHostAddress::LocalHost );

    The::settings().setHelperControlPort( m_serverSocket->serverPort() );
}


void
ControlInterface::clientConnect()
{
    // Incoming connection from either a webbrowser or
    // some other program like the firefox extension.
    QMutexLocker locker( &m_mutex );
    delete m_clientSocket;

    m_clientSocket = m_serverSocket->nextPendingConnection();

    connect( m_clientSocket, SIGNAL( readyRead() ), SLOT( clientRequest() ) );
}


void
ControlInterface::clientRequest()
{
    QMutexLocker locker( &m_mutex );
    QString request = QString::fromUtf8( m_clientSocket->readAll() );

    LOGL( 3, "clientRequest (old instance): " << request );

    if ( !request.isEmpty() )
    {
        if ( request.contains( "--quit" ) )
        {
            LOGL( 3, "old instance calling parent->quit" );

            m_parent->quit();
        }

        m_clientSocket->flush();
    }

    m_clientSocket->close();
    m_clientSocket->deleteLater();
    m_clientSocket = 0;
}


bool
ControlInterface::sendToInstance( const QString& data )
{
    LOGL( 3, "sendToInstance (new instance): " << data );

    QTcpSocket socket;
    socket.connectToHost( QHostAddress::LocalHost, The::settings().helperControlPort() );

    if ( socket.waitForConnected( 500 ) )
    {
        if ( data.length() > 0 )
        {
            QByteArray utf8Data = data.toUtf8();
            socket.write( utf8Data, utf8Data.length() );
            socket.flush();
        }

        socket.close();
        return true;
    }
    else
    {
        LOGL( 1, "sendToInstance failed, SocketError: " << socket.error() );
    }

    return false;
}
