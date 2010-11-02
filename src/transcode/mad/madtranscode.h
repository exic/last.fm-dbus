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

#ifndef MP3TRANSCODE_H
#define MP3TRANSCODE_H

#include "interfaces/TranscodeInterface.h"
#include <mad.h>
#include <QByteArray>


/*************************************************************************/ /**
    Responsible for decoding an MP3 stream.
    
    This class is not thread-safe since it's used as part of a separate
    audio thread managed by the AudioController.fourht n
******************************************************************************/
class MadTranscode : public TranscodeInterface
{
    Q_OBJECT
    Q_INTERFACES( TranscodeInterface )

    public:
        MadTranscode();
        virtual ~MadTranscode();

        virtual QStringList
        supportedMimeTypes() const;
        
        virtual QStringList
        supportedFileExtensions() const;

        virtual bool
        needsData();
        
        virtual bool
        hasData();

        virtual void
        data( QByteArray& fillMe, int numBytes );

        virtual void
        setBufferCapacity( int bytes ) { m_decodedBufferCapacity = bytes; }

        int
        bufferSize() { return m_decodedBuffer.size(); }

        virtual TranscodeInterface*
        newInstance();

        virtual void
        deleteInstance( TranscodeInterface* i );

    public slots:
        virtual void clearBuffers();

        // Setting noDecode to true causes only the frame headers to be read and the
        // stream to advance without decoding. FIXME: BROKEN!
        virtual bool processData( const QByteArray& data, bool noDecode = false );
        
    signals:
        void streamInitialized( long sampleRate, int channels );

    private:
        QByteArray m_encodedBuffer;
        QByteArray m_decodedBuffer;
        int m_decodedBufferCapacity;

        bool m_mpegInitialised;
        struct mad_decoder m_decoder;
        struct mad_stream m_stream;
        struct mad_frame m_frame;
        struct mad_synth m_synth;
        struct mad_header m_madHeader;

        mad_timer_t m_mad_timer;

};

#endif
