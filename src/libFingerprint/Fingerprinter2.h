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

#ifndef FINGERPRINTER2_H
#define FINGERPRINTER2_H

#include "TrackInfo.h"

#include "fplib/include/FingerprintExtractor.h"
#include "Sha256File.h"

#include <QThread>
#include <QMutex>
#include <QMutexLocker>

class QFile;
class TranscodeInterface;

/*****************************************************************************
 * @author <petgru@last.fm>
 * @author <erik@last.fm>
 *
 * Helperclass to do the fingerprinting.
 *****************************************************************************/
class Fingerprinter2 : public QThread
{
    Q_OBJECT
    
    public:

        enum Mode
        {
            Query,
            Full
        };

        Fingerprinter2( QObject* parent = 0 );
        
        void run();
        
        /*
            The thread won't start if isFree() returns false;
        */
        void startFullFingerprint();
        void startQueryFingerprint();
        
        Mode mode() { return m_mode; }

        void setTrack( TrackInfo track ) { QMutexLocker locker( &m_trackMutex ); m_track = track; }
        TrackInfo& track() { QMutexLocker locker( &m_trackMutex ); return m_track; }
        QByteArray& data() { QMutexLocker locker ( &m_dataMutex ); return m_fingerprint; }
        QString sha256 ();
        
        void stop();
        void reset();
        
        /*
            Use this to check whether this thread can be used again.
        */
        bool isFree();
        
    signals:
        void threadFinished( Fingerprinter2* _this );

    protected:
        //void fingerprintOld( QString path );
        void fingerprint( QString path );
        
        TrackInfo m_track;

        fingerprint::FingerprintExtractor m_extractor;
        QByteArray m_fingerprint;
        
        QMutex m_trackMutex;
        QMutex m_dataMutex;
        
    private:
        virtual void start();

        bool decode( QFile& inFile,
                     TranscodeInterface* transcoder,
                     bool skip );

        bool m_abort;
        bool m_reset;
        Mode m_mode;
        
        int m_sampleRate;
        int m_numChannels;

    
    private slots:
        void onStreamInitialized( long sampleRate, int channels );
        void onThreadFinished();
        

};

#endif

