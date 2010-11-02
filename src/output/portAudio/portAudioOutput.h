/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
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

/*! \class PortAudioOutput
    \brief Sound-output plugin which uses PortAudio as its engine.
*/

#ifndef PORT_AUDIO_OUTPUT_H
#define PORT_AUDIO_OUTPUT_H

#include "interfaces/OutputInterface.h"
#include "portaudio.h"
#include <QObject>
#include <QMutex>

#define SAMPLERATE 44100
#define CHANNELS 2

class PortAudioOutput : public OutputInterface
{
    Q_OBJECT
    Q_INTERFACES( OutputInterface )

    public:
        PortAudioOutput();
        ~PortAudioOutput();

        virtual void initAudio( long sampleRate, int channels );
        virtual float volume() { return m_volume; }
        virtual void pause() { m_active = false; }
        virtual void resume() { m_active = true; }
        virtual bool hasData() { return m_buffer.size() > 0; }
        virtual bool needsData();
        virtual void processData( const QByteArray &buffer );
        virtual QStringList soundSystems();
        virtual QStringList devices();
        virtual void setDevice( int n ) { m_deviceNum = n; initAudio( SAMPLERATE, CHANNELS ); }

        virtual void setBufferCapacity( int size ) { m_bufferCapacity = size; }
        virtual int bufferSize() { return m_buffer.size(); }

    /// used by the callback
        bool isActive() { return m_active; }
        QMutex* mutex() { return &m_mutex; }
        QByteArray* buffer() { return &m_buffer; }
        PaDeviceInfo* deviceInfo() { return &m_deviceInfo; }
        int sourceChannels() { return m_sourceChannels; }

    public slots:
        virtual void clearBuffers();
        virtual void startPlayback();
        virtual void stopPlayback();
        virtual void setVolume( int volume ) { m_volume = (float)volume / 100.0; }

    signals:
        virtual void error( int error, const QString& reason );

    private:
        PaStream* m_audio;
        bool m_bufferEmpty;
        bool m_active;

        PaDeviceInfo m_deviceInfo;
        float m_volume;
        int m_sourceChannels;

        int m_deviceNum;

        QByteArray m_buffer;
        QMutex m_mutex;
        
        int m_bufferCapacity;
        
        int internalSoundCardID( int settingsID );
};

#endif
