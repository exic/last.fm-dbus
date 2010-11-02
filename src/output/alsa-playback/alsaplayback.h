/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
 *      Max Howell, Last.fm Ltd <max@last.fm>                              *
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

#ifndef ALSA_PLAYBACK_H
#define ALSA_PLAYBACK_H

#include "interfaces/OutputInterface.h"


class AlsaPlayback : public OutputInterface
{
    Q_OBJECT
    Q_INTERFACES( OutputInterface )

        virtual void initAudio( long sampleRate, int channels );

        virtual float volume() { return m_volume; }
        
        virtual void pause() { Q_ASSERT( !"Not implemented" ); }
        virtual void resume() { Q_ASSERT( !"Not implemented" ); }
        
        virtual bool hasData();
        virtual bool needsData();
        virtual void processData( const QByteArray& );
        virtual void setBufferCapacity( int size );
        virtual int bufferSize();
        
        virtual QStringList soundSystems();
        virtual QStringList devices();
        virtual void setDevice( int n ) { m_deviceNum = n; initAudio( 44100, 2 ); }

        bool isActive() { return true; }

    public slots:
        virtual void clearBuffers();
        virtual void startPlayback();
        virtual void stopPlayback();
        virtual void setVolume( int volume );

    signals:
        virtual void error( int error, const QString& reason );

    public:
        AlsaPlayback();
        ~AlsaPlayback();

    private:
        class AlsaAudio *m_audio;
        int m_bufferCapacity;

        static float m_volume;
        int m_deviceNum;

        QString internalSoundCardID( int settingsID );
};

#endif
