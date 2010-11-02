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

#include "FingerprintQueryer.h"

#include "WebService/FingerprintQueryRequest.h"
#include "Settings.h"
#include "Collection.h"
#include "logger.h"
#include "FingerprintExtractor.h"

#include <QApplication>


FingerprintQueryer::FingerprintQueryer( QObject* parent ) :
    QObject( parent ),
    m_currentFinger( 0 )
{
    m_stop = false;
    m_serversBusyCounter = m_networkFailCounter = 0;
}


FingerprintQueryer::~FingerprintQueryer()
{}


void
FingerprintQueryer::fingerprint( TrackInfo& track )
{
    m_stop = false;

    if (track.source() != TrackInfo::Player)
    {
        qDebug() << "Track not from local media player; won't fingerprint.";
        emit cantFingerprintTrack( track, tr( "The track is not local" ) );
        return;
    }

    if (track.path().isEmpty())
    {
        qDebug() << "Track has an empty path; won't fingerprint";
        emit cantFingerprintTrack( track, tr( "The track has an empty path" ) );
        return;
    }

    if (!track.path().toLower().endsWith(".mp3"))
    {
        qDebug() << "We only support fingerprinting mp3's right now; won't fingerprint: " << track.path();
        emit cantFingerprintTrack( track, tr( "The track is not an mp3" ) );
        return;
    }

    QFileInfo fi( track.path() );
    if ( !fi.exists() )
    {
        qDebug() << "File: " << track.path() << " doesn't exist! Won't fingerprint.";
        emit cantFingerprintTrack( track, tr( "The track does not exist!" ) );
        return;
    }

    if ( !fi.isReadable() )
    {
        qDebug() << "File: " << track.path() << " is not readable, so we won't fingerprint.";
        emit cantFingerprintTrack( track, tr( "The track is not readable" ) );
        return;
    }
    
    int minLen = (int)( fingerprint::FingerprintExtractor::getMinimumDurationMs() / 1000 );
    if ( track.duration() < minLen )
    {
        qDebug() << "File: " << track.path() << " is too short (" << QString::number( track.duration() ) << " - min: " << minLen << "); we won't fingerprint.";
        emit cantFingerprintTrack( track, tr( "The track is too short" ) );
        return;
    }

    QString fpId = Collection::instance().getFingerprint( track.path() );
    
    m_track = track;

    if ( !fpId.isEmpty() )
    {
        qDebug() << "Fingerprint found in cache for" << track.toString();
        setFpId( fpId );
        return;
    }

    emit started();
    tryStartThread();

    return;
}


void
FingerprintQueryer::onThreadFinished( Fingerprinter2* fingerprinter )
{
    if ( fingerprinter != m_currentFinger )
    {
        // An old dangler, we don't care
        fingerprinter->deleteLater();
        return; 
    }
    
    if ( fingerprinter->data().size() == 0 )
    {
        qDebug() << "We got no fingerprint.";
        emit cantFingerprintTrack( fingerprinter->track(), tr( "Fingerprinting failed." ) );
        return;
    }
    
    m_currentQuery = new FingerprintQueryRequest( fingerprinter->track(), fingerprinter->data() );
    m_currentQuery->setSha256( fingerprinter->sha256() );
    m_currentQuery->setUsername( m_username );
    m_currentQuery->setPasswordMd5( m_passwordMd5 );
    m_currentQuery->setPasswordMd5Lower( m_passwordMd5Lower );
    m_currentQuery->setFpVersion( QString::number( fingerprint::FingerprintExtractor::getVersion() ) );
    m_currentQuery->setClientVersion( m_version );

    connect ( m_currentQuery, SIGNAL( result( Request* ) ), this, SLOT( onFingerprintQueryReturn( Request* ) ) );
    m_currentQuery->start();

    fingerprinter->deleteLater();
    m_currentFinger = 0;
}


void
FingerprintQueryer::onFingerprintQueryReturn(Request* req)
{
    FingerprintQueryRequest* queryReq = dynamic_cast<FingerprintQueryRequest*>( req );
    Q_ASSERT(queryReq);

    if ( queryReq != m_currentQuery )
    {
        // Old dangler, don't care
        return;
    }

    QMutexLocker emitLocker( &m_networkErrorEmitMutex );
    if ( queryReq->failed() )
    {
        qDebug() << "Network error: " << queryReq->errorMessage();
    
        // TODO: clean up these signals, they're weird
        if ( queryReq->aborted() )
            emit networkError( FingerprintQueryer::RequestAborted, QString() );
        else if ( queryReq->responseHeaderCode() == 400 )
        {
            emit cantFingerprintTrack( queryReq->track(), "Getting bad request with this track, skipping." );
            emit networkError( FingerprintQueryer::BadRequest, queryReq->errorMessage() );
        }
        else
            emit networkError( FingerprintQueryer::OtherError, queryReq->errorMessage() );

        return;
    }
    emitLocker.unlock();

    m_track = queryReq->track();
    QString id = queryReq->fpId();
    setFpId( id, queryReq->fullFpRequested() );

    Collection::instance().setFingerprint( m_track.path(), id );

    m_track = TrackInfo();
}


void
FingerprintQueryer::setFpId( QString id, bool fullFpRequested )
{
    m_track.setFpId( id );
    emit trackFingerprinted( m_track, fullFpRequested );
}


bool
FingerprintQueryer::tryStartThread()
{
    QMutexLocker locker_q( &m_queueMutex );
    QMutexLocker locker_try( &m_tryStartMutex );

    // Abort on-going fp operation, if any
    stop();

    m_currentFinger = new Fingerprinter2( this );

    connect( m_currentFinger, SIGNAL( threadFinished( Fingerprinter2* ) ),
             this,            SLOT( onThreadFinished( Fingerprinter2* ) ), Qt::QueuedConnection );

    m_currentFinger->setTrack( m_track );
    m_currentFinger->startQueryFingerprint();
    m_currentFinger->setPriority( QThread::IdlePriority );
        
    emit trackFingerprintingStarted( m_track );

    return true;
}


void
FingerprintQueryer::stop()
{
    if ( m_currentFinger != 0 )
        m_currentFinger->stop();
}
