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

#include "madtranscode.h"
#include "logger.h"

#include <QtPlugin>

#define MP3_MIN_BUFFER_SIZE 4096

typedef struct audio_dither
{
    mad_fixed_t error[3];
    mad_fixed_t random;
} audio_dither;


/* fast 32-bit pseudo-random number generator */
/* code from madplay */
static inline unsigned long prng( unsigned long state )
{
    return (state * 0x0019660dL + 0x3c6ef35fL) & 0xffffffffL;
}


/* dithers 24-bit output to 16 bits instead of simple rounding */
/* code from madplay */
static inline signed int dither( mad_fixed_t sample, audio_dither *dither )
{
    unsigned int scalebits;
    mad_fixed_t output, mask, random;

    enum
    {
        MIN = -MAD_F_ONE,
        MAX =  MAD_F_ONE - 1
    };

    /* noise shape */
    sample += dither->error[0] - dither->error[1] + dither->error[2];

    dither->error[2] = dither->error[1];
    dither->error[1] = dither->error[0] / 2;

    /* bias */
    output = sample + (1L << (MAD_F_FRACBITS + 1 - 16 - 1));

    scalebits = MAD_F_FRACBITS + 1 - 16;
    mask = (1L << scalebits) - 1;

    /* dither */
    random  = prng(dither->random);
    output += (random & mask) - (dither->random & mask);

    dither->random = random;

    /* clip */
    /* TODO: better clipping function */
    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;
    if (output >= MAD_F_ONE)
        output = MAD_F_ONE - 1;
    else if (output < -MAD_F_ONE)
        output = -MAD_F_ONE;

    /* quantize */
    output &= ~mask;

    /* error feedback */
    dither->error[0] = sample - output;

    /* scale */
    return output >> scalebits;
}


MadTranscode::MadTranscode() :
        m_decodedBufferCapacity( 32 * 1024 ),
        m_mpegInitialised( false )
{
    LOGL( 3, "Initialising MAD Transcoding" );

    mad_stream_init( &m_stream );
    mad_frame_init( &m_frame );
    mad_synth_init( &m_synth );
    mad_header_init( &m_madHeader );

    mad_timer_reset(&m_mad_timer);
}


MadTranscode::~MadTranscode()
{
    mad_synth_finish( &m_synth );
    mad_frame_finish( &m_frame );
    mad_stream_finish( &m_stream );
    mad_header_finish( &m_madHeader );
}


QStringList
MadTranscode::supportedMimeTypes() const
{
    return QStringList( "application/x-mp3" );
}


QStringList
MadTranscode::supportedFileExtensions() const
{
    return QStringList( "mp3" );
}


// FIXME: switching from nodecode to decode during the same track is currently broken!
// Heap corruption ensues on certain tracks.
bool
MadTranscode::processData( const QByteArray &buffer, bool noDecode )
{
    static audio_dither left_dither, right_dither;
    m_encodedBuffer.append( buffer );

    while ( m_encodedBuffer.count() >= MP3_MIN_BUFFER_SIZE )
    {
        mad_stream_buffer( &m_stream, (const unsigned char*)m_encodedBuffer.data(), MP3_MIN_BUFFER_SIZE );

        while ( true )
        {
            int err;
            if ( noDecode )
                err = mad_header_decode( &m_madHeader, &m_stream );
            else
                err = mad_frame_decode( &m_frame, &m_stream );

            if ( err != 0 )
            {
                if ( m_stream.error == MAD_ERROR_BUFLEN )
                {
                    if ( m_stream.next_frame != 0 )
                    {
                        size_t read = m_stream.next_frame - m_stream.buffer;
                        m_encodedBuffer.remove( 0, read );
                    }
                    else
                    {
                        size_t read = m_stream.bufend - m_stream.buffer;
                        m_encodedBuffer.remove( 0, read );
                    }
                    break;
                }
                else if ( m_stream.error != MAD_ERROR_LOSTSYNC )
                {
                    qDebug() << "libmad error:" << mad_stream_errorstr( &m_stream );
                }

                if ( !MAD_RECOVERABLE( m_stream.error ) )
                {
                    return false;
                }
                else
                {
                    err = 0;
                    continue;
                }
            }

            if ( noDecode )
            {
                /*
                mad_timer_add(&m_mad_timer, madHeader.duration);
                if ( mad_timer_count(m_mad_timer, MAD_UNITS_MILLISECONDS) >= 17500 )
                {
                    qDebug() << "17.5 ms";
                }
                */

                mad_timer_t time = m_madHeader.duration;
                float seconds = time.seconds + time.fraction * ( 1.0f / MAD_TIMER_RESOLUTION );
                int numChannels = m_madHeader.mode == MAD_MODE_SINGLE_CHANNEL ? 1 : 2;

                // Times 2 because each sample is 2 bytes (is this safe to assume?)
                int numBytes = (int)( seconds * m_madHeader.samplerate * numChannels * 2 );

                m_decodedBuffer.append( QByteArray( numBytes, 0 ) );
            }
            else
            {
                mad_synth_frame( &m_synth, &m_frame );

                if ( !m_mpegInitialised )
                {
                    long sampleRate = m_synth.pcm.samplerate;
                    int channels = m_synth.pcm.channels;

                    LOGL( 3, "madTranscode( Samplerate:" << sampleRate <<
                            " - Channels:" << channels << " )" );

                    m_mpegInitialised = true;
                    emit streamInitialized( sampleRate, channels > 0 ? channels : 2 );
                }

                for ( int i = 0; i < m_synth.pcm.length; i++ )
                {
                    union PCMDATA
                    {
                        short i;
                        unsigned char b[2];
                    } pcmData;

                    pcmData.i = dither( m_synth.pcm.samples[0][i], &left_dither );
                    m_decodedBuffer.append( pcmData.b[0] );
                    m_decodedBuffer.append( pcmData.b[1] );

                    if ( m_synth.pcm.channels == 2 )
                    {
                        pcmData.i = dither( m_synth.pcm.samples[1][i], &right_dither );
                        m_decodedBuffer.append( pcmData.b[0] );
                        m_decodedBuffer.append( pcmData.b[1] );
                    }

                } // end for each sample

            } // end !noDecode

        } // end while more frames

    } // end while more indata

    return true;
}


void
MadTranscode::clearBuffers()
{
    mad_synth_finish( &m_synth );
    mad_frame_finish( &m_frame );
    mad_stream_finish( &m_stream );
    mad_header_finish( &m_madHeader );

    m_encodedBuffer.clear();
    m_decodedBuffer.clear();
    m_mpegInitialised = false;

    mad_stream_init( &m_stream );
    mad_frame_init( &m_frame );
    mad_synth_init( &m_synth );
    mad_header_init( &m_madHeader );
}


bool
MadTranscode::needsData()
{
    return m_decodedBuffer.size() < m_decodedBufferCapacity;
}


bool
MadTranscode::hasData()
{
    return !m_decodedBuffer.isEmpty();
}


void
MadTranscode::data( QByteArray& fillMe, int numBytes )
{
    fillMe = m_decodedBuffer.left( numBytes );
    m_decodedBuffer.remove( 0, numBytes );
}


TranscodeInterface*
MadTranscode::newInstance()
{
    return new MadTranscode();
}


void
MadTranscode::deleteInstance( TranscodeInterface* i )
{
    Q_ASSERT( i != 0 );
    delete reinterpret_cast<MadTranscode*>( i );
}


Q_EXPORT_PLUGIN2( transcode, MadTranscode )
