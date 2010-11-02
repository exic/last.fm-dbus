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

#ifndef SCROBBLERSUBMITTER_H
#define SCROBBLERSUBMITTER_H

#include <QtGui>
#include "http.h"

#include <TrackInfo.h>

class ScrobblerSubmitter : public QObject
{
    Q_OBJECT

    public:
        // Will probably need more but these will do for now
        enum StatusCode
        {
            Info,
            InfoTrack,
            ScrobbledOK,
            ScrobbleFailed,
            BadAuth
        };

        static QString XML_VERSION;
        static QString PROTOCOL_VERSION;
        static QString CLIENT_ID;
        static QString CLIENT_VERSION;
        static QString HANDSHAKE_HOST;

        ScrobblerSubmitter( QObject* parent );
        ~ScrobblerSubmitter();

        // Call to re-initialise after construct time
        void init( const QString& username, const QString& password, const QString& version );

    public slots:
        void submitItem( TrackInfo item );

    signals:
        void scrobbled(); 
        void statusChanged( int status, QString msg );
        void submitThreadHandOver();

    private:
        // on failure, start at MIN_BACKOFF, and double on subsequent failures
        // until MAX_BACKOFF is reached
        static const int MIN_BACKOFF = 60;
        static const int MAX_BACKOFF = 60 * 60;

        void handshake();
        void submit();

        void saveSubmitQueue();
        void readSubmitQueue( QString path, TrackInfo::Source source = TrackInfo::Player );

        bool canSubmit() const;
        bool schedule( bool failure );

//         QString m_submitResultBuffer;
        uint m_backoff;
        QTimer m_timer;
        QString m_savePath;

        QString m_username;
        QString m_password;
        QString m_challenge;
        QUrl    m_submitUrl;

        Http m_http;
        int m_handshakeId;
        Http m_submitHttp;
        //Http* mp_submitHttp;
        int m_submitID;

        QList<TrackInfo> m_submitQueue;
        QList<TrackInfo> m_progressQueue;

        bool m_inProgress;
        bool m_needHandshake;
        uint m_prevSubmitTime;
        uint m_lastSubmissionFinishTime;
        uint m_interval;

        int m_submitCounter;

    private slots:
        void submitItemInThread();
        void handshakeFinished( int id, bool error );
        void submitFinished( int id, bool error );
        void scheduledTimeReached();
};


#endif /* SCROBBLER_H */
