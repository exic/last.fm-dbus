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

#include "FingerprintCollector.h"

#include "WebService/SubmitFullFingerprintRequest.h"
#include "Settings.h"
#include "logger.h"
#include "FingerprintExtractor.h"

#include <QApplication>


FingerprintCollector::FingerprintCollector( int numberOfThreads, QObject* parent )
                     : QObject( parent )
{
    for( int i = 0; i < numberOfThreads; i++ )
    {
        Fingerprinter2* finger = new Fingerprinter2();
        
        connect( finger, SIGNAL( threadFinished( Fingerprinter2* ) ),
                 this, SLOT( onThreadFinished( Fingerprinter2* ) ), Qt::QueuedConnection );
        
        m_fingerprinters << finger;
    }
    
    m_stop = false;
    m_serversBusyCounter = m_networkFailCounter = 0;
}


FingerprintCollector::~FingerprintCollector()
{
}


FingerprintReturnInfo
FingerprintCollector::fingerprint( QList<TrackInfo> scrobbledTracks )
{
    m_stop = false;
    if (scrobbledTracks.size() == 0)
        return FingerprintReturnInfo();

    m_ongoingRequestsMutex.lock();
    m_queueMutex.lock();

    FingerprintReturnInfo retinfo;

    foreach( TrackInfo track, scrobbledTracks )
    {
        if (track.source() != TrackInfo::Player)
        {
            qDebug() << "Track not from local media player; won't fingerprint.";
            emit cantFingerprintTrack( track, tr( "The track is not local" ) );
            retinfo.tracksWithErrors++;
            continue;
        }

        if (track.path().isEmpty())
        {
            qDebug() << "Track has an empty path; won't fingerprint";
            emit cantFingerprintTrack( track, tr( "The track has an empty path" ) );
            retinfo.tracksWithErrors++;
            continue;
        }

        if (!track.path().toLower().endsWith(".mp3"))
        {
            qDebug() << "We only support fingerprinting mp3's right now; won't fingerprint: " << track.path();
            emit cantFingerprintTrack( track, tr( "The track is not an mp3" ) );
            retinfo.tracksWithErrors++;
            continue;
        }

        QFileInfo fi( track.path() );
        if ( !fi.exists() )
        {
            qDebug() << "File: " << track.path() << " doesn't exist! Won't fingerprint.";
            emit cantFingerprintTrack( track, tr( "The track does not exist!" ) );
            retinfo.tracksWithErrors++;
            continue;
        }

        if ( !fi.isReadable() )
        {
            qDebug() << "File: " << track.path() << " is not readable, so we won't fingerprint.";
            emit cantFingerprintTrack( track, tr( "The track is not readable" ) );
            retinfo.tracksWithErrors++;
            continue;
        }
        
        QString path = track.path();
        
        int minLen = (int)( fingerprint::FingerprintExtractor::getMinimumDurationMs() / 1000 );
        if ( track.duration() < minLen )
        {
            qDebug() << "File: " << track.path() << " is too short (" << QString::number( track.duration() ) << " - min: " << minLen << "); we won't fingerprint.";
            emit cantFingerprintTrack( track, tr( "The track is too short" ) );
            retinfo.tracksWithErrors++;
            continue;
        }

        if (m_ongoingRequests.contains( track.path() ))
        {
            qDebug() << "Track is currently being sent; won't fingerprint";
            retinfo.tracksSkipped++;
            continue;
        }

        retinfo.tracksToFingerprint++;

        m_queue.enqueue( track );
    }

    m_ongoingRequestsMutex.unlock();
    m_queueMutex.unlock();

    emit started();

    tryStartThreads();

    return retinfo;
}


void
FingerprintCollector::onThreadFinished( Fingerprinter2* fingerprinter )
{
    if ( fingerprinter->data().size() == 0 )
    {
        qDebug() << "Error during fingerprinting. Don't send";
        emit cantFingerprintTrack( fingerprinter->track(), tr( "Fingerprinting failed, skipping." ) );
        fingerprinter->reset();
        tryStartThreads();
        return;
    }
    
    //qDebug() << "Fingerprinting thread finished.";
    SubmitFullFingerprintRequest* req = new SubmitFullFingerprintRequest(fingerprinter->track(),fingerprinter->data());
    req->setSha256( fingerprinter->sha256() );
    req->setUsername( username() );
    req->setPasswordMd5( passwordMd5() );
    req->setPasswordMd5Lower( passwordMd5Lower() );
    req->setFpVersion( QString::number( fingerprint::FingerprintExtractor::getVersion() ) );
    connect ( req, SIGNAL( result( Request* ) ), this, SLOT (onFingerprintSent( Request* ) ) );

    req->start();
    fingerprinter->reset();
}


void
FingerprintCollector::onFingerprintSent( Request* req )
{
    SubmitFullFingerprintRequest* submitreq = dynamic_cast<SubmitFullFingerprintRequest*>( req );
    Q_ASSERT(submitreq);

    QMutexLocker emitLocker( &m_networkErrorEmitMutex );
    if ( req && req->failed() )
    {
        qDebug() << "Network error: " << submitreq->errorMessage();

        if ( req->aborted() )
            emit networkError( FingerprintCollector::RequestAborted, QString() );

        else if ( req->responseHeaderCode() == 400 )
        {
            emit cantFingerprintTrack( submitreq->track(), tr( "Getting bad request with this track, skipping." ) );
            emit networkError( FingerprintCollector::BadRequest, submitreq->errorMessage() );
        }
        else
            emit networkError( FingerprintCollector::OtherError, submitreq->errorMessage() );

        // Removed this because it caused several result signals to be emitted which led to
        // deadlocks when the FingerprinterApplication tried to grab a mutex.
        //if (!m_stop)
        //{
        //    qDebug() << "Resending fingerprint";
        //    SubmitFingerprintRequest* newsubmitreq = new SubmitFingerprintRequest( submitreq->track(), submitreq->data());
        //    newsubmitreq->setSha256( submitreq->sha256() );
        //    newsubmitreq->setUsername( username() );
        //    newsubmitreq->setPasswordMd5( passwordMd5() );
        //    newsubmitreq->setPasswordMd5Lower( passwordMd5Lower() );
        //    newsubmitreq->setFpVersion( FINGERPRINT_LIB_VERSION );
        //    connect ( newsubmitreq, SIGNAL( result( Request* ) ), this, SLOT (onFingerprintSent( Request* ) ) );
        //    newsubmitreq->start();
        //}
        return;
    }
    emitLocker.unlock();

    emit trackFingerprinted( submitreq->track() );
    //qDebug() << "Fingerprint has been sent, writing to DB";

    tryStartThreads();

    QMutexLocker locker_pr ( &m_ongoingRequestsMutex );

    m_ongoingRequests.removeAt( m_ongoingRequests.indexOf( submitreq->track().path() ) );
}


bool
FingerprintCollector::tryStartThreads()
{
    QMutexLocker locker_q( &m_queueMutex );
    QMutexLocker locker_try( &m_tryStartMutex );

    if ( m_queue.isEmpty() )
    {
        locker_q.unlock();
        locker_try.unlock();

        emit queueIsEmpty();

        foreach( Fingerprinter2* fingerprinter, m_fingerprinters )
        {
            if (!fingerprinter->isFree())
                return false;
        }

        emit stopped( true /*finished*/  );
        return false;
    }

    if ( m_stop )
    {
        locker_q.unlock();
        locker_try.unlock();

        foreach( Fingerprinter2* fingerprinter, m_fingerprinters )
        {
            if (!fingerprinter->isFree())
                return false;
        }

        emit stopped( false /*stopped, but not finished*/  );
        return false;
    }

    QMutexLocker locker_pr ( &m_ongoingRequestsMutex );

    bool ret = false;
    foreach( Fingerprinter2* fingerprinter, m_fingerprinters )
    {
        if ( m_queue.isEmpty() )
        {
            emit queueIsEmpty( );
            break;
        }

        if ( !fingerprinter->isFree() )
        {
            continue;
        }
        else 
        {
            //qDebug() << "Fingerprint queue is not empty, starting thread";
            TrackInfo track = m_queue.dequeue();
            m_ongoingRequests.append( track.path() );

            fingerprinter->setTrack( track );
            fingerprinter->startFullFingerprint();
            fingerprinter->setPriority( QThread::IdlePriority );
            ret = true;
            
            emit trackFingerprintingStarted( track );
        }
    }
    return ret;
}


void
FingerprintCollector::stop()
{
    QMutexLocker locker_q( &m_queueMutex );
    QMutexLocker locker_pr( &m_ongoingRequestsMutex );
    m_queue.clear();
    m_ongoingRequests.clear();
    m_stop = true;
    if ( isStopped() )
        emit stopped( true );
}


void
FingerprintCollector::pause()
{
    m_stop = true;
    if ( isStopped() )
        emit stopped( !m_queue.isEmpty() );
}


void
FingerprintCollector::resume()
{
    m_stop = false;

    tryStartThreads();

    emit resumed();
}


bool
FingerprintCollector::isStopped()
{
    foreach( Fingerprinter2* fingerprinter, m_fingerprinters )
    {
        if (!fingerprinter->isFree())
            return false;
    }
    return true;
}

