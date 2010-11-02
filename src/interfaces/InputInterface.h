/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
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

/*! \class InputInterface
    \brief Interface to Input plugins.

    Interface to Input plugins which take care of retrieving and streaming
    from local as well as remote data-sources.
*/

#ifndef INPUT_INTERFACE_H
#define INPUT_INTERFACE_H

#include <QObject>

#include "TrackInfo.h"
#include "RadioEnums.h"

class InputInterface : public QObject
{
    public:
        virtual ~InputInterface() {}

        /** \brief Returns true if new data is available. */
        virtual bool hasData() = 0;

        /** \brief Returns the data buffer and empties it afterwards. */
        virtual void data( QByteArray& fillMe, int numBytes ) = 0;

        /** \brief Returns the buffer capacity. */
        virtual int bufferCapacity() = 0;

        /** \brief Returns the actual buffer size. */
        virtual int bufferSize() = 0;

        /** \brief Changes the buffer-size.
          * \param size The new buffer-size. */
        virtual void setBufferCapacity( int size ) = 0;

        /** \brief Returns the current state. */
        virtual RadioState state() = 0;

    public slots:
        /** \brief Starts the streaming. */
        virtual void startStreaming() = 0;

        /** \brief Stops streaming. */
        virtual void stopStreaming() = 0;

        /** \brief Changes the current stream url. */
        virtual void load( const QString& url ) = 0;

        virtual void setSession( const QString& session ) = 0;

    signals:

        /** \brief Emits a state from the RadioState enum. */
        virtual void stateChanged( RadioState newState ) = 0;

        /** \brief Gets emitted whenever an error occurs during streaming or retrieving a url. */
        virtual void error( int errorCode, const QString& reason ) = 0;

        /*********************************************************************/ /**
            Emitted if the stream has to rebuffer. Buffering finishes when
            size == total.
            
            @param size - current buffer size
            @param total - total buffer size
        **************************************************************************/
        virtual void buffering( int size, int total ) = 0;    

};

Q_DECLARE_INTERFACE( InputInterface, "fm.last.Input/1.0" )

#endif
