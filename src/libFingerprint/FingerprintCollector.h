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

#ifndef FINGERPRINTCOLLECTOR_H
#define FINGERPRINTCOLLECTOR_H

#include <QObject>
#include <QQueue>
#include <QFileInfo>

#include "Fingerprinter2.h"

#include "TrackInfo.h"
#include "WebService/Request.h"

#include "FingerprintDllExportMacro.h"

struct FINGERPRINT_DLLEXPORT FingerprintReturnInfo
{
    FingerprintReturnInfo( int tToFingerprint = 0,
                           int tSkipped = 0,
                           int tWithErrors = 0 ) :
                           tracksToFingerprint( tToFingerprint ),
                           tracksSkipped( tSkipped ),
                           tracksWithErrors( tWithErrors ) {}
    int tracksToFingerprint;
    int tracksSkipped;
    int tracksWithErrors;
};

/**
 *   @author Peter Grundström <petgru@last.fm>
 **/
class FINGERPRINT_DLLEXPORT FingerprintCollector : public QObject
{
    Q_OBJECT
    
    public: enum NetworkErrors
    {
        ServersBusy = 0,
        RequestAborted,
        BadRequest,
        OtherError
    };
    
    public:
        FingerprintCollector( int numberOfThreads = 1, QObject* parent = 0 );
        ~FingerprintCollector();
        
        void setUsername( QString user ) { m_username = user; }
        void setPasswordMd5( QString pass ) { m_passwordMd5 = pass; }
        void setPasswordMd5Lower( QString passl ) { m_passwordMd5Lower = passl; }
        
        QString username() { return m_username; }
        QString passwordMd5() { return m_passwordMd5; }
        QString passwordMd5Lower() { return m_passwordMd5Lower; }
        
        bool isStopped();
        int queueSize() { return queue().size(); }
        
    public slots:
        FingerprintReturnInfo fingerprint( QList<TrackInfo> );
        
        void stop();
        void pause();
        void resume();
        
    signals:
        void trackFingerprintingStarted( TrackInfo );
        void trackFingerprinted( TrackInfo );
        void cantFingerprintTrack( TrackInfo track, QString reason );
        
        void queueIsEmpty( );
        void started();
        void stopped( bool finished );
        void resumed();
        void networkError( FingerprintCollector::NetworkErrors, QString );

    protected:

        QQueue<TrackInfo>& queue() { QMutexLocker locker ( &m_queueMutex ); return m_queue; }

        QStringList& ongoingRequests() { QMutexLocker locker ( &m_ongoingRequestsMutex ); return m_ongoingRequests; }

        bool tryStartThreads();
        
        QMutex m_queueMutex;
        QMutex m_ongoingRequestsMutex;
        QMutex m_networkErrorEmitMutex;
        
        QMutex m_tryStartMutex;
    
    protected slots:
        void onThreadFinished( Fingerprinter2* );
        void onFingerprintSent( Request* req );
        
    private:
        QList<Fingerprinter2*> m_fingerprinters;
        QQueue<TrackInfo> m_queue; // these are the files that have not yet been sent for fingerprinting.
        QStringList m_ongoingRequests; // these are the files that is being fingerprintet right now.
        QString m_username,m_passwordMd5,m_passwordMd5Lower;
        bool m_stop;
        int m_serversBusyCounter;
        int m_networkFailCounter;
};

#endif
