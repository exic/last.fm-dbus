/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
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

#include "Fingerprinter2.h"

#include "interfaces/TranscodeInterface.h"

#include "logger.h"
#include "Sha256File.h"
#include "Sha256.h"
#include "MP3_Source_Qt.h"

#include <QFile>
#include <QDateTime>


using namespace fingerprint;

static const uint k_bufferSize = 1024 * 8;

Fingerprinter2::Fingerprinter2( QObject* parent ) :
    QThread( parent ),
    m_abort( false ),
    m_mode( Query )
{
    connect( this, SIGNAL( finished() ), SLOT( onThreadFinished() ) );
    reset();
}

// private slot:
void Fingerprinter2::onThreadFinished()
{
    emit threadFinished( this );
}

void Fingerprinter2::reset()
{
    m_fingerprint = QByteArray();
    m_track = TrackInfo();
    
    m_abort = false;
    m_reset = true;
}

bool Fingerprinter2::isFree()
{
    return !isRunning() && m_reset;
}


void Fingerprinter2::startFullFingerprint()
{
    m_mode = Full;
    start();
}


void Fingerprinter2::startQueryFingerprint()
{
    m_mode = Query;
    start();
}


void Fingerprinter2::stop()
{
    m_abort = true;
}


void Fingerprinter2::start()
{
    if (isFree())
    {
        m_reset = false;
        QThread::start();
    }
    else
    {
        Q_ASSERT( !"Error: Fingerprinter2-thread cannot start since it is not free. Always check isFree() first!" );
    }
}

void Fingerprinter2::run()
{
    //qDebug() << "Fingerprinting thread started.";
    fingerprint(m_track.path());
}

QString Fingerprinter2::sha256()
{
    QMutexLocker locker( &m_trackMutex );
    unsigned char hash[SHA256_HASH_SIZE];
    QString sha;
    Sha256File::getHash( m_track.path().toStdString(), hash );
    
    for (int i = 0; i < SHA256_HASH_SIZE; ++i) {
        QString hex = QString("%1").arg(uchar(hash[i]), 2, 16,
                                        QChar('0'));
        sha.append(hex);
    }
    
    return sha;
}


/*
void Fingerprinter2::fingerprintOld( QString filename )
{
    //int time = QDateTime::currentDateTime().toTime_t();
    
    QString fileExt = QFileInfo( filename ).suffix();
    TranscodeInterface* transcoder = TranscodeInterface::getDecoder( fileExt );

    // For all failures in this class, the caller will have to check whether no
    // fingerprint data was produced and deduce failure.
    if ( transcoder == 0 )
    {
        qDebug() << "Failed to find transcoder for extension:" << fileExt;
        return;
    }
    
    TranscodeInterface* transcoder2 = transcoder->newInstance();
    if ( transcoder2 == 0 )
    {
        qDebug() << "Failed to create transcoder2";
        return;
    }
    
    QFile inFile( filename );
    bool fine = inFile.open( QIODevice::ReadOnly );
    if ( !fine )
    {
        qDebug() << "Failed to open file:" << filename;
        transcoder->deleteInstance( transcoder2 );
        return;
    }
    
    //QFile outFile( MooseUtils::savePath( "out.wav" ) );
    //outFile.open( QIODevice::WriteOnly | QIODevice::Unbuffered );

    m_sampleRate = -1;
    m_numChannels = -1;

    // We can't start fingerprinting until the transcoder has called back
    // with the correct samplerate and channel count
    connect( transcoder2, SIGNAL( streamInitialized( long, int ) ),
             this,        SLOT( onStreamInitialized( long, int ) ), Qt::DirectConnection );

    qDebug() << "--- Starting FP for: " << filename;

    bool fpDone = false;

    // Decode until the decoder sends us the streamInitialized signal letting
    // us know what the sample rate is
    while ( m_sampleRate == -1 && fine && !inFile.atEnd() )
    {
        fine = decode( inFile, transcoder2, false );

        qDebug() << "fp decode loop " << inFile.pos();
    }

    if ( inFile.atEnd() )
    {
        fine = false;
        qDebug() << "Caught file that never returned samplerate: " << filename;
    }

    if ( fine )
    {
        QByteArray data;
        
        uint bytesToSkip;
        uint bytesRead = 0;
        if ( mode() == Full )
        {
            m_extractor.initForFullSubmit( m_sampleRate, m_numChannels );
            bytesToSkip = 0;    
        }
        else
        {
            m_extractor.initForQuery( m_sampleRate, m_numChannels );

            // Skippety skip for as long as the skipper sez (optimisation)
            //inFile.reset();
            float secsToSkip = m_extractor.getToSkipMs() / 1000.0f;
            bytesToSkip = ( secsToSkip * m_sampleRate * m_numChannels * 2 ) - // 1 sample = 2 bytes
                          ( 10 * k_bufferSize ); // safety

            qDebug() << "Will skip " << bytesToSkip << " bytes";
        }

        bool skipping = bytesRead < bytesToSkip;
        while ( !fpDone && !inFile.atEnd() )
        {
            if ( m_abort ) break;

            fine = decode( inFile, transcoder2, skipping );
            if ( !fine )
                break;

            try
            {
                while ( transcoder2->hasData() && !fpDone )
                {
                    transcoder2->data( data, k_bufferSize );

                    bytesRead += data.size();
                    skipping = bytesRead < bytesToSkip;
                    
                    //qDebug() << "bytes read: " << bytesRead << ", skip: " << skipping;
                    //qDebug() << "data start address: " << (void*)data.data();
                    //qDebug() << "data end address: " << (void*)(data.data() + data.size());

                    fpDone = m_extractor.process(
                        reinterpret_cast<const short*>( data.data() ),
                        data.size() / sizeof( short ),
                        inFile.atEnd() && !transcoder2->hasData() );

                    //outFile.write( data );
                }
            }
            catch ( const std::exception& e )
            {
                qDebug() << "Fingerprinter failed: " << e.what();
                break;
            }

        } // end outer while

    } // end if fine

    if ( fpDone )
    {
        // We succeeded
        std::pair<const char*, size_t> fpData = m_extractor.getFingerprint();
        m_fingerprint = QByteArray::fromRawData( fpData.first, fpData.second );
    }
    else
    {
        qDebug() << "FingerprintExtractor::process never returned true, fingerprint not calculated";
        m_fingerprint.clear();
    }

    inFile.close();
    transcoder->deleteInstance( transcoder2 );
}
*/

void Fingerprinter2::fingerprint( QString filename )
{
    //int time = QDateTime::currentDateTime().toTime_t();
    
    int duration, samplerate, bitrate, nchannels;
    MP3_Source ms;

    try
    {
        MP3_Source::getInfo( filename, duration, samplerate, bitrate, nchannels );

        m_sampleRate = samplerate;
        m_numChannels = nchannels;

        ms.init( filename );
    }
    catch ( std::exception& e )
    {
        qDebug() << "Failed to read file: " << filename;
        qDebug() << "MP3_Source error: " << e.what();
        return;
    }
    
    ms.skipSilence();

    bool fpDone = false;
    try
    {
        if ( mode() == Full )
        {
            qDebug() << "*** Starting full FP for: " << filename;
            m_extractor.initForFullSubmit( m_sampleRate, m_numChannels );
        }
        else
        {
            qDebug() << "--- Starting query FP for: " << filename;
            m_extractor.initForQuery( m_sampleRate, m_numChannels, duration );

            // Skippety skip for as long as the skipper sez (optimisation)
            ms.skip( m_extractor.getToSkipMs() );
            float secsToSkip = m_extractor.getToSkipMs() / 1000.0f;
            fpDone = m_extractor.process(
                0,
                static_cast<size_t>( m_sampleRate * m_numChannels * secsToSkip ),
                false );
        }
    }
    catch ( const std::exception& e )
    {
        qDebug() << "Fingerprinter failed during initialisation: " << e.what();
        return;
    }
    
    const size_t PCMBufSize = 131072; 
    short* pPCMBuffer = new short[PCMBufSize];

    while ( !fpDone )
    {
        if ( m_abort )
            break;

        size_t readData = ms.updateBuffer( pPCMBuffer, PCMBufSize );
        if ( readData == 0 )
            break;

        try
        {
            fpDone = m_extractor.process( pPCMBuffer, readData, ms.eof() );
        }
        catch ( const std::exception& e )
        {
            qDebug() << "Fingerprinter failed: " << e.what();
            break;
        }

    } // end while

    delete[] pPCMBuffer;

    if ( !fpDone )
    {
        qDebug() << "FingerprintExtractor::process never returned true, fingerprint not calculated";
        m_fingerprint.clear();
        return;
    }
    
    try
    {
        // We succeeded
        std::pair<const char*, size_t> fpData = m_extractor.getFingerprint();
        m_fingerprint = QByteArray( fpData.first, fpData.second );
    }
    catch ( const std::exception& e )
    {
        qDebug() << "Fingerprint failed at getFingerprint: " << e.what();
        m_fingerprint.clear();
    }

}


void Fingerprinter2::onStreamInitialized( long sampleRate, int channels )
{
    m_sampleRate = sampleRate;
    m_numChannels = channels;
}


bool Fingerprinter2::decode( QFile& inFile, TranscodeInterface* transcoder2, bool skip )
{
    char buffer[k_bufferSize];

    int numBytes = inFile.read( buffer, k_bufferSize );
    if ( numBytes == -1 )
    {
        qDebug() << "Failed to read data from file.";
        return false;
    }

    // This avoids making a deep copy into the QByteArray
    QByteArray ba = QByteArray::fromRawData( buffer, numBytes );
    bool fine = transcoder2->processData( ba, skip );
    if ( !fine )
    {
        qDebug() << "The encoder choked on the data in file.";
        return false;
    }
    
    return true;
}
