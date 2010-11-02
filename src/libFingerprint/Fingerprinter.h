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

#ifndef FINGERPRINTER_H
#define FINGERPRINTER_H

#include <QThread>
#include <QMutex>
#include <QMutexLocker>

#include "TrackInfo.h"

#include "recommendation-commons/utils/Sha256File.h"
#include "fingerprint/KeyGenerator.h"

/*****************************************************************************
 * @author <petgru@last.fm>
 *
 * Helperclass to do the fingerprinting.
 *****************************************************************************/
class Fingerprinter : public QThread
{
    Q_OBJECT
    
    signals:
        void threadFinished( Fingerprinter* _this );

    public:
        Fingerprinter();
        
        void run();
        
        /*
            The thread won't start if isFree() returns false;
        */
        void start();
        
        void setTrack( TrackInfo track ) { QMutexLocker locker( &m_trackMutex ); m_track = track; }
        TrackInfo& track() { QMutexLocker locker( &m_trackMutex ); return m_track; }
        QByteArray& data() { QMutexLocker locker ( &m_dataMutex ); return m_data; }
        QString sha256 ();
        
        void reset();
        
        /*
            Use this to check whether this thread can be used again.
        */
        bool isFree();
        
    protected:
        void fingerprint( QString path );
        
        TrackInfo m_track;
        QByteArray m_data;
        
        QMutex m_trackMutex;
        QMutex m_dataMutex;
        
    private:
    
        fingerprint::KeyGenerator m_kgen;
        bool m_reset;
    
    private slots:
        void onThreadFinished();
};

#endif

