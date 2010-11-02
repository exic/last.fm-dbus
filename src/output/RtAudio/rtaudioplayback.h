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

#ifndef RTAUDIOPLAYBACK_H
#define RTAUDIOPLAYBACK_H

#include "interfaces/OutputInterface.h"
#include "rtaudio/RtAudio.h"

#include <QObject>
#include <QTimer>
#include <QMutex>

class RtAudioPlayback : public OutputInterface
{
    Q_OBJECT
    Q_INTERFACES( OutputInterface )

    public:
        RtAudioPlayback();

        virtual void initAudio( long sampleRate, int channels );

        virtual float volume();

        virtual void pause();

        virtual void resume();

        virtual bool isActive() { return true; }
        virtual bool needsData();
        virtual bool hasData();

        virtual void processData( const QByteArray& data );

        QStringList soundSystems();
        QStringList devices();
        void setDevice( int n ) { m_deviceNum = n; initAudio( 44100, 2 ); }
    

        int audioCallback( char *buffer, int bufferSize );

        /*********************************************************************/ /**
            Returns the size of the buffer right now.
        **************************************************************************/
        int bufferSize();

        void setBufferCapacity( int size ) { m_bufferCapacity = size; }

    public slots:
        void clearBuffers();

        void startPlayback();
        void stopPlayback();

        void setVolume( int volume );

    signals:
        void error( int error, const QString& reason );

    private:
        int internalSoundCardID( int settingsID );

        RtAudio* m_audio;

        QByteArray m_buffer;
        int m_bufferCapacity;
        
        int m_deviceNum;
        
        float m_volume;

        QMutex m_mutex;
        
};

#endif
