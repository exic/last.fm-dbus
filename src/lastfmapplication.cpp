/***************************************************************************
 *   Copyright (C) 2005 - 2008 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
 *      Jono Cole, Last.fm Ltd <jono@last.fm>                              *
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

#include "lastfmapplication.h"

#include "configwizard.h"
#include "container.h"
#include "LastMessageBox.h"
#include "libFingerprint/FingerprintCollector.h"
#include "libFingerprint/FingerprintQueryer.h"
#include "logger.h"
#include "loginwidget.h"
#include "MediaDeviceScrobbler.h"
#include "playercommands.h"
#include "playerlistener.h"
#include "User.h"
#include "Radio.h"
#include "WebService.h"
#include "WebService/Request.h"
#include "Scrobbler-1.2.h"
#include "LastFmSettings.h"
#include "version.h"
#include "CachedHttpJanitor.h"
#include "interfaces/ExtensionInterface.h"

#ifndef Q_WS_X11
    #include "Bootstrapper/PluginBootstrapper.h"
#endif

#include "MooseCommon.h"
#include "UnicornCommon.h"
#include "WebService/FrikkinNormanRequest.h"
#include "mbid_mp3.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#ifdef WIN32
    #include <windows.h>
#endif

#ifdef Q_WS_MAC
    #include "itunesscript.h"
    #include "ITunesPluginInstaller.h"
    #include <Carbon/Carbon.h>
#endif

static void qMsgHandler( QtMsgType, const char* );


LastFmApplication::LastFmApplication( int& argc, char** argv )
        : QApplication( argc, argv ),
          m_endSessionEmitted( false ),
          m_handshaked( false ),
          m_user( 0 ),
          m_activeNorman( 0 ),
          m_state( State::Stopped ),
          m_proxyTestDone( false ),
          m_extensionsLoaded( false )
{
    #ifdef Q_WS_MAC
        m_pidFile.setFileName( MooseUtils::savePath( "lastfm.pid" ) );
        if ( !m_pidFile.open( QIODevice::WriteOnly | QIODevice::Text ) )
        {
            qDebug() << "could not write to " << MooseUtils::savePath( "lastfm.pid" );
        }
        m_pidFile.write( "last.fm" );
        qDebug() << "******* PID FILE == " << MooseUtils::savePath( "lastfm.pid" );
    #endif

    // run init() as first event in event-queue
    // TODO why do we do this, and do we need to?
    //   Good question, this was added as a hacky workaround to try and fix
    //   some QHttp wonkiness we had on some machines back in the day. Best
    //   not removing it. May fix obscure bugs like, ws.as.com is not accessible
    //   for 3 or 4 minutes
    // NOTE it's prolly a good thing for feedback reasons anyway, the UI appears
    //   quicker and thus feels like it starts faster
    QTimer::singleShot( 0, this, SLOT( init() ) );

    // We're doing this here because the version.h file is updated just before
    // each build of container so if we include it directly in lastfmtools it
    // will always lag one version number behind.
    The::settings().setVersion( LASTFM_CLIENT_VERSION );
    The::settings().setPath( QCoreApplication::applicationFilePath() );

    // These are static properties on the CachedHttp object which we need to
    // set up to provide our "special" user agent string and cache path.
    CachedHttp::setCustomUserAgent( QString( "Last.fm Client " ) + LASTFM_CLIENT_VERSION );
    CachedHttp::setCustomCachePath( MooseUtils::cachePath() );

    // do this asap to prevent multiple instances instantiating
    m_control = new QTcpServer( this );
    connect( m_control, SIGNAL( newConnection() ), SLOT( onControlConnect() ) );

    // Try to use the default port, otherwise revert to using an available port
    // and write the port we're using to the Settings. sendToInstance() will use
    // the port stored in the Settings when trying to connect to us.
    if ( !m_control->listen( QHostAddress::LocalHost, The::settings().controlPort() ) )
        m_control->listen( QHostAddress::LocalHost );

    The::settings().setControlPort( m_control->serverPort() );

    connect( this, SIGNAL( event( int, QVariant ) ), SLOT( onAppEvent( int, QVariant ) ) );

    initTranslator();
    registerMetaTypes();

    setQuitOnLastWindowClosed( false );
  #ifdef Q_WS_X11
    setWindowIcon( QIcon( MooseUtils::dataPath( "icons/as.ico" ) ) );
  #endif

  #ifdef WIN32
    MooseUtils::disableHelperApp();    
  #endif

    // This is needed so that relative paths will work on Windows regardless
    // of where the app is launched from.
    QDir::setCurrent( applicationDirPath() );

    // this must be set before dialogs spawn in init() or whatever
    m_user = new User( this );

    // TODO this class merged here!
    The::webService(); //init webservice stuff
    connect( The::webService(), SIGNAL( result( Request* ) ), SLOT( onRequestReturned( Request* ) ) );

    // Scrobbler must be initialised before the listener, otherwise crashes might ensue
    m_scrobbler = new ScrobblerManager( this );
    connect( m_scrobbler, SIGNAL(status( int, QVariant )), SLOT(onScrobblerStatusUpdate( int, QVariant )) );

    m_listener = new CPlayerListener( this );
    connect( m_listener, SIGNAL( trackChanged( TrackInfo, bool ) ),
             this,         SLOT( onListenerNewSong( TrackInfo, bool ) ), Qt::QueuedConnection );
    connect( m_listener, SIGNAL( trackScrobbled( TrackInfo ) ),
             this,         SLOT( onScrobblePointReached( TrackInfo ) ), Qt::QueuedConnection );
    connect( m_listener, SIGNAL( exceptionThrown( QString ) ),
             this,         SLOT( onListenerException( QString ) ) );
    connect( m_listener, SIGNAL( bootStrapping( QString, QString ) ),
             this,         SLOT( onBootstrapReady( QString, QString ) ) );

    // Start listener worker thread
    m_listener->start();

  #ifdef Q_WS_MAC
    new ITunesScript( this, m_listener );
  #endif

    m_fpCollector = new FingerprintCollector( 1 /*number of threads*/, this );
    m_fpQueryer = new FingerprintQueryer( this );
    connect( m_fpQueryer, SIGNAL( trackFingerprinted( TrackInfo, bool ) ),
                          SLOT( onFingerprintQueryDone( TrackInfo, bool ) ) );

    m_radio = new Radio( this );
    connect( m_radio, SIGNAL( stateChanged( RadioState ) ), SLOT( onRadioStateChanged( RadioState ) ) );

    m_container = new Container;
    connect( m_container, SIGNAL( becameVisible() ), SLOT( fetchMetaData() ) );

    // Look for expired cached files and remove them
    new CachedHttpJanitor( MooseUtils::cachePath(), this );

    m_playbackEndedTimer = new QTimer( this );
    m_playbackEndedTimer->setSingleShot( true );
    m_playbackEndedTimer->setInterval( 200 );
    connect( m_playbackEndedTimer, SIGNAL( timeout() ), SLOT( onPlaybackEndedTimerTimeout() ) );

    // This is needed for the app to shut down properly when asked by Windows
    // to shut down.
    connect( this, SIGNAL( endSession() ), SLOT( quit() ) );
}


void
LastFmApplication::init()
{
  #ifdef Q_WS_MAC
    if ( QSysInfo::MacintoshVersion < QSysInfo::MV_10_4 )
    {
        LastMessageBox::critical( tr( "Unsupported OS Version" ),
                                  tr( "We are sorry, but Last.fm requires OS X version 10.4 (Tiger) or above." ) );

        quit();
        return;
    }
  #endif


    foreach ( QString const arg, qApp->arguments().mid( 1 ) ) //skip arg[0] the appname+path
        parseCommand( arg );

    // Need to save the state from before we run the wizard as the wizard will change it
    bool firstRunBeforeWizard = The::settings().isFirstRun();

    #ifdef Q_OS_MAC
      // the installation of the plugin is done in the ctor
      ITunesPluginInstaller pluginInstaller;
      pluginInstaller.install();
    #endif


    if ( The::settings().isFirstRun() )
    {
        // HACK: this is only here to fix the bug where the initial wizard runs and we're behind a proxy.
        // We need to have done these tests before running the VerifyUserRequest, otherwise the Http object
        // will not pick an autodetected proxy if there is one. Refactor for 1.5.
        if( The::settings().isFirstRun() && !The::settings().isUseProxy() )
        {
            ProxyTestRequest* proxyOnTest = new ProxyTestRequest( true );
            ProxyTestRequest* proxyOffTest = new ProxyTestRequest( false );
            proxyOnTest->start();
            proxyOffTest->start();
        }

        LOGL( 3, "First run, launching config wizard" );
        QFile( MooseUtils::savePath( "mediadevice.db" ) ).remove();

        ConfigWizard wiz( NULL, ConfigWizard::Login );

        if ( wiz.exec() == QDialog::Rejected )
        {
            // If user cancels config wizard, we need to exit
            quit();
            return;
        }
    }
  #ifndef LINUX
    else
    {
      #ifdef Q_OS_MAC
        // We just installed the plugin for the first time
        bool const needsBootstrap = pluginInstaller.needsTwiddlyBootstrap();
      #else
        // The update wizard invites the user to upgrade the iTunes plugin, and
        // it did this last time the client was invoked. iTunes has been 
        // stoppped, so the new plugin will be running when we Twiddly starts
        // iTunes during the bootstrap
        bool needsBootstrap = The::settings().weWereJustUpgraded();
      #endif
        
        //NOTE some of this code is duplicated in Container::updateCheckDone()
        if ( needsBootstrap )
        {
            // the NULL is strange, and I have no idea why we do it here or above
            // hopefully whoever did it in the first place will comment it :P
            ConfigWizard( NULL, ConfigWizard::MediaDevice ).exec();
        }
    }
  #endif

    // Do we have a current user?
    QString currentUser = The::settings().currentUsername();
    bool doLogin = false;
    if ( currentUser.isEmpty() )
    {
        LOG( 3, "No current user\n" );
        doLogin = true;
    }
    else
    {
        doLogin = !The::settings().user( currentUser ).rememberPass();
    }

    if ( doLogin && !firstRunBeforeWizard )
    {
        LOG( 3, "Ask for login\n" );

        LoginWidget login( NULL, LoginWidget::LOGIN, currentUser );
        if ( login.createDialog().exec() != QDialog::Accepted )
        {
            quit();
            return;
        }
    }
    else
    {
        setUser( currentUser );
    }

    if ( !arguments().contains( "-tray" ) && !arguments().contains( "--tray" ) )
        m_container->show();
}


LastFmApplication::~LastFmApplication()
{
    delete m_control;
    delete m_container;

    //FIXME may cause two stops since radio does too
    emit event( Event::PlaybackEnded, QVariant::fromValue( m_currentTrack ) );

    m_radio->stop();

    LOGL( Logger::Debug, "Radio state at shutdown: " << radioState2String( m_radio->state() ) );

    // Bottom line here is that we must wait until the radio thread has stopped
    // and it has dealt with all outstanding events.
    int count = 0;
    do {
        processEvents();
      #ifdef WIN32
        Sleep( 10 ); // milliseconds 10E-3
      #else
        usleep( 10 * 1000 ); // microseconds 10E-6
      #endif

        // Sanity check. If the radio has hung and will not respond within 5 seconds,
        // it most likely never will, so it should be safe to shut down the listener.
        // Otherwise our process might get stuck zombie-like for all eternity.
        if ( count++ > 500 )
            break;
    }
    while ( m_radio->state() != State_Stopped &&
            m_radio->state() != State_Uninitialised &&
            m_radio->state() != State_Handshaking &&
            m_radio->state() != State_Handshaken );

    LOGL( 3, "Shutting down listener" );
    m_listener->Stop();

    sendPostedEvents( m_scrobbler, 0 /*all event types*/ );
    //TODO send events to individual scrobblers in the manager too?

    delete m_fpQueryer;
    delete m_fpCollector;

    #ifdef Q_WS_MAC
        if ( !m_pidFile.remove() )
        {
            qDebug() << "filename: " << m_pidFile.fileName();
            qDebug() << "could not remove lastfm.pid";
            qDebug() << "error: " << m_pidFile.error();
        }
        else
            qDebug() << "PID file removed.";
    #endif

    delete &The::settings();
}


void
LastFmApplication::initTranslator()
{
    QString langCode;

    #ifdef HIDE_RADIO
        langCode = "jp";
        The::settings().setAppLanguage( langCode );
    #else
        langCode = The::settings().appLanguage();
    #endif

    if ( !The::settings().customAppLanguage().isEmpty() )
        LOGL( 3, "Language set by user to: " << langCode );

    setLanguage( langCode );
    installTranslator( &m_translatorApp );
    installTranslator( &m_translatorQt );

    Request::setLanguage( The::settings().appLanguage() );
}


void
LastFmApplication::setLanguage( QString langCode )
{
    LOGL( 3, "Setting language to: " << langCode );

    m_lang = langCode;

    // Discards previously loaded translations
    m_translatorApp.load( MooseUtils::dataPath( "i18n/lastfm_%1" ).arg( langCode ) );
    m_translatorQt.load( MooseUtils::dataPath( "i18n/qt_%1" ).arg( langCode ) );
}


void
LastFmApplication::registerMetaTypes()
{
    // This is needed so we can pass MetaData objects as signal/slot params
    // with queued connections.
    qRegisterMetaType<TrackInfo>( "TrackInfo" );
    qRegisterMetaType<MetaData>( "MetaData" );
    qRegisterMetaType<CPlayerCommand>( "CPlayerCommand" );
    qRegisterMetaType<RadioError>( "RadioError" );
    qRegisterMetaType<RadioState>( "RadioState" );
}


void
LastFmApplication::setUser( const QString& username )
{
    Q_ASSERT( !username.isEmpty() );
    
    The::settings().setCurrentUsername( username );
    The::webService()->setUsername( username );
    The::webService()->setPassword( The::currentUser().password() );

    if ( !m_extensionsLoaded )
    {
        // Can't load extensions until we have a current user
        loadExtensions();
        m_extensionsLoaded = true;
    }

    if ( m_user )
    {
        // we no longer care about any signals this user may emit
        disconnect( m_user, 0, this, 0 );
        m_user->shutdownThenDelete();
    }

    m_handshaked = false;
    m_user = new User( username, this );

#ifndef Q_WS_X11
    m_proxyTestDone = false;
    if( !The::settings().isUseProxy() )
    {
        //Send proxy test requests
        ProxyTestRequest* proxyOnTest = new ProxyTestRequest( true );
        ProxyTestRequest* proxyOffTest = new ProxyTestRequest( false );

        disconnect( The::webService(), SIGNAL( proxyTestResult( bool, WebRequestResultCode ) ),
                    this,              SLOT( onProxyTestComplete( bool, WebRequestResultCode ) ) );
        connect( The::webService(), SIGNAL( proxyTestResult( bool, WebRequestResultCode ) ),
                this,              SLOT( onProxyTestComplete( bool, WebRequestResultCode ) ) );

        proxyOnTest->start();
        proxyOffTest->start();

    }
    else
    {
        emit event( Event::UserChanged, username );
        onProxyTestComplete( false, Request_Success );
    }
#else
    m_proxyTestDone = false;
    onProxyTestComplete( false, Request_Success  );
#endif
}


void
LastFmApplication::onProxyTestComplete( bool proxySet, WebRequestResultCode result, bool authProxyTimerComplete )
{
    // The only reason for this slot is to delay the handshake until we know whether
    // to use the proxy or not.
    qDebug() << "*********** ProxyTest Complete Result code = " << result;

    
    if( result == Request_ProxyAuthenticationRequired && !authProxyTimerComplete )
    {
        // Proxy authentication required.
        // Wait 3 seconds to see if a direct connection is available before 
        // prompting the user for login details.
        QTimer::singleShot( 3000, this, SLOT( onAuthenticatedProxyTimerTimeout() ) );
        return;
    }

    if ( m_proxyTestDone ) return;
    m_proxyTestDone = true;
    
#ifndef Q_WS_X11
    LOGL( 3, ( proxySet ? "" : "not "  ) << "using autodetected proxy settings" );
#endif

    // HACK: since we're in a different function, we need to set the username again
    // as it was a parameter to setUser.
    QString username = m_user->settings().username();

    emit event( Event::UserChanged, username );

    QString password = m_user->settings().password();
    QString version = The::settings().version();

    // as you can see we are initialising the fingerprinter, I like this comment
    m_fpCollector->setUsername( username );
    m_fpCollector->setPasswordMd5( password );
    m_fpCollector->setPasswordMd5Lower( password ); // FIXME: surely they can't be the same!
    m_fpQueryer->setUsername( username );
    m_fpQueryer->setPasswordMd5( password );
    m_fpQueryer->setPasswordMd5Lower( password ); // FIXME: surely they can't be the same!
    m_fpQueryer->setVersion( The::settings().version() );

    // init radio YTIO
    m_radio->init( username, password, version );

    // Shut down the current listener, this will cause a scrobble if required
    // and reset the new user's track progress bar. Only for media players, not
    // for radio as that always gets stopped on user switching.
    if ( m_listener->GetActivePlayer() && m_listener->GetNowPlaying().source() != TrackInfo::Radio )
    {
        CPlayerCommand command( PCMD_STOP, m_listener->GetActivePlayer()->GetID(), m_listener->GetNowPlaying() );
        m_listener->Handle( command );

        command.mCmd = PCMD_START;
        m_listener->Handle( command );
    }

    // initialise the scrobbler YTIO
    Scrobbler::Init init;
    init.username = username;
    init.client_version = version;
    init.password = password;
    m_scrobbler->handshake( init );
}


////////////////////////////////////////////////////////////////////////////
// After a timeout period this will determine if a direct connection has  // 
// been established by another proxyTestRequest. If not then              //
// authentication required dialog is shown before resuming user handshake.//
////////////////////////////////////////////////////////////////////////////
void
LastFmApplication::onAuthenticatedProxyTimerTimeout()
{
    if( !m_proxyTestDone )
    {
        LastMessageBox::information( tr( "Proxy Authentication Required" ),
            tr( "The proxy autodetection has detected a proxy server but does not have enough information to authenticate.\n\n"
            "Please set the proxy settings to manual and enter the username and password required." ) );

        The::container().showSettingsDialog( 3 );
        onProxyTestComplete( true, Request_ProxyAuthenticationRequired, true );
    }
}


void
LastFmApplication::loadExtensions()
{
    foreach( QString path, Moose::extensionPaths() )
    {
        LOGL( 3, "Loading extension: " << path );
        QObject* plugin = QPluginLoader( path ).instance();
        if ( !plugin )
	    {
            LOGL( 1, "Failed to load " << path );
            continue;
        }

        LOGL( 3, "Extension loaded" );
        ExtensionInterface* iExtension = qobject_cast<ExtensionInterface *>( plugin );
        if ( iExtension )
        {
            QSettings* us = new CurrentUserSettings( this );
            iExtension->setSettings( us );
        }
    }
}


#ifdef WIN32
bool
LastFmApplication::winEventFilter( MSG * msg, long * result )
{
    /*
    typedef struct MSG {
        HWND        hwnd;
        UINT        message;
        WPARAM      wParam;
        LPARAM      lParam;
        DWORD       time;
        POINT       pt;
    }*/

    // This message is sent by Windows when we're being shut down as part
    // of a Windows shutdown. Don't want to just minimise to tray so we
    // emit a special endSession signal to Container. It can get sent
    // several times though so we must guard against emitting the signal
    // more than once.
    if ( msg->message == WM_QUERYENDSESSION )
    {
        if ( !m_endSessionEmitted )
        {
            m_endSessionEmitted = true;
            emit endSession();
        }
        *result = 1; // tell Windows it's OK to shut down
        return true; // consume message
    }

    return false; // let Qt handle it
}
#endif // WIN32


void
LastFmApplication::onListenerNewSong( const TrackInfo& track, bool started )
{
    QVariant const v = QVariant::fromValue( track );
    TrackInfo const oldtrack = m_currentTrack;
    m_currentTrack = track;

    // Need to abort these, otherwise a pending metadata request can come in
    // and populate the new track
    if ( !m_activeArtistReq.isNull() ) m_activeArtistReq->abort();
    if ( !m_activeTrackReq.isNull() ) m_activeTrackReq->abort();

    if ( !started )
    {
        // Here we know that we're dealing with a stopped track
        if ( oldtrack.source() == TrackInfo::Radio )
        {
            // With the radio, we emit PlaybackEnded on its Stopped signal,
            // so we don't need to worry about that here.
            return;
        }

        // Don't emit multiple stops as it makes the UI unresponsive
        if ( m_state != State::Stopped && m_state != State::TuningIn )
        {
            // We use a timer as some players say they stop before starting a new
            // track, which results in us flickering between the two widgets, and 
            // that looks shite
            m_playbackEndedTimer->start();
            m_state = State::Stopped;
        }
    }
    else if ( m_state == State::Stopped )
    {
        m_state = State::Playing;
        emit event( Event::PlaybackStarted, v );
    }
    // Currently no way of getting into a paused state but this logic is sound.
    else if ( m_state == State::Paused && m_currentTrack.sameAs( oldtrack ) )
    {
        m_state = State::Playing;
        emit event( Event::PlaybackUnpaused, v );
    }
    else
    {
        m_state = State::Playing; //might have been tuning in or paused then play a different track
        emit event( Event::TrackChanged, v );
    }
}


void
LastFmApplication::onPlaybackEndedTimerTimeout()
{
    switch ( m_state )
    {
        case State::Playing:
        case State::TuningIn:
        break;

        case State::Stopped:
        case State::Paused:
            emit event( Event::PlaybackEnded, QVariant() );
        break;

        default:
            Q_ASSERT( !"Unhandled state here :(" );
        break;
    }
}


void
LastFmApplication::onScrobblePointReached( const TrackInfo& track )
{
    emit event( Event::ScrobblePointReached, QVariant::fromValue( track ) );
}


void
LastFmApplication::onScrobblerStatusUpdate( int code, const QVariant& data )
{
    if ( code == Scrobbler::Handshaken )
    {
        QString const username = data.toString();
        IPodScrobbler( username, m_container ).exec();
    }
}


void
LastFmApplication::onRadioStateChanged( RadioState newState )
{
    switch ( newState )
    {
        case State_Handshaken:
        {
            LOGL( 3, "Radio streamer handshake successful." );

            m_handshaked = true;
            emit event( Event::UserHandshaken );

            if ( m_preloadStation.contains( "lastfm://" ) )
            {
                m_radio->playStation( m_preloadStation );
                m_preloadStation.clear();
            }
            else if ( m_user->settings().resumePlayback() && !m_user->settings().resumeStation().isEmpty() )
            {
                m_radio->playStation( m_user->settings().resumeStation() );
            }
        }
        break;

        case State_Buffering:
            emit event( Event::PlaybackStalled );
        break;

        case State_Streaming:
            //Radio tells the player listener about the track
            //FIXME encapsulate the player-listener and make its functionality concise!
        break;

        case State_Handshaking:
        case State_Stopped:
        {
            if ( m_state != State::Stopped )
            {
                m_state = State::Stopped;
                emit event( Event::PlaybackEnded );

                m_currentTrack = MetaData();
            }
        }
        break;

        case State_Stopping:
        case State_Uninitialised:
        case State_Skipping:
            // no App::state change
        break;

        case State_ChangingStation:
        case State_FetchingPlaylist:
        {
            if ( m_state != State::TuningIn )
            {
                m_state = State::TuningIn;
                emit event( Event::TuningIn );
            }
        }
        break;

        case State_FetchingStream:
        case State_StreamFetched:
            //TODO should show some feed back
            break;

        default:
            Q_ASSERT( !"Undefined state case reached in onRadioStateChanged!" );
    }
}


void
LastFmApplication::onAppEvent( int event, const QVariant& /* data */ )
{
    //Do not respond to any events if there is no user logged in
    if( !m_user )
        return;
        
    switch ( event )
    {
        case Event::PlaybackStarted:
        case Event::TrackChanged:
        {
            m_playbackEndedTimer->stop();

            if ( false /*m_currentTrack.artist().isEmpty() || m_currentTrack.track().isEmpty()*/ )
            {
                // We don't have enough ID3 data, need to rely on fingerprinting to get it
            }
            else
            {
                // We have sufficient ID3 data to kick off NP, metadata requests etc
                if ( m_user->settings().isLogToProfile() )
                {
                    m_scrobbler->nowPlaying( m_currentTrack );
                }

                char mbid[MBID_BUFFER_SIZE];
                if ( m_currentTrack.source() == TrackInfo::Player )
                {
                    // FIXME: Path needs to use unicode.
                    if ( getMP3_MBID( m_currentTrack.path().toLocal8Bit().constData(), mbid ) != -1 )
                        m_currentTrack.setMbId( mbid );
                    else
                        LOGL( 2, "Failed to extract MBID for: " << m_currentTrack.path() );
                }

                if ( m_container->isVisible() )
                    fetchMetaData();

                if ( QFile::exists( m_currentTrack.path() ) &&
                     The::settings().currentUser().fingerprintingEnabled() )
                {
                    m_activeNorman = 0;
                    m_fpQueryer->fingerprint( m_currentTrack );
                }
            }
        }
        break;

        case Event::ScrobblePointReached:
        {
            if ( m_user->settings().isLogToProfile() )
            {
                // we scrobble for the user who started the track always
                //
                // FIXME: this will only happen for the currently visible track.
                // If a background track down in the PlayerListener reaches its
                // scrobble point, it won't get to here as its scrobblePointReached
                // signal will be swallowed by the listener.
                //
                // This just caches the track for safety. When the track actually
                // gets submitted to the scrobbler from the PlayerConnection, the
                // duplicate cache entry will get pruned.
                //
                // However, the m_currentTrack we have in here might contain different
                // info to the TrackInfo object held in the relevant PlayerConnection
                // which is submitted to the scrobbler when the track finishes. This
                // sucks. We need ONE OBJECT and ONE OBJECT ONLY that manages the
                // active TrackInfos.
                m_currentTrack.setRatingFlag( TrackInfo::Scrobbled );
                ScrobbleCache cache( m_currentTrack.username() );
                cache.append( m_currentTrack );
            }
        }
        break;
    }
}


void
LastFmApplication::fetchMetaData()
{
    if ( m_currentTrack.isEmpty() || !m_user->settings().isMetaDataEnabled() )
        return;

    m_activeArtistReq = new ArtistMetaDataRequest();
    m_activeTrackReq = new TrackMetaDataRequest();

    m_activeArtistReq->setArtist( m_currentTrack.artist() );
    m_activeArtistReq->setParent( this );
    m_activeArtistReq->setLanguage( The::settings().appLanguage() );
    m_activeArtistReq->start();

    m_activeTrackReq->setTrack( m_currentTrack );
    m_activeTrackReq->setParent( this );
    m_activeTrackReq->setLanguage( The::settings().appLanguage() );
    m_activeTrackReq->start();
}


void
LastFmApplication::onRequestReturned( Request* request )
{
    Q_ASSERT( request );

    //TODO if already cached, pass everything immediately
    //TODO error handling

    switch ( request->type() )
    {
        case TypeHandshake:
        {
            if ( request->failed() )
                break;

            Handshake* handshake = static_cast<Handshake*>(request);
            The::user().m_isSubscriber = handshake->isSubscriber();
        }
        break;

        case TypeArtistMetaData:
        {
            if ( request != m_activeArtistReq || request->failed() )
                break;

            MetaData metadata = static_cast<ArtistMetaDataRequest*>(request)->metaData();

            // Copy new stuff into our own m_currentTrack
            m_currentTrack.setArtist( metadata.artist() );
            m_currentTrack.setArtistPicUrl( metadata.artistPicUrl() );
            m_currentTrack.setArtistPageUrl( metadata.artistPageUrl() );
            m_currentTrack.setNumListeners( metadata.numListeners() );
            m_currentTrack.setNumPlays( metadata.numPlays() );
            m_currentTrack.setWiki( metadata.wiki() );
            m_currentTrack.setWikiPageUrl( metadata.wikiPageUrl() );
            m_currentTrack.setArtistTags( metadata.artistTags() );
            m_currentTrack.setSimilarArtists( metadata.similarArtists() );
            m_currentTrack.setTopFans( metadata.topFans() );

            // if track has changed before we got this metadata, don't emit the event ;)
            emit event( Event::ArtistMetaDataAvailable, QVariant::fromValue( m_currentTrack ) );
        }
        break;

        case TypeTrackMetaData:
        {
            if ( request != m_activeTrackReq || request->failed() )
                break;

            MetaData metadata = static_cast<TrackMetaDataRequest*>(request)->metaData();

            // Copy new stuff into our own m_currentTrack
            m_currentTrack.setArtist( metadata.artist() );
            m_currentTrack.setTrack( metadata.track() );
            m_currentTrack.setTrackPageUrl( metadata.trackPageUrl() );
            if ( !metadata.album().isEmpty() )
            {
                m_currentTrack.setAlbum( metadata.album() );
                m_currentTrack.setAlbumPageUrl( metadata.albumPageUrl() );
            }
            m_currentTrack.setAlbumPicUrl( metadata.albumPicUrl() );
            m_currentTrack.setLabel( metadata.label() );
            m_currentTrack.setLabelUrl( metadata.labelUrl() );
            m_currentTrack.setNumTracks( metadata.numTracks() );
            m_currentTrack.setReleaseDate( metadata.releaseDate() );
            m_currentTrack.setBuyTrackString( metadata.buyTrackString() );
            m_currentTrack.setBuyTrackUrl( metadata.buyTrackUrl() );
            m_currentTrack.setBuyAlbumString( metadata.buyAlbumString() );
            m_currentTrack.setBuyAlbumUrl( metadata.buyAlbumUrl() );

            // if track has changed before we got this metadata, don't emit the event ;)
            emit event( Event::TrackMetaDataAvailable, QVariant::fromValue( m_currentTrack ) );
        }
        break;

        default:
            break;
    }
}


void
LastFmApplication::onListenerException( const QString& msg )
{
  #ifndef LASTFM_MULTI_PROCESS_HACK
    // Can't do much else than fire up a dialog box of doom at this stage
    LastMessageBox::critical( tr( "Plugin Listener Error" ), msg );
  #else
    // don't make debugging a pita, clearly this isn't meant for release builds though
    m_container->statusBar()->showMessage( msg );
  #endif
}


void
LastFmApplication::onControlConnect()
{
    // Incoming connection from either a webbrowser or
    // some other program like the firefox extension.

    QTcpSocket* socket = m_control->nextPendingConnection();
    connect( socket, SIGNAL( readyRead() ), SLOT( onControlRequest() ) );
}


void
LastFmApplication::onControlRequest()
{
    QTcpSocket* socket = (QTcpSocket*)sender();
    QString request = socket->readAll();

    LOGL( 3, "clientRequest (old instance): " << request );

    foreach( QString cmd, request.split( " " ) )
        parseCommand( cmd );

    socket->flush();
    socket->close();
    socket->deleteLater();
}


void
LastFmApplication::parseCommand( const QString& request )
{
    Q_DEBUG_BLOCK << request;

    if ( request.contains( "lastfm://" ) )
    {
        #ifndef HIDE_RADIO
            LOGL( 3, "Calling radio with station" );
            if ( !m_handshaked )
                m_preloadStation = StationUrl( request );
            else
                m_radio->playStation( StationUrl( request ) );
        #endif // HIDE_RADIO
    }

    if ( request.contains( "container://show" ) )
    {
        m_container->restoreWindow();
    }

  #ifndef Q_WS_X11
    if ( request.contains( "container://SubmitScrobbleCache/Device" ) )
    {
        QStringList const params = request.split( "container://SubmitScrobbleCache/Device/" ).value( 1 ).split( "/" );
        int const vid = params.value( 2 ).toInt(); //vendor id
        int const pid = params.value( 3 ).toInt(); //product id
        QString const uid = params.value( 0 ) + '/' + params.value( 1 );
        QString username = The::settings().usernameForDeviceId( uid );

        if ( username.isEmpty() )
        {
            username = The::currentUsername();
            The::settings().addMediaDevice( uid, username );
        }
        
        if ( The::currentUsername() != username )
        {
            LastMessageBox::warning( tr("iPod Scrobbling"),
                                     tr("<p>This iPod is associated with a different Last.fm account."
                                        "<p>Please log in as <b>%1</b> to submit the scrobbles."
                                        "<p>You can change the user association in the Options dialog.").arg( username ),
                                     QMessageBox::Ok,
                                     QMessageBox::Ok );
        }
        else
        {
            IPodScrobbler( username, m_container ).exec();
        }
    }

    if ( request.startsWith( "container://Notification" ) && !ConfigWizard::isActive() )
    {
        QString title, message;
        QStringList keys = request.mid( 12 ).split( '/' );

        if (keys.value( 1 ) == "Twiddly")
        {
            title = tr("iPod Scrobbling");
            QString key = keys.value( 2 );
                        
            // ask Toby if you wonder why we don't show any of these messages
            // by default
            if (The::currentUser().giveMoreIPodScrobblingFeedback())
            {
                if ( key == "Started" )
                    message = tr( "Your iPod scrobbles are being determined. Please don't exit iTunes." );
                
                if ( key == "Error" )
                {
                    title = tr( "iPod Scrobbling Error" );
                    message = keys.value( 3 ).replace( '_', ' ' );
                }
            
                if ( key == "Finished" )
                {
                    int const n = keys.value( 3 ).toInt();
                    if (n)
                        message = tr( "Last.fm found %n scrobbles on your iPod.", "", n );
                    else
                        message = tr( "No scrobbles were found on your iPod." );
                }
            }
            
            if (key == "Bootstrap")
            {
                key = keys.value( 3 );
                if (key == "Started")
                    message = tr("Preparing for iPod scrobbling, please don't exit iTunes or sync your iPod.");
                if (key == "Finished")
                    message = tr("Last.fm is now ready for iPod scrobbling.");
            }
        }
        else
        {
            QString key = keys.value( 2 );

            if ( key == "IPodDetected" )
            {
                title = tr( "iPod detected" );
                message = tr( "Your iPod will be scrobbled to your Last.fm profile from now on." );
            }
        }
        
        if (!message.isEmpty())
        {
            m_container->showNotification( title, message );
        }
    }
  #endif
}


void
LastFmApplication::onBootstrapReady( QString userName, QString pluginId )
{
    #ifndef Q_WS_X11
    // Ignore the bootstrap if we're not currently logged in as the user who initiated it.
    // (The bootstrap file will be detected when the user who initiated it logs back in.)
    if ( userName != The::currentUsername() )
    {
        LastMessageBox::warning( "Bootstrap detected",
            QString( "Bootstrap information was detected for user %1.\n"
            "Please log in as %1 to submit this." ).arg( userName ),
            QMessageBox::Ok,
            QMessageBox::Ok );
        return;
    }

    PluginBootstrapper* bootstrapper = new PluginBootstrapper( pluginId, this );
    bootstrapper->submitBootstrap();
    #endif
}


void
LastFmApplication::onFingerprintQueryDone( TrackInfo track, bool fullFpRequested )
{
    // We're using the path here as the track metadata could have been changed by
    // a metadata request in-between requesting the fp and getting it.

    if ( m_currentTrack.path() != track.path() )
        return;

    m_currentTrack.setFpId( track.fpId() );

    if ( fullFpRequested && The::settings().currentUser().fingerprintingEnabled() )
    {
        m_fpCollector->fingerprint( QList<TrackInfo>() << m_currentTrack );
    }

    if ( qApp->arguments().contains( "--norman" ) )
    {
        if ( track.fpId() != "0" && !track.fpId().isEmpty() && !fullFpRequested )
        {
            m_activeNorman = new FrikkinNormanRequest();
            m_activeNorman->setFpId( track.fpId() );
            connect( m_activeNorman, SIGNAL( result( Request* ) ), SLOT( onNormanRequestDone( Request* ) ) );
            m_activeNorman->start();
        }
        else
        {
            m_activeNorman = 0;
            //m_container->statusBar()->showMessage( "Norman sez: I not know dis one" );
        }
    }
}


void
LastFmApplication::onNormanRequestDone( Request* r )
{
    FrikkinNormanRequest* req = static_cast<FrikkinNormanRequest*>( r );
    if ( req != m_activeNorman || req->failed() )
        return;
    //m_container->statusBar()->showMessage( req->metadata() );
    m_activeNorman = 0;
}


namespace The
{
    User& user() { return The::app().user(); }
    Radio& radio() { return The::app().radio(); }
    ScrobblerManager& scrobbler() { return The::app().scrobbler(); }
}
