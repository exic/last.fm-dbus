/***************************************************************************
*   Copyright (C) 2005 - 2007 by                                          *
*      Jono Cole, Last.fm Ltd <jono@last.fm>                              *
*      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                 *
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

#ifndef DIAGNOSTICSDIALOG_H
#define DIAGNOSTICSDIALOG_H

#include <QTimer>
#include <iostream>
#include <fstream>

#include "ui_DiagnosticsDialog.h"

#include "TrackInfo.h"

class DiagnosticsDialog : public QDialog
{
    Q_OBJECT

public:
    DiagnosticsDialog( QWidget *parent = 0 );
    ~DiagnosticsDialog( void );

    Ui::DiagnosticsDialog ui;

    void show();

private:
    void populateCacheList( const QString& username );
	void scrobbleIpod( bool isManual = false );

    class QTimer* m_logTimer;
    std::ifstream m_logFile;

public slots:
    void reconnect();

private slots:
    void close();

    void onHttpBufferSizeChanged( int bufferSize );
    void onDecodedBufferSizeChanged( int bufferSize );
    void onOutputBufferSizeChanged( int bufferSize );

    void radioHandshakeReturn( class Request* );
    void onAppEvent( int event, const QVariant& );
    void onScrobblerEvent( int );

    void onRefresh();
    void onCopyToClipboard();

    void onTrackFingerprintingStarted( TrackInfo );
    void onTrackFingerprinted( TrackInfo );
    void onCantFingerprintTrack( TrackInfo track, QString reason );

	void onScrobbleIpodClicked();
	void onLogPoll();
};

#endif //DIAGNOSTICSDIALOG_H
