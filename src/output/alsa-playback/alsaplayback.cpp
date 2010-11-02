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

#include "alsaaudio.h"
#include "alsaplayback.h"
#include "Settings.h"
#include "logger.h"
#include <qplugin.h>

#include "RadioEnums.h"


float AlsaPlayback::m_volume = 0.5;


AlsaPlayback::AlsaPlayback()
        : m_audio( 0 )
        , m_deviceNum( 0 )
{
}


AlsaPlayback::~AlsaPlayback()
{
    delete m_audio;
}


bool
AlsaPlayback::hasData()
{
//     qDebug() << "APlayback:" << m_audio->hasData();
    bool has = m_audio->hasData() > 0;
    return has;
}


bool
AlsaPlayback::needsData()
{
    return ( m_audio->get_thread_buffer_filled() < m_bufferCapacity );
}


void
AlsaPlayback::setBufferCapacity( int size )
{
    m_bufferCapacity = size;
}


int
AlsaPlayback::bufferSize()
{
    return m_audio->get_thread_buffer_filled();
}


void
AlsaPlayback::setVolume( int volume )
{
    m_audio->setVolume( (float)volume / 100.0 );
}


QStringList
AlsaPlayback::soundSystems()
{
    return QStringList() << "Alsa";
}


QStringList
AlsaPlayback::devices()
{
    Q_DEBUG_BLOCK << "Querying audio devices";

    QStringList devices;
    for (int i = 0, n = m_audio->getCards(); i < n; i++)
        devices << m_audio->getDeviceInfo( i ).name;

//     qDebug() << "Names:" << devices;

    return devices;
}


void
AlsaPlayback::startPlayback()
{
    if ( !m_audio )
    {
        Q_DEBUG_BLOCK << "No AlsaAudio instance available.";
        goto _error;
    }

    if ( m_audio->startPlayback() )
    {
        Q_DEBUG_BLOCK << "Error starting playback.";
        goto _error;
    }

    return;

_error:
    // We need to send a stop signal to m_iInput here, otherwise
    // it will keep running and filling up the buffers even though
    // there is no available device.

    emit error( Radio_NoSoundcard, tr( "The ALSA soundsystem is either busy or not present." ) );
}


void
AlsaPlayback::stopPlayback()
{
    m_audio->stopPlayback();
}


void
AlsaPlayback::initAudio( long /*sampleRate*/, int /*channels*/ )
{
    int channels = 2;
    int sampleRate = 44100;
    int periodSize = 1024;  // According to mplayer, these two are good defaults.
    int periodCount = 16;   // They create a buffer size of 16384 frames.
    QString cardDevice;

    delete m_audio;
    m_audio = new AlsaAudio;
    m_audio->clearBuffer();

    cardDevice = internalSoundCardID( m_deviceNum );

    // We assume host byte order
#ifdef WORDS_BIGENDIAN
    if (!m_audio->alsaOpen( cardDevice, FMT_S16_BE, sampleRate, channels, periodSize, periodCount, m_bufferCapacity ))
#else
    if (!m_audio->alsaOpen( cardDevice, FMT_S16_LE, sampleRate, channels, periodSize, periodCount, m_bufferCapacity ))
#endif
    {
        // We need to send a stop signal to m_iInput here, otherwise
        // it will keep running and filling up the buffers even though
        // there is no available device.

        emit error( Radio_NoSoundcard, tr("The ALSA soundsystem is either busy or not present.") );
    }
}


void
AlsaPlayback::processData( const QByteArray &buffer )
{
    m_audio->alsaWrite( buffer );
}


void
AlsaPlayback::clearBuffers()
{
    m_audio->clearBuffer();
}


QString
AlsaPlayback::internalSoundCardID( int settingsID )
{
    int cards = m_audio->getCards();

    if ( settingsID < cards )
        return m_audio->getDeviceInfo( settingsID ).device;
    else
        return "default";
}


Q_EXPORT_PLUGIN2( output_alsa, AlsaPlayback )
