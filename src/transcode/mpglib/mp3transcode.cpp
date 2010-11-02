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

#include "mp3transcode.h"
#include "logger.h"

#define MP3_MIN_BUFFER_SIZE 4096

struct mpstr_tag mpeg;

MP3Transcode::MP3Transcode() :
        m_decodedBufferCapacity( 32 * 1024 ),
        m_mpegInitialised( false )
{
    LOGL( 3, "Initialising MP3 Transcoding" );

    if ( !InitMP3( &mpeg ) )
    {
        LOGL( 1, "Something went wrong when initiliasing mpglib. "
            " God knows what. Chris can read the \"best documentation "
            " there is\" to find out. :P" );
    }
}


MP3Transcode::~MP3Transcode()
{
    ExitMP3( &mpeg );
}


QStringList
MP3Transcode::supportedMimeTypes() const
{
    return QStringList( "application/x-mp3" );
}

QStringList
MP3Transcode::supportedFileExtensions() const
{
    return QStringList( "mp3" );
}

bool
MP3Transcode::processData( const QByteArray &buffer )
{
    m_encodedBuffer.append( buffer );

    // wait till we got a minimal buffer of data, which we need for mpglib
    // being able to properly read the audio metadata (samplerate, channels...)
    if ( m_encodedBuffer.size() <= MP3_MIN_BUFFER_SIZE )
    {
        return true;
    }

    char tempBuffer[16384];
    int size;

    int result = decodeMP3(
        &mpeg, // handle
        (unsigned char*)m_encodedBuffer.data(), // in-data
        MP3_MIN_BUFFER_SIZE, // size of in-data
        tempBuffer, // out-data
        sizeof( tempBuffer ), // size of out-data
        &size ); // done size

    if ( result == MP3_ERR )
    {
        LOG( 1, "decodeMP3 failed. result: " << result );
        return false;
    }

    if ( !m_mpegInitialised )
    {
        long sampleRate = freqs[mpeg.fr.sampling_frequency];
        int channels = mpeg.fr.stereo > 0 ? mpeg.fr.stereo : 2;

        LOGL( 3, "mpegTranscode( Samplerate:" << sampleRate <<
                 " - Channels:" << channels << " )" );

        // For certain corrupt previews, we get a result of OK, but the sample
        // rate comes out all wrong. If we let these proceed, things will crash
        // horribly.
        if ( sampleRate != 44100 || channels != 2 )
        {
            LOGL( 1, "Stream is not 44.1k stereo, aborting" );
            return false;
        }

        m_mpegInitialised = true;
        emit streamInitialized( sampleRate, channels );
    }

    m_encodedBuffer.remove( 0, MP3_MIN_BUFFER_SIZE );
    while ( result == MP3_OK )
    {
        for ( int i = 0; i < ( size / 2 ); i++ )
        {
            m_decodedBuffer.append( tempBuffer[i * 2] );
            m_decodedBuffer.append( tempBuffer[i * 2 + 1] );
        }

        result = decodeMP3( &mpeg, NULL, 0, tempBuffer, sizeof( tempBuffer ), &size );

        if ( result == MP3_ERR )
        {
            LOGL( 1, "decodeMP3 failed. result: " << result );
            return false;
        }
    }

    qDebug() << "decbuf:" << m_decodedBuffer.count();

    return true;
}


void
MP3Transcode::clearBuffers()
{
    ExitMP3( &mpeg );

    m_encodedBuffer.clear();
    m_decodedBuffer.clear();
    m_mpegInitialised = false;

    if ( !InitMP3( &mpeg ) )
    {
        LOGL( 1, "Something went wrong when initiliasing mpglib. "
            " God knows what. Chris can read the \"best documentation "
            " there is\" to find out. :P" );
    }
}


bool
MP3Transcode::needsData()
{
    return m_decodedBuffer.size() < m_decodedBufferCapacity;
}


bool
MP3Transcode::hasData()
{
    return !m_decodedBuffer.isEmpty();
}


void
MP3Transcode::data( QByteArray& fillMe, int numBytes )
{
    fillMe = m_decodedBuffer.left( numBytes );
    m_decodedBuffer.remove( 0, numBytes );
}


Q_EXPORT_PLUGIN2( transcode, MP3Transcode )
