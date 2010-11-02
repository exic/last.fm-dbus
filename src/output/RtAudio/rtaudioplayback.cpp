/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jalevik, Last.fm Ltd <erik@last.fm>                           *
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

#include <QtGui>

#include "rtaudioplayback.h"
#include "logger.h"

#include "RadioEnums.h"

int
audioCallback( char *buffer, int bufferSize, void* data_src )
{
    RtAudioPlayback* parent = reinterpret_cast<RtAudioPlayback*>( data_src );
    return parent->audioCallback( buffer, bufferSize );
}

int
RtAudioPlayback::audioCallback( char *buffer, int bufferSize )
{
    // this is a safety check, as rtaudio can only have specific buffer sizes
    // or apparently it crashes, muesli knows more on this
    if ( !buffer || bufferSize != 512 )
        return 0;

    int bufs = bufferSize * 4;
    memset( buffer, 0, bufs );

    m_mutex.lock();

    int ourBufferSize = m_buffer.size();
    if ( ourBufferSize > 0 )
    {
        // Apply volume scaling
        int i;
        for ( i = 0; i < ( bufs / 2 ) && i < ( ourBufferSize / 2 ); i++ )
        {
            union PCMDATA
            {
                short i;
                unsigned char b[2];
            } pcmData;

            pcmData.b[0] = m_buffer.at( i * 2 );
            pcmData.b[1] = m_buffer.at( i * 2 + 1 );

            float pcmValue = (float)pcmData.i * m_volume;
            pcmData.i = (short)pcmValue;

            buffer[i * 2] = pcmData.b[0];
            buffer[i * 2 + 1] = pcmData.b[1];
        }

        // Pad with zeroes if we didn't fill the whole buffer
        for ( int j = i; j < ( bufs / 2 ); j++ )
        {
            buffer[j * 2] = 0;
            buffer[j * 2 + 1] = 0;
        }

        // Will remove the entirety if bufs > buffer size
        m_buffer.remove( 0, bufs );
    }
    else
    {
        // Pad with zeroes if we didn't fill the whole buffer
        for ( int k = 0; k < ( bufs / 2 ); k++ )
        {
            buffer[k * 2] = 0;
            buffer[k * 2 + 1] = 0;
        }
    }

    m_mutex.unlock();

    return 0;
}



RtAudioPlayback::RtAudioPlayback() :
    m_audio( 0 ),
    m_volume( 0.5 ),
    m_deviceNum( 0 )
{
    LOGL( 3, "Initialising RTAudio Playback" );
}

void
RtAudioPlayback::pause()
{
    Q_ASSERT( !"Not implemented" );
}

void
RtAudioPlayback::resume()
{
    Q_ASSERT( !"Not implemented" );
}

bool
RtAudioPlayback::hasData()
{
    m_mutex.lock();

    bool has = m_buffer.size() > 0;

    m_mutex.unlock();

    return has;
}

bool
RtAudioPlayback::needsData()
{
    m_mutex.lock(); 
    int size = m_buffer.size();
    m_mutex.unlock();

    return ( size < m_bufferCapacity );
}


void
RtAudioPlayback::processData( const QByteArray &buffer )
{
    m_mutex.lock(); 
    m_buffer.append( buffer );
    m_mutex.unlock();
}


QStringList
RtAudioPlayback::soundSystems()
{
    QStringList l;

    #ifdef WIN32
        l << "DirectSound";
    #endif

    #ifdef Q_WS_X11
        l << "Alsa";
    #endif

    #ifdef Q_WS_MAC
        l << "CoreAudio";
    #endif

    return l;
}


QStringList
RtAudioPlayback::devices()
{
    QStringList l;

    if ( !m_audio )
        return l;

    try
    {
        qDebug() << "Device nums:" << m_audio->getDeviceCount();

        for ( int i = 1; i <= m_audio->getDeviceCount(); i++ )
        {
            RtAudioDeviceInfo info;
            info = m_audio->getDeviceInfo( i );
            qDebug() << "Device name:" << QString::fromStdString( info.name )
                     << "- output:" << info.outputChannels
                     << "- input:" << info.inputChannels
                     << "- duplex:" << info.duplexChannels;

            if ( info.outputChannels > 0 )
            {
                l << QString::fromStdString( info.name ); // check if we can make it utf8 compatible
            }
        }
    }
    catch ( RtError &error )
    {
        LOGL( 1, "Getting device names failed. RtAudio error type: " << error.getType() <<
                 " Message: " << error.getMessage() );
    }

    return l;
}


void
RtAudioPlayback::startPlayback()
{
    if ( !m_audio )
    {
        emit error( Radio_NoSoundcard, tr( "Your soundcard is either busy or not present. "
            "Try restarting the application." ) );
        return;
    }

    try
    {
        m_audio->setStreamCallback( &::audioCallback, this );
        m_audio->startStream();
    }
    catch ( RtError &error )
    {
        LOGL( 1, "Starting stream failed. RtAudio error type: " << error.getType() <<
                 " Message: " << error.getMessage() );
        emit this->error( Radio_PlaybackError, tr( "Couldn't start playback. Error:\n\n%1" ).
            arg( QString::fromStdString( error.getMessage() ) ) );
    }
}


void
RtAudioPlayback::stopPlayback()
{
    if ( !m_audio )
    {
        return;
    }

    m_audio->stopStream();
    m_audio->cancelStreamCallback();

    m_mutex.lock();
    m_buffer.clear();
    m_mutex.unlock();
}


float
RtAudioPlayback::volume()
{
    QMutexLocker locker( &m_mutex );
    return m_volume;
}


void
RtAudioPlayback::setVolume( int volume )
{
    QMutexLocker locker( &m_mutex );
    m_volume = (float)volume / 100.0;
}


void
RtAudioPlayback::initAudio(
    long sampleRate,
    int channels )
{
    //int channels = 2;
    //int sampleRate = 44100;

    int nBuffers = 16;
    int bufferSize = 512;

    try
    {
        RtAudio::RtAudioApi api = RtAudio::UNSPECIFIED;
        RtAudioFormat format = RTAUDIO_SINT16;
        m_audio = new RtAudio();

        int card = internalSoundCardID( m_deviceNum );

        #ifdef Q_WS_X11
        api = RtAudio::LINUX_ALSA;
        #endif

        RtAudioDeviceInfo info = m_audio->getDeviceInfo( card );
        delete m_audio;

        if ( info.nativeFormats & RTAUDIO_SINT32 )
        {
            format = RTAUDIO_SINT32;
        }
        if ( info.nativeFormats & RTAUDIO_SINT24 )
        {
            format = RTAUDIO_SINT24;
        }
        if ( info.nativeFormats & RTAUDIO_SINT16 )
        {
            format = RTAUDIO_SINT16;
        }

        m_audio = new RtAudio( card, channels, 0, 0, format, sampleRate, &bufferSize, nBuffers, api );
    }
    catch ( RtError &error )
    {
        LOGL( 1, "Initialising RtAudio failed. RtAudio error type: " << error.getType() <<
                 " Message: " << error.getMessage() );

        // Don't delete m_audio here or it will crash.
        // We don't emit an error signal here, it happens on startPlayback if
        // we have no m_audio.
        m_audio = 0;

        return;
    }
}


int
RtAudioPlayback::bufferSize()
{
    m_mutex.lock(); 
    int size = m_buffer.size();
    m_mutex.unlock();

    return size;
}


void
RtAudioPlayback::clearBuffers()
{
    m_mutex.lock();
    m_buffer.clear();
    m_mutex.unlock();
}


int
RtAudioPlayback::internalSoundCardID( int settingsID )
{
    if ( !m_audio )
        return -1;

    if ( settingsID < 0 )
        settingsID = 0;

    try
    {
        int card = 0;

        for ( int i = 1; i <= m_audio->getDeviceCount(); i++ )
        {
            RtAudioDeviceInfo info;
            info = m_audio->getDeviceInfo( i );
            if ( info.outputChannels > 0 )
            {
                if ( card++ == settingsID )
                    return i;
            }
        }
    }
    catch ( RtError &error )
    {
        LOGL( 1, "Getting internal soundcard ID failed. RtAudio error type: " << error.getType() <<
                 " Message: " << error.getMessage() );
    }

    #ifdef Q_WS_MAC
        return 3;
    #endif
    return 1;
}


Q_EXPORT_PLUGIN2( output_rtaudio, RtAudioPlayback )
