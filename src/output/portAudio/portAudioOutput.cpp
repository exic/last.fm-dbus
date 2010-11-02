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

#include <QMutexLocker>
#include <QtPlugin>
#include <QStringList>
#include <QMessageBox>

#include "portAudioOutput.h"
#include "logger.h"
#include "RadioEnums.h"


int
audioCallback( const void*, void* outputBuffer, unsigned long frameCount,
               const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* data_src )
{
    PortAudioOutput* parent = (PortAudioOutput*)data_src;
    QMutexLocker locker( parent->mutex() );

    if ( !outputBuffer || frameCount != 512 )
        return 0;

    char* buffer = (char*)outputBuffer;
    int bufs = frameCount * 2 * parent->sourceChannels();
//     memset( buffer, 0, bufs * ( parent->deviceInfo()->maxOutputChannels / 2 ) );
    memset( buffer, 0, bufs );

    int x = 0;
    if ( parent->buffer()->size() && parent->isActive() )
    {
        // Apply volume scaling
        for ( int i = 0; i < bufs / 2 && i < parent->buffer()->size() / 2; i++ )
        {
            union PCMDATA
            {
                short i;
                unsigned char b[2];
            } pcmData;

            pcmData.b[0] = parent->buffer()->at( i * 2 );
            pcmData.b[1] = parent->buffer()->at( i * 2 + 1 );

            float pcmValue = (float)pcmData.i * parent->volume();
            pcmData.i = (short)pcmValue;

//             for ( int y = 0; y < parent->deviceInfo()->maxOutputChannels / 2; y++ )
            {
                buffer[x++] = pcmData.b[0];
                buffer[x++] = pcmData.b[1];
            }
        }

        parent->buffer()->remove( 0, bufs > parent->buffer()->size() ? parent->buffer()->size() : bufs );
    }

    return 0;
}


PortAudioOutput::PortAudioOutput() :
        m_audio( 0 ),
        m_bufferEmpty( true ),
        m_active( true ),
        m_volume( 0.9f ),
        m_deviceNum( -1 )
{
    Q_DEBUG_BLOCK;

    PaError error = Pa_Initialize();
    if ( error == paNoError )
    {
        devices();
    }
    else
        qDebug() << "PortAudio Error:" << Pa_GetErrorText( error );
}


PortAudioOutput::~PortAudioOutput()
{
    Q_DEBUG_BLOCK;

    if ( m_audio )
        Pa_CloseStream( m_audio );

    Pa_Terminate();
}


void
PortAudioOutput::initAudio( long sampleRate, int channels )
{
    if ( m_audio )
    {
        Pa_CloseStream( m_audio );
        m_audio = 0;
    }

    if ( m_deviceNum >= Pa_GetDeviceCount() || m_deviceNum < 0 )
        m_deviceNum = 0;

    int bufferSize = 512;
    int deviceID = internalSoundCardID( m_deviceNum );
    qDebug() << "Internal ID:" << deviceID << "-" << "Config:" << m_deviceNum;

    if ( deviceID < 0 )
    {
        emit error( Radio_NoSoundcard, tr( "Your soundcard is either busy or not present. "
            "Try restarting the application." ) );
        return;
    }

    PaStreamParameters p;
    memset( &p, 0, sizeof( PaStreamParameters ) );

    p.sampleFormat = paInt16;
    p.channelCount = 0;

    while ( p.channelCount < channels && deviceID < Pa_GetDeviceCount() )
    {
#ifdef Q_WS_WIN
        p.device = Pa_HostApiDeviceIndexToDeviceIndex( Pa_HostApiTypeIdToHostApiIndex( paDirectSound ), deviceID++ );
#endif
#ifdef Q_WS_MAC
        p.device = Pa_HostApiDeviceIndexToDeviceIndex( Pa_HostApiTypeIdToHostApiIndex( paCoreAudio ), deviceID++ );
#endif
#ifdef Q_WS_X11
        p.device = Pa_HostApiDeviceIndexToDeviceIndex( Pa_HostApiTypeIdToHostApiIndex( paALSA ), deviceID++ );
#endif

        p.suggestedLatency = Pa_GetDeviceInfo( p.device )->defaultHighOutputLatency;
        p.channelCount = Pa_GetDeviceInfo( p.device )->maxOutputChannels;
    }

    qDebug() << "Using device with id:" << --deviceID;
    p.channelCount = channels;
//     Pa_IsFormatSupported( 0, &p, bufferSize );

    m_deviceInfo = *Pa_GetDeviceInfo( p.device );
    m_sourceChannels = channels;

    PaError error = Pa_OpenStream( &m_audio, 0, &p, sampleRate, bufferSize, 0, audioCallback, this );
    if ( error != paNoError )
    {
        qDebug() << "PortAudio Error:" << Pa_GetErrorText( error );
        m_audio = 0;
    }
}


QStringList
PortAudioOutput::soundSystems()
{
    return QStringList()

        #ifdef WIN32
        << "DirectSound"
        #endif

        #ifdef Q_WS_X11
        << "Alsa"
        #endif

        #ifdef Q_WS_MAC
        << "CoreAudio"
        #endif

        ;
}


QStringList
PortAudioOutput::devices()
{
    Q_DEBUG_BLOCK;

    QStringList l;

    int const N = Pa_GetDeviceCount();
    for (int i = 0; i < N; i++)
    {
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo( i );

        if ( deviceInfo->maxOutputChannels > 0 )
        {
            l << deviceInfo->name;
            qDebug() << "Device #" << i << "named" << deviceInfo->name << "-" << deviceInfo->maxOutputChannels << "channels";
        }

    }

    return l;
}


#define PLAYBACK_ERROR( PaError ) \
    emit error( Radio_PlaybackError, \
                "<p><b>" + tr("A playback error occurred.") + "</b>" + \
                "<p>" + Pa_GetErrorText( PaError ) );


void
PortAudioOutput::startPlayback()
{
    if (!m_audio) {
        emit error( Radio_NoSoundcard, tr("No soundcard available.") );
        return;
    }

    PaError e = Pa_StartStream( m_audio );
    if ( e != paNoError &&  e != paStreamIsNotStopped )
        emit PLAYBACK_ERROR( e );
}


void
PortAudioOutput::stopPlayback()
{
    if (!m_audio)
        return;

    PaError e = Pa_StopStream( m_audio );
    if ( e != paNoError && e != paStreamIsStopped )
        emit PLAYBACK_ERROR( e );

    QMutexLocker locker( &m_mutex );
    m_buffer.clear();
}


bool
PortAudioOutput::needsData()
{
    if (m_buffer.isEmpty() && !m_bufferEmpty)
        m_bufferEmpty = true;

    return (m_buffer.size() < m_bufferCapacity);
}


void
PortAudioOutput::processData( const QByteArray &buffer )
{
    QMutexLocker locker( &m_mutex );

    m_buffer.append( buffer );

    if (m_bufferEmpty && !buffer.isEmpty())
        m_bufferEmpty = false;
}


void
PortAudioOutput::clearBuffers()
{
    QMutexLocker locker( &m_mutex );

    m_buffer.clear();
    m_bufferEmpty = true;
}


int
PortAudioOutput::internalSoundCardID( int settingsID )
{
    if ( settingsID < 0 )
        settingsID = 0;

    int firstCardFound = -1;
    int card = 0;
    int const N = Pa_GetDeviceCount();
    for (int i = 0; i < N; i++)
    {
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo( i );

        if ( deviceInfo->maxOutputChannels > 0 )
        {
            if ( firstCardFound < 0 )
                firstCardFound = card;

            if ( card++ == settingsID )
                return i;
        }

    }

    return Pa_GetDefaultOutputDevice();
//    return firstCardFound;
}


Q_EXPORT_PLUGIN2( output, PortAudioOutput )
