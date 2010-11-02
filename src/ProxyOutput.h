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

#ifndef PROXYOUTPUT_H
#define PROXYOUTPUT_H

#include <QObject>
#include <QMutex>
#include <QTcpServer>
#include <QTcpSocket>
#include <QStringList>

class ProxyOutput : public QObject
{
    Q_OBJECT

    public:
        ProxyOutput();
        ~ProxyOutput();

        virtual void pause() { m_active = false; }
        virtual void resume() { m_active = true; }
        virtual void processData( const QByteArray &buffer );

        bool needsData() { return m_buffer.count() < 16384; }

    /// used by the callback
        bool isActive() { return m_active; }

    public slots:
        virtual void clearBuffers();
        virtual void startPlayback();
        virtual void stopPlayback();

    signals:
        virtual void error( int error, const QString& reason );

    private slots:
        void onClientConnected();

    private:
        bool m_active;

        QTcpServer* m_socket;
        QList<QTcpSocket*> m_clients;
        QByteArray m_buffer;
};

#endif
