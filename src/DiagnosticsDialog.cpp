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

#include "Radio.h"
#include "WebService/Request.h"
#include "Scrobbler-1.2.h"
#include "DiagnosticsDialog.h"
#include "LastFmSettings.h"
#include "container.h"
#include "lastfmapplication.h"
#include "libMoose/LastFmSettings.h"
#include "libFingerprint/FingerprintCollector.h"

#include <QProcess>
#include <QClipboard>

#include <QByteArray>
#include <QTimer>
#include <QFile>


static void smallFontise( QWidget* w )
{
    #ifdef WIN32
    return; //small fonts look wrong on Windows
    #endif

    QFont f = w->font();
    #if defined LINUX
        f.setPointSize( f.pointSize() - 2 );
    #else
        f.setPointSize( 10 );
    #endif
    w->setFont( f );
}


DiagnosticsDialog::DiagnosticsDialog( QWidget *parent )
        : QDialog( parent )
{
    ui.setupUi( this );

    #ifdef HIDE_RADIO
    ui.radioGroupBox->setVisible( false );
    #endif
    
    #ifdef Q_WS_X11
        ui.tabWidget->removeTab( 3 );
    #endif

    // not possible to do this with designer, and varies by platform
    smallFontise( ui.cachedTracksLabel );
    smallFontise( ui.cachedTracksTitle );
    smallFontise( ui.fingerprintedTracksTitle );

    #ifndef LINUX
    // Qt 4.3.1 b0rked? as this is specified in Designer, but only works on Linux :(
    foreach ( QGroupBox* b, findChildren<QGroupBox*>() )
    {
        b->layout()->setMargin( 9 ); //Qt ignores our setting in Designer :(
        #ifdef WIN32
        if ( QSysInfo::WindowsVersion != QSysInfo::WV_VISTA )
            b->setFlat( false );
        #endif
    }
    #endif

    #ifdef Q_OS_MAC
    layout()->setMargin( 7 );
    delete ui.line;
    ui.cachedTracksList->setAttribute( Qt::WA_MacShowFocusRect, false );
    ui.vboxLayout1->setSpacing( 18 );
    #endif

    ui.httpBufferLabel->setMinimumWidth( ui.httpBufferProgress->fontMetrics().width( "100.0k" ) );

    ui.httpBufferProgress->setMaximum( 500000 );
    ui.decodedBufferProgress->setMaximum( 100000 );
    ui.outputBufferProgress->setMaximum( 100000 );

    connect( ui.closeButton, SIGNAL( clicked()), SLOT( close() ) );
    connect( qApp, SIGNAL( event( int, QVariant ) ), SLOT( onAppEvent( int, QVariant ) ) );
    connect( &The::scrobbler(), SIGNAL( status( int, QVariant ) ), SLOT( onScrobblerEvent( int ) ) );
    connect( ui.viewLogButton,  SIGNAL( clicked() ), &The::container(), SLOT( onAltShiftL() ) );
    connect( ui.refreshButton, SIGNAL( clicked() ), SLOT( onRefresh() ) );
    connect( ui.copyToClipboardButton, SIGNAL( clicked() ), SLOT( onCopyToClipboard() ) );

    connect( ui.scrobbleIpodButton, SIGNAL( clicked() ), SLOT( onScrobbleIpodClicked() ) );

    // Fingerprint collector
    ui.fpQueueSizeLabel->setText( "0" );
    connect( The::app().m_fpCollector, SIGNAL( trackFingerprintingStarted( TrackInfo ) ),
             this,                     SLOT( onTrackFingerprintingStarted( TrackInfo ) ),
             Qt::QueuedConnection );
    connect( The::app().m_fpCollector, SIGNAL( trackFingerprinted( TrackInfo ) ),
             this,                     SLOT( onTrackFingerprinted( TrackInfo ) ),
             Qt::QueuedConnection );
    connect( The::app().m_fpCollector, SIGNAL( cantFingerprintTrack( TrackInfo, QString ) ),
             this,                     SLOT( onCantFingerprintTrack( TrackInfo, QString ) ),
             Qt::QueuedConnection );

    m_logTimer = new QTimer( this );
    connect( m_logTimer, SIGNAL( timeout() ),
             this,       SLOT( onLogPoll() ) );
             
             
     ui.reconnect->parentWidget()->layout()->setAlignment( ui.reconnect, Qt::AlignRight );
     connect( ui.reconnect, SIGNAL(clicked()), SLOT(reconnect()) );

}


DiagnosticsDialog::~DiagnosticsDialog()
{}


void
DiagnosticsDialog::show()
{
    //initialize the progress bars to 0
    onDecodedBufferSizeChanged( 0 );
    onHttpBufferSizeChanged( 0 );
    onOutputBufferSizeChanged( 0 );

    connect( &The::audioController().m_thread, SIGNAL( httpBufferSizeChanged( int ) ),
        this,                              SLOT( onHttpBufferSizeChanged( int ) ),
        Qt::QueuedConnection );

    connect( &The::audioController().m_thread, SIGNAL( decodedBufferSizeChanged( int ) ),
        this,                              SLOT( onDecodedBufferSizeChanged( int ) ),
        Qt::QueuedConnection );

    connect( &The::audioController().m_thread, SIGNAL( outputBufferSizeChanged( int ) ),
        this,                              SLOT( onOutputBufferSizeChanged( int ) ),
        Qt::QueuedConnection );

    onRefresh();

    QDialog::show();
}


void 
DiagnosticsDialog::close()
{
    disconnect( &The::audioController().m_thread, SIGNAL( httpBufferSizeChanged( int ) ),
        this,                                     SLOT( onHttpBufferSizeChanged( int ) ) );

    disconnect( &The::audioController().m_thread, SIGNAL( decodedBufferSizeChanged( int ) ),
        this,                                     SLOT( onDecodedBufferSizeChanged( int ) ) );

    disconnect( &The::audioController().m_thread, SIGNAL( outputBufferSizeChanged( int ) ),
        this,                                     SLOT( onOutputBufferSizeChanged( int ) ) );

    if( m_logFile.is_open() )
    {
        m_logFile.close();
    }
    m_logTimer->stop();

    QDialog::accept();
}


void
DiagnosticsDialog::onHttpBufferSizeChanged(int bufferSize)
{
    ui.httpBufferProgress->setValue( bufferSize );
    ui.httpBufferLabel->setText( QString::number( bufferSize / 1000.0f, 'f', 1 ) + "k" );
}


void
DiagnosticsDialog::onDecodedBufferSizeChanged(int bufferSize)
{
    ui.decodedBufferProgress->setValue( bufferSize );
    ui.decodedBufferLabel->setText( QString::number( bufferSize / 1000.0f, 'f', 1 ) + "k" );
}


void
DiagnosticsDialog::onOutputBufferSizeChanged(int bufferSize)
{
    ui.outputBufferProgress->setValue( bufferSize );
    ui.outputBufferLabel->setText( QString::number( bufferSize / 1000.0f, 'f', 1 ) + "k" );
}


void
DiagnosticsDialog::onAppEvent( int event, const QVariant& /* data */ )
{
    switch ( event )
    {
        case Event::ScrobblePointReached:
        {
            populateCacheList( The::settings().currentUser().username() );
        }
        break;

        default:
        break;
    }
}


void
DiagnosticsDialog::onScrobblerEvent( int status )
{
    QString const username = The::settings().currentUser().username();
    
    QString submissionStatus = tr( "OK" );
    if ( The::scrobbler().lastError( username ) != Scrobbler::NoError && The::scrobbler().lastError( username ) != Scrobbler::ErrorNotInitialized)
    {
        submissionStatus = tr( "Error: " ) + Scrobbler::errorDescription( The::scrobbler().lastError( username ) );
    }
    else { 
        if (status == Scrobbler::Connecting)
            submissionStatus = tr( "Connecting to Last.fm..." );
        ui.lastConnectionStatusLabel->setText( QDateTime::currentDateTime().toString( "d/M/yyyy h:mm" ) );
    }

    ui.submissionServerStatusLabel->setText( submissionStatus );
    populateCacheList( The::settings().currentUser().username() );
}


void
DiagnosticsDialog::radioHandshakeReturn( Request* req )
{
    Handshake* handshake = static_cast<Handshake*>( req );
    if ( handshake->failed() )
    {
        ui.radioServerStatusLabel->setText( tr( "Error: " ) + handshake->errorMessage() );
    } else
    {
        ui.radioServerStatusLabel->setText( tr( "OK" ) );
    }
}


void 
DiagnosticsDialog::populateCacheList( const QString& username )
{
    ScrobbleCache scrobbleCache( username );

    QList<QTreeWidgetItem *> items;
    const QList<TrackInfo>& cachedTracks = scrobbleCache.tracks();
    for( QList<TrackInfo>::const_iterator i = cachedTracks.begin(); i != cachedTracks.end(); i++ )
    {
        if ( i->isScrobbled() )
            items.append( new QTreeWidgetItem( (QTreeWidget*)0,
                          QStringList() << i->artist() << i->track() << i->album() ) );
    }

    ui.cachedTracksList->clear();
    ui.cachedTracksList->insertTopLevelItems( 0, items );

    if (scrobbleCache.tracks().isEmpty())
        ui.cachedTracksLabel->setText( tr( "The cache is empty" ) );
    else
        ui.cachedTracksLabel->setText( tr( "%n cached tracks", "", items.count() ) );
}


void 
DiagnosticsDialog::onRefresh()
{
    LastFmUserSettings& user = The::settings().currentUser();

    QString username = user.username();
    QString password = user.password();
    QString version = The::settings().version();

    // Not strictly necessary. Will refresh itself via signal, too.
    onScrobblerEvent( Scrobbler::Handshaken );

//     ui.submissionServerStatusLabel->setText( tr( "Checking.." ) );
    ui.radioServerStatusLabel->setText( tr( "Checking.." ) );

    Handshake* radioHandshake = new Handshake;
    radioHandshake->setUsername( username );
    radioHandshake->setPassword( password );
    radioHandshake->setVersion( version );
    radioHandshake->setLanguage( The::settings().appLanguage() );
    radioHandshake->start();

    connect( radioHandshake, SIGNAL( result( Request* ) ),
        this,           SLOT  ( radioHandshakeReturn( Request* ) ),
        Qt::QueuedConnection );

    populateCacheList( username );
}


void
DiagnosticsDialog::onCopyToClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();
    QString clipboardText;

    //TODO should read "Last successful submission" - that's what it actually shows at least
    clipboardText.append( tr( "Last successful connection: " ) + ui.lastConnectionStatusLabel->text() + "\n\n" );
    clipboardText.append( tr( "Submission Server: " ) + ui.submissionServerStatusLabel->text() + "\n" );
    clipboardText.append( ui.cachedTracksLabel->text() + ":\n\n" );

    // Iterate through cached tracks list and add to clipboard contents
    for(int row = 0; row < ui.cachedTracksList->topLevelItemCount(); row++)
    {
        QTreeWidgetItem *rowData = ui.cachedTracksList->topLevelItem( row );
        for(int col = 0; col < rowData->columnCount(); col++)
        {
            clipboardText.append( rowData->data( col, Qt::DisplayRole ).toString() );
            clipboardText.append( "\t:\t" );
        }
        //remove trailing seperators
        clipboardText.chop(3);
        clipboardText.append( "\n" );
    }

    #ifndef HIDE_RADIO
    clipboardText.append("\n" + tr( "Radio Server: " ) + ui.radioServerStatusLabel->text() + "\n" );
    #endif

    clipboard->setText( clipboardText );
}


void
DiagnosticsDialog::onTrackFingerprintingStarted( TrackInfo track )
{
    ui.fpCurrentTrackLabel->setText( track.toString() );
    ui.fpQueueSizeLabel->setText( QString::number( The::app().m_fpCollector->queueSize() ) );
}


void
DiagnosticsDialog::onTrackFingerprinted( TrackInfo track  )
{
    ui.fpCurrentTrackLabel->setText( "" );
    ui.fpQueueSizeLabel->setText( QString::number( The::app().m_fpCollector->queueSize() ) );
    
    new QTreeWidgetItem( ui.fingerprintedTracksList, QStringList() << track.artist() << track.track() << track.album() );
}


void
DiagnosticsDialog::onCantFingerprintTrack( TrackInfo /* track */, QString /* reason */ )
{
    ui.fpCurrentTrackLabel->setText( "" );
    ui.fpQueueSizeLabel->setText( QString::number( The::app().m_fpCollector->queueSize() ) );
}


void 
DiagnosticsDialog::onLogPoll()
{
    //Clear all state flags on the file stream
    //this avoids stale state information
    m_logFile.clear();

    //This will reset the state information to !good
    //if at end of file.
    m_logFile.peek();

    //early out if at EOF or other error
    if( !m_logFile.good() )
        return;
    
    char* dataBuffer = new char[1000];

    //Read the log file in batches of 10 lines
    //(this is not ideal but avoids hanging the ui thread too much)
    for( int i = 0; i < 10 && m_logFile.good(); ++i )
    {
        QString data;
        m_logFile.getline( dataBuffer, 1000 );
        data = dataBuffer;
        data = data.trimmed();

        //ignore empty lines 
        if( data.isEmpty() )
            continue;

        ui.ipodInfoList->addItem( data );
        ui.ipodInfoList->scrollToBottom();
    }
    delete[] dataBuffer;
}

void 
DiagnosticsDialog::onScrobbleIpodClicked()
{
    //TODO: make DRY - this is replicated in WizardTwiddlyBootstrapPage.cpp
    #ifdef Q_OS_MAC
        #define TWIDDLY_EXECUTABLE_NAME "/../../Resources/iPodScrobbler"
    #else
        #define TWIDDLY_EXECUTABLE_NAME "/../iPodScrobbler.exe"
    #endif

    QStringList args = (QStringList() 
                    << "--device" << "diagnostic" 
                    << "--vid" << "0000" 
                    << "--pid" << "0000" 
                    << "--serial" << "UNKNOWN");

    bool isManual = ( ui.iPodScrobbleType->currentIndex() == 1 );
    if( isManual )
        args << "--manual";

    if( !m_logFile.is_open() )
    {
        #ifdef Q_WS_MAC
            QString twiddlyLogName = "Last.fm Twiddly.log";
        #else
            QString twiddlyLogName = "Twiddly.log";
        #endif
        
        qDebug() << "Watching log file: " << MooseUtils::logPath( twiddlyLogName );       
 
        m_logFile.open(MooseUtils::logPath( twiddlyLogName ).toStdString().c_str());

        m_logFile.seekg( 0, std::ios_base::end );
        m_logTimer->start( 10 );
    }

    QProcess::startDetached( The::settings().path() + TWIDDLY_EXECUTABLE_NAME, args );
}


void
DiagnosticsDialog::reconnect()
{
    The::app().setUser( The::currentUsername() );
}
