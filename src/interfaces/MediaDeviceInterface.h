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

#ifndef MEDIADEVICE_INTERFACE_H
#define MEDIADEVICE_INTERFACE_H

#include "TrackInfo.h"
#include <QDateTime>

class MediaDeviceInterface : public QObject
{
    public:
        virtual ~MediaDeviceInterface() {}

        virtual QString LibraryPath() = 0;

        virtual TrackInfo firstTrack( const QString& file ) = 0;
        virtual TrackInfo nextTrack() = 0;

        virtual void forceDetection( const QString& path ) = 0;
        virtual void setupWatchers() = 0;

    signals:
        void deviceAdded( const QString& uid );
        void deviceChangeStart( const QString& uid, QDateTime lastItunesUpdateTime );
        void deviceChangeEnd( const QString& uid );
        void progress( int percentage, const TrackInfo& track );

        void trackChanged( const TrackInfo& track, int playCounter );
};

Q_DECLARE_INTERFACE( MediaDeviceInterface, "fm.last.MediaDevice/1.0" )

#endif
