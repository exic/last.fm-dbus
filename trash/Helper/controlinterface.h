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

#ifndef CONTROL_INTERFACE_H
#define CONTROL_INTERFACE_H

#include <QObject>
#include <QMutex>

class ControlInterface : public QObject
{
    Q_OBJECT

public:
    ControlInterface( QCoreApplication* parent );

    static bool sendToInstance( const QString& data = "" );

private slots:
    void clientConnect();
    void clientRequest();

private:
    class QCoreApplication* m_parent;

    class QTcpServer* m_serverSocket;
    class QTcpSocket* m_clientSocket;

    QMutex m_mutex;
};

#endif
