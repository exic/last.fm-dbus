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

#ifndef FINGERPRINTQUERYER_H
#define FINGERPRINTQUERYER_H

#include "Fingerprinter2.h"

#include "TrackInfo.h"
#include "WebService/FingerprintQueryRequest.h"

#include "FingerprintDllExportMacro.h"

#include <QObject>
#include <QFileInfo>

/**
 *   @author <erik@last.fm>
 *   @brief Assigns the fpId to the TrackInfo object
 *   Max and Jono summarising this:
 *   Checks Collection::instance() for a fingerprint id for this track based on track.path()
 *   If exists, does nothing
 *   If we have no fpId we then generate the short id and submit it to last.fm
 *
 *   Thus the name of this class is crap :P --mxcl
 *
 **/
class FINGERPRINT_DLLEXPORT FingerprintQueryer : public QObject
{
    Q_OBJECT
    
    public:
    
        enum NetworkErrors
        {
            ServersBusy = 0,
            RequestAborted,
            BadRequest,
            OtherError
        };

        FingerprintQueryer( QObject* parent = 0 );
        ~FingerprintQueryer();
        
        void setUsername( QString user ) { m_username = user; }
        void setPasswordMd5( QString pass ) { m_passwordMd5 = pass; }
        void setPasswordMd5Lower( QString passl ) { m_passwordMd5Lower = passl; }
        void setVersion( QString v ) { m_version = v; }
        
        bool isStopped();
        
    public slots:
        void fingerprint( TrackInfo& );
        
        void stop();
        
    signals:
        void trackFingerprintingStarted( TrackInfo );
        /** thanks for ths documentation */
        void trackFingerprinted( TrackInfo, bool fullFpRequested = false );
        void cantFingerprintTrack( TrackInfo track, QString reason );
        
        void started();
        void stopped();
        void resumed();
        void networkError( FingerprintQueryer::NetworkErrors, QString );

    protected:

        bool tryStartThread();
        
        QMutex m_dbMutex;
        QMutex m_queueMutex;
        QMutex m_ongoingRequestsMutex;
        QMutex m_networkErrorEmitMutex;
        
        QMutex m_tryStartMutex;
    
    protected slots:
        void onThreadFinished( Fingerprinter2* );
        void onFingerprintQueryReturn( Request* req );
        
    private:
        void setFpId( QString id, bool fullFpRequested = false );

        Fingerprinter2* m_currentFinger;
        FingerprintQueryRequest* m_currentQuery;

        TrackInfo m_track;
        QString m_username, m_passwordMd5, m_passwordMd5Lower, m_version;
        bool m_stop;
        int m_serversBusyCounter;
        int m_networkFailCounter;
};

#endif
