/***************************************************************************
 *   Copyright (C) 2005 - 2008 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
 *      Max Howell, Last.fm Ltd <max@last.fm>                              *
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

#include <QDesktopServices>

#include "container.h"

#include "aboutdialog.h"
#include "autoupdater.h"
#include "configwizard.h"
#include "confirmdialog.h"
#include "MooseCommon.h"
#include "deleteuserdialog.h"
#include "failedlogindialog.h"
#include "iconshack.h"
#include "lastfmapplication.h"
#include "LastMessageBox.h"
#include "logger.h"
#include "loginwidget.h"
#include "mediadevices/ipod/IpodDevice.h"
#include "MediaDeviceScrobbler.h"
#include "RestStateMessage.h"
#include "metadata.h"
#include "MetaDataWidget.h"
#include "playerlistener.h"
#include "Radio.h"
#include "ShareDialog.h"
#include "settingsdialog.h"
#include "DiagnosticsDialog.h"
#include "Scrobbler-1.2.h"
#include "LastFmSettings.h"
#include "SideBarView.h"
#include "systray.h"
#include "tagdialog.h"
#include "RestStateWidget.h"
#include "updatewizard.h"
#include "User.h"
#include "toolbarvolumeslider.h"
#include "WebService.h"
#include "WebService/Request.h"
#include "StationUrl.h"

#include <QShortcut>
#include <QLabel>
#include <QCloseEvent>
#include <QDragMoveEvent>
#include <QDesktopServices>
#include <QFileDialog>

#ifndef Q_WS_MAC
    #include "winstyleoverrides.h"
#else
    #include "macstyleoverrides.h"
#endif

Container* Container::s_instance = 0;


Container::Container()
        : QMainWindow(),
          m_userCheck( false ),
          m_sidebarEnabled( false ),
          m_sidebarWidth( 190 )
#ifndef Q_WS_MAC
        , m_styleOverrides( 0 )
#endif
{
    s_instance = this;
    m_shareDialog = new ShareDialog( this );
    m_diagnosticsDialog = new DiagnosticsDialog( this );
    m_updater = new CAutoUpdater( this );

    setupUi();
    setupTimeBar();
    setupTrayIcon();
    updateAppearance();
    applyPlatformSpecificTweaks();
    applyMenuTweaks();
    setupConnections();
    restoreState();
}


void
Container::setupUi()
{
    ui.setupUi( this );

    ui.actionCheckForUpdates->setMenuRole( QAction::ApplicationSpecificRole );
    ui.actionAboutLastfm->setMenuRole( QAction::AboutRole );
    ui.actionSettings->setMenuRole( QAction::PreferencesRole );
    ui.actionStop->setVisible( false );
    ui.statusbar->addPermanentWidget( ui.scrobbleLabel = new ScrobbleLabel );

////// PlayControls
    QWidget *playControls = new QWidget;
    ui.playcontrols.setupUi( playControls );
    ui.toolbar->addWidget( playControls );

////// splitter
    ui.splitter->setCollapsible( 0, false );
    ui.splitter->setCollapsible( 1, false );
    ui.splitter->setStretchFactor( 0, 0 );
    ui.splitter->setStretchFactor( 1, 1 );

////// force a white background as we don't support colour variants, soz 
    QPalette p = centralWidget()->palette();
    p.setBrush( QPalette::Base, Qt::white );
    p.setBrush( QPalette::Text, Qt::black );
    centralWidget()->setPalette( p );

////// Main Widgets
    ui.restStateWidget = new RestStateWidget( this );
    ui.metaDataWidget = new MetaDataWidget( this );

////// SideBar
    ui.sidebar = new SideBarTree( this );
    ui.sidebarFrame->layout()->addWidget( ui.sidebar );
    ui.sidebarFrame->hide();
    connect( ui.sidebar, SIGNAL( statusMessage( QString ) ), SLOT( statusMessage( QString ) ) );
    connect( ui.sidebar, SIGNAL( plsShowRestState() ), SLOT( showRestState() ) );
    connect( ui.sidebar, SIGNAL( plsShowNowPlaying() ), SLOT( showMetaDataWidget() ) );

////// ui.stack
    ui.stack->setBackgroundRole( QPalette::Base );
    ui.stack->addWidget( ui.restStateWidget );
    ui.stack->addWidget( ui.metaDataWidget );

#ifdef HIDE_RADIO
    ui.playcontrols.volume->setVisible( false );
    ui.menuControls->menuAction()->setVisible( false );
    ui.actionPlay->setVisible( false );
    ui.actionStop->setVisible( false );
    ui.actionSkip->setVisible( false );
    ui.actionBan->setVisible( false );
    ui.actionPlaylist->setVisible( false );
    ui.actionToggleDiscoveryMode->setVisible( false );
    ui.actionVolumeUp->setVisible( false );
    ui.actionVolumeDown->setVisible( false );
    ui.actionMute->setVisible( false );

    ui.stack->removeWidget( ui.restStateWidget );
#endif

    if ( qApp->arguments().contains( "--debug" ) )
        ui.menuHelp->addAction( "kr4sh pls, kthxbai", this, SLOT( crash() ) );

    ui.restStateWidget->setFocus();
}


void
Container::setupTimeBar()
{
    // Station bar grey bg colour
    const QColor k_stationBarGreyTop( 0xba, 0xba, 0xba, 0xff );
    const QColor k_stationBarGreyMiddle( 0xe2, 0xe2, 0xe2, 0xff );
    const QColor k_stationBarGreyBottom( 0xff, 0xff, 0xff, 0xff );

    // Track bar blue bg colour
    const QColor k_trackBarBkgrBlueTop( 0xeb, 0xf0, 0xf2, 0xff );
    const QColor k_trackBarBkgrBlueMiddle( 0xe5, 0xe9, 0xec, 0xff );
    const QColor k_trackBarBkgrBlueBottom( 0xdc, 0xe2, 0xe5, 0xff );

    // Track bar progress bar colour
    const QColor k_trackBarProgressTop( 0xd6, 0xde, 0xe6, 0xff );
    const QColor k_trackBarProgressMiddle( 0xd0, 0xd9, 0xe2, 0xff );
    const QColor k_trackBarProgressBottom( 0xca, 0xd4, 0xdc, 0xff );

    // Track bar scrobbled colour
    const QColor k_trackBarScrobbledTop( 0xba, 0xc7, 0xd7, 0xff );
    const QColor k_trackBarScrobbledMiddle( 0xb8, 0xc4, 0xd5, 0xff );
    const QColor k_trackBarScrobbledBottom( 0xb5, 0xc1, 0xd2, 0xff );

    ////// song bar and scrobble bar
    ui.songTimeBar->setEnabled( false );
    ui.songTimeBar->setItemType( UnicornEnums::ItemTrack );

    QLinearGradient trackBarBkgrGradient( 0, 0, 0, ui.songTimeBar->height() );
    trackBarBkgrGradient.setColorAt( 0, k_trackBarBkgrBlueTop );
    trackBarBkgrGradient.setColorAt( 0.5, k_trackBarBkgrBlueMiddle );
    trackBarBkgrGradient.setColorAt( 0.51, k_trackBarBkgrBlueBottom );
    trackBarBkgrGradient.setColorAt( 1, k_trackBarBkgrBlueBottom );

    QLinearGradient trackBarProgressGradient( 0, 0, 0, ui.songTimeBar->height() );
    trackBarProgressGradient.setColorAt( 0, k_trackBarProgressTop );
    trackBarProgressGradient.setColorAt( 0.5, k_trackBarProgressMiddle );
    trackBarProgressGradient.setColorAt( 0.51, k_trackBarProgressBottom );
    trackBarProgressGradient.setColorAt( 1, k_trackBarProgressBottom );

    QLinearGradient trackBarScrobbledGradient( 0, 0, 0, ui.songTimeBar->height() );
    trackBarScrobbledGradient.setColorAt( 0, k_trackBarScrobbledTop );
    trackBarScrobbledGradient.setColorAt( 0.5, k_trackBarScrobbledMiddle );
    trackBarScrobbledGradient.setColorAt( 0.51, k_trackBarScrobbledBottom );
    trackBarScrobbledGradient.setColorAt( 1, k_trackBarScrobbledBottom );

    ui.songTimeBar->setBackgroundGradient( trackBarBkgrGradient );
    ui.songTimeBar->setForegroundGradient( trackBarProgressGradient );
    ui.songTimeBar->setScrobbledGradient( trackBarScrobbledGradient );

    ui.stationTimeBar->setProgressEnabled( false );
    ui.stationTimeBar->setVisible( false );
    ui.stationTimeBar->setItemType( UnicornEnums::ItemStation );

    QLinearGradient stationBarGradient( 0, 0, 0, ui.stationTimeBar->height() );
    stationBarGradient.setColorAt( 0, k_stationBarGreyTop );
    stationBarGradient.setColorAt( 0.07, k_stationBarGreyMiddle );
    stationBarGradient.setColorAt( 0.14, k_stationBarGreyBottom );
    stationBarGradient.setColorAt( 1, k_stationBarGreyBottom );

    ui.stationTimeBar->setBackgroundGradient( stationBarGradient );
    ui.stationTimeBar->setStopWatch( &The::radio().stationStopWatch() );

    if ( qApp->arguments().contains( "--sanity" ) )
        stationBarGradient = trackBarBkgrGradient;
}


void
Container::setupTrayIcon()
{
    Q_DEBUG_BLOCK;

    QMenu *menu = new QMenu( this );
    menu->addAction( tr( "Open" ), this, SLOT( restoreWindow() ) );

  #ifdef Q_WS_MAC
    menu->addSeparator();
    menu->addAction( ui.actionSettings );
  #endif
    menu->addSeparator();
    menu->addAction( ui.actionDashboard );
    menu->addAction( ui.actionToggleScrobbling );
    (menu->addAction( tr( "Change User" ) ) )->setMenu( ui.menuUser );
    menu->addSeparator();
    menu->addAction( ui.actionShare );
    menu->addAction( ui.actionTag );
    menu->addAction( ui.actionPlaylist );
    menu->addSeparator();
    menu->addAction( ui.actionLove );
    menu->addAction( ui.actionBan );
  #ifndef HIDE_RADIO
    menu->addSeparator();
    menu->addAction( ui.actionPlay );
    menu->addAction( ui.actionStop );
    menu->addAction( ui.actionSkip );
  #endif
    menu->addSeparator();
    menu->addAction( ui.actionQuit );

  #ifdef Q_WS_MAC
    // strangely text is amended in Application menu, but nowhere else
    ui.actionSettings->setText( tr( "Preferences..." ) );
    ui.actionQuit->setText( tr( "Quit Last.fm" ) );
  #endif

    m_trayIcon = new TrayIcon( this );
    m_trayIcon->setContextMenu( menu );

    connect( m_trayIcon,
             SIGNAL( activated( QSystemTrayIcon::ActivationReason ) ),
             SLOT( onTrayIconActivated( QSystemTrayIcon::ActivationReason ) ) );

    connect( &The::settings(),
             SIGNAL( userSettingsChanged( LastFmUserSettings& ) ),
             m_trayIcon,
             SLOT( setUser( LastFmUserSettings& ) ) );
}


void
Container::applyMenuTweaks()
{
    // Qt 4.3.4, setSeparatorsCollapsible doesn't work as advertised :(
    foreach( QAction* a, menuBar()->actions() )
    {
        bool lastItemWasSeparator = false;
        foreach( QAction* b, a->menu()->actions() )
        {
            if ( !b->isVisible() )
                continue;

            if ( b->isSeparator() )
            {
                if ( lastItemWasSeparator )
                    delete b;

                lastItemWasSeparator = true;
            }
            else
                lastItemWasSeparator = false;
        }
    }
}


void
Container::applyPlatformSpecificTweaks()
{
#ifdef Q_WS_X11
    ui.actionCheckForUpdates->setVisible( false );
    ui.actionQuit->setShortcut( tr( "CTRL+Q" ) );
    ui.actionQuit->setText( tr( "&Quit" ) );

    m_styleOverrides = new WinXPStyleOverrides;
    m_styleOverrides->setParent( this );
    ui.statusbar->setStyle( m_styleOverrides ); //no ugly surrounding lines

    setWindowIcon( QIcon( MooseUtils::dataPath( "icons/as.png" ) ) );
    //aesthetics, separates the statusbar and central widget slightly
    static_cast<QVBoxLayout*>(ui.centralwidget->layout())->addSpacing( 2 );
#else
    ui.actionScrobbleManualIPod->setVisible( false );
#endif

#ifdef WIN32
    // Can't use 32 for height as labels get truncated on Win classic
    ui.toolbar->setIconSize( QSize( 50, 34 ) );
    ui.menuTools->insertSeparator( ui.actionPlaylist );

    // This is in order to remove the borders around the status bar widgets
    if ( qApp->style()->objectName() != "windowsxp" )
    {
        m_styleOverrides = new WinStyleOverrides();
        statusBar()->setStyle( m_styleOverrides );
        ui.stack->layout()->setMargin( 1 );
    }
    else
    {
        m_styleOverrides = new WinXPStyleOverrides;
        m_styleOverrides->setParent( this );
        statusBar()->setStyle( m_styleOverrides );
    }
#endif

#ifdef Q_WS_MAC
    setUnifiedTitleAndToolBarOnMac( true );
    ui.toolbar->setAttribute( Qt::WA_MacNoClickThrough );
    ui.actionFAQ->setShortcut( tr( "Ctrl+?" ) );
    ui.splitter->setHandleWidth( 1 );
    ui.actionMute->setShortcut( QKeySequence( Qt::CTRL + Qt::ALT + Qt::Key_Down ) );
    ui.menuFile->menuAction()->setVisible( false );

    qApp->setStyle( new MacStyleOverrides() );

    QPalette p = ui.centralwidget->palette();
    p.setColor( QPalette::Window, QColor( 0xe9, 0xe9, 0xe9 ) );
    ui.centralwidget->setPalette( p );

    p = statusBar()->palette();
    p.setColor( QPalette::WindowText, QColor( 0x59, 0x59, 0x59 ) );
    ui.scrobbleLabel->label()->setPalette( p );

    QLinearGradient g( 0.5, 0.0, 0.5, 20.0 );
    g.setColorAt( 0.0, QColor( 0x8c, 0x8c, 0x8c ) );
    g.setColorAt( 0.05, QColor( 0xf7, 0xf7, 0xf7 ) );
    g.setColorAt( 1.0, QColor( 0xe4, 0xe4, 0xe4 ) );
    p.setBrush( QPalette::Window, QBrush( g ) );
    ui.statusbar->setPalette( p );

    // mac specific mainwindow adjustments
    QFrame *hline = new QFrame;
    hline->setFrameStyle( QFrame::HLine | QFrame::Plain );
    p.setColor( QPalette::WindowText, QColor( 140, 140, 140 ) );
    hline->setPalette( p );
    ui.vboxLayout1->insertWidget( ui.vboxLayout1->indexOf( ui.stack ), hline );
    ui.stack->setFrameStyle( QFrame::NoFrame );

    //alter the spacings and that
    ui.vboxLayout2->setMargin( 14 );
    for ( int x = 0; x < ui.vboxLayout1->count(); ++x )
        if ( ui.vboxLayout1->itemAt( x )->spacerItem() )
            delete ui.vboxLayout1->takeAt( x );

    QFont f = ui.statusbar->font();
    f.setPixelSize( 10 );
    ui.statusbar->setFont( f );
    ui.scrobbleLabel->label()->setFont( f );
#endif

#ifndef WIN32
    ui.actionGetPlugin->setEnabled( false );
    ui.actionGetPlugin->setVisible( false );
#endif
}


void
Container::setupConnections()
{
    connect( ui.actionDashboard, SIGNAL( triggered() ), SLOT( gotoProfile() ) );
    connect( ui.actionSettings, SIGNAL( triggered() ), SLOT( showSettingsDialog() ) );
    connect( ui.actionGetPlugin, SIGNAL( triggered() ), SLOT( getPlugin() ) );
    connect( ui.actionCheckForUpdates, SIGNAL( triggered() ), SLOT( checkForUpdates() ) );
    connect( ui.actionAddUser, SIGNAL( triggered() ), SLOT( addUser() ) );
    connect( ui.actionDeleteUser, SIGNAL( triggered() ), SLOT( deleteUser() ) );
    connect( ui.actionToggleScrobbling, SIGNAL( triggered() ), SLOT( toggleScrobbling() ) );
    connect( ui.actionToggleDiscoveryMode, SIGNAL( triggered() ), SLOT( toggleDiscoveryMode() ) );
    connect( ui.actionAboutLastfm, SIGNAL( triggered() ), SLOT( about() ) );
    connect( ui.menuUser, SIGNAL( aboutToShow() ), SLOT( onAboutToShowUserMenu() ) );
    connect( ui.menuUser, SIGNAL( triggered( QAction* ) ), SLOT( onUserSelected( QAction* ) ) );
    connect( ui.actionSkip, SIGNAL( triggered() ), SLOT(skip() ) );
    connect( ui.actionStop, SIGNAL( triggered() ), SLOT(stop() ) );
    connect( ui.actionPlay, SIGNAL( triggered() ), SLOT( play() ) );
    connect( ui.scrobbleLabel, SIGNAL( clicked() ), SLOT( toggleScrobbling() ) );
    connect( ui.actionVolumeUp, SIGNAL( triggered() ), SLOT( volumeUp() ) );
    connect( ui.actionVolumeDown, SIGNAL( triggered() ), SLOT( volumeDown() ) );
    connect( ui.actionMute, SIGNAL( triggered() ), SLOT( mute() ) );
    connect( ui.actionFAQ, SIGNAL( triggered() ), SLOT( showFAQ() ) );
    connect( ui.actionForums, SIGNAL( triggered() ), SLOT( showForums() ) );
    connect( ui.actionInviteAFriend, SIGNAL( triggered() ), SLOT( inviteAFriend() ) );
    connect( ui.actionDiagnostics, SIGNAL( triggered() ), SLOT( showDiagnosticsDialog() ) );
    connect( ui.actionSendToTray, SIGNAL( triggered() ), SLOT( close() ) );
    connect( ui.actionQuit, SIGNAL( triggered() ), SLOT( quit() ) );
    connect( ui.playcontrols.volume, SIGNAL( valueChanged( int ) ), &The::radio(), SLOT( setVolume( int ) ) );
    connect( The::webService(), SIGNAL( success( Request* ) ), SLOT( webServiceSuccess( Request* ) ) );
    connect( The::webService(), SIGNAL( failure( Request* ) ), SLOT( webServiceFailure( Request* ) ), Qt::QueuedConnection );
    connect( &The::settings(), SIGNAL( userSettingsChanged( LastFmUserSettings& ) ), SLOT( updateUserStuff( LastFmUserSettings& ) ) );
    connect( &The::settings(), SIGNAL( appearanceSettingsChanged() ), SLOT( updateAppearance() ) );
    connect( m_updater, SIGNAL( updateCheckDone( bool, bool, QString ) ), SLOT( updateCheckDone( bool, bool, QString ) ) );
    connect( ui.stack, SIGNAL( currentChanged( int ) ), SIGNAL( stackIndexChanged( int ) ) );
    connect( ui.actionMyProfile, SIGNAL( triggered() ), SLOT( toggleSidebar() ) );
    connect( ui.actionPlaylist, SIGNAL( triggered() ), SLOT( addToMyPlaylist() ) );
    connect( ui.actionTag, SIGNAL( triggered() ), SLOT( showTagDialog() ) );
    connect( ui.actionShare, SIGNAL( triggered() ), SLOT( showShareDialog() ) );
    connect( ui.actionLove, SIGNAL( triggered() ), SLOT( love() ) );
    connect( ui.actionBan, SIGNAL( triggered() ), SLOT( ban() ) );
    connect( ui.metaDataWidget, SIGNAL( tagButtonClicked() ), SLOT( showTagDialogMD() ) );
    connect( ui.metaDataWidget, SIGNAL( urlHovered( QUrl ) ), SLOT( displayUrlInStatusBar( QUrl ) ) );
    connect( ui.actionScrobbleManualIPod, SIGNAL( triggered() ), SLOT( scrobbleManualIpod() ) );
    connect( ui.splitter, SIGNAL( splitterMoved( int, int ) ), SLOT( splitterMoved( int ) ) );

////// important connections
    connect( &The::scrobbler(),
             SIGNAL( status( int, QVariant ) ),
             SLOT( onScrobblerStatusChange( int, QVariant ) ) );
    connect( &The::radio(),
             SIGNAL( error( RadioError, QString ) ),
             SLOT( onRadioError( RadioError, QString ) ), Qt::QueuedConnection /* crash if direct connect */ );
    connect( &The::radio(),
             SIGNAL( stateChanged( RadioState ) ),
             SLOT( onRadioStateChanged( RadioState ) ) );
    connect( &The::radio(),
             SIGNAL( buffering( int, int ) ),
             SLOT( onRadioBuffering( int, int ) ) );

    connect( qApp, SIGNAL( event( int, QVariant ) ), SLOT( onAppEvent( int, QVariant ) ) );

////// we don't use CTRL because that varies by platform, NOTE don't tr()!
    QObject* hides  = new QShortcut( QKeySequence( "Ctrl+W" ), this );
    QObject* openL = new QShortcut( QKeySequence( "Alt+Shift+L" ), this );
    QObject* openF = new QShortcut( QKeySequence( "Alt+Shift+F" ), this );
    connect( hides, SIGNAL( activated() ), SLOT( hide() ) );
    connect( openL, SIGNAL( activated() ), SLOT( onAltShiftL() ) );
    connect( openF, SIGNAL( activated() ), SLOT( onAltShiftF() ) );
#ifdef WIN32
    QObject* openP = new QShortcut( QKeySequence( "Alt+Shift+P" ), this );
    connect( openP, SIGNAL( activated() ), SLOT( onAltShiftP() ) );
#endif
}


void
Container::restoreState()
{
    //NOTE it is important that the connections are done first
    ui.playcontrols.volume->setValue( The::settings().volume() );

    if ( The::currentUser().sidebarWidth() > 0 )
    {
        m_sidebarWidth = The::currentUser().sidebarWidth();
        LOGL( 3, "Restoring sidebar width: " << m_sidebarWidth );
    }

    if ( The::currentUser().sidebarEnabled() )
    {
        ui.splitter->setSizes( QList<int>() << m_sidebarWidth );
    }

    // figure out minimum width of our toolbar (this will change due to translations!)
    int w = width();
    m_geometry = The::settings().containerGeometry();
    if ( m_geometry.isEmpty() )
    {
        w = 0;
        QList<QAction*> a = ui.toolbar->actions();

        for( QList<QAction*>::iterator i = a.begin(); i != a.end(); ++i )
        {
            if ( (*i)->isSeparator() || !(*i)->isVisible() )
                continue;

            QWidget *widget = ui.toolbar->widgetForAction( *i );

            // don't adjust the playcontrols, their size-policy is expanding and will mess up this calculation
            if ( widget->objectName() != "PlayControls" )
            {
                widget->adjustSize();
                w += widget->width();
            }
            else
            {
                w += 140; // HACK: i couldn't figure out a proper way to get the mnimum width of the playcontrols
            }
        }

        w += 16; // spacing
        if ( w < width() )
            w = width();
    }

#ifdef Q_WS_MAC
    // on mac qt returns height without unified toolbar height included :(
    // so first time we have to do it like this
    if ( m_geometry.isEmpty() )
        resize( w, 496 );
    else
        restoreGeometry( m_geometry );
#else
    restoreGeometry( m_geometry );

    if ( m_geometry.isEmpty() )
        resize( w, height() );
#endif
    setWindowState( The::settings().containerWindowState() );
}


Container::~Container()
{
    LOGL( 3, "Saving app state" );

    The::settings().setContainerGeometry( m_geometry );
    The::settings().setContainerWindowState( windowState() );
    The::settings().setVolume( ui.playcontrols.volume->value() );
    The::currentUser().setSidebarEnabled( m_sidebarEnabled );
    The::currentUser().setSidebarWidth( m_sidebarWidth );

    LOGL( 3, "Saving config" );
    QSettings().sync();
}


void
Container::quit()
{
    if ( ConfirmDialog::quit( this ) )
    {
        qApp->quit();
    }
}


void
Container::closeEvent( QCloseEvent *event )
{
    bool quit = false;

    #ifdef Q_WS_MAC
    if ( !event->spontaneous() )
        quit = true;
    else
    #endif

    // ELSE FOR OSX ABOVE!
    // FIXME check for showDockIcon too, or this is totally stupid
    // NOTE there is no setting, it's all determined in settingsDialog
    if ( !The::settings().showTrayIcon() )
    {
      #ifdef Q_WS_X11
        quit = true;
      #else
        if ( !ConfirmDialog::hide( this ) )
        {
            event->ignore();
            return;
        }
      #endif
    }

    if ( quit )
    {
        qApp->quit();
        event->accept();
    }
    else
    {
        // Just minimise to tray
        minimiseToTray();
        event->ignore();
    }
}


void
Container::minimiseToTray()
{
    hide();

  #ifdef WIN32
    // Do animation and fail gracefully if not possible to find systray
    RECT rectFrame;    // animate from
    RECT rectSysTray;  // animate to

    ::GetWindowRect( (HWND)winId(), &rectFrame );

    // Get taskbar window
    HWND taskbarWnd = ::FindWindow( L"Shell_TrayWnd", NULL );
    if ( taskbarWnd == NULL )
        return;

    // Use taskbar window to get position of tray window
    HWND trayWnd = ::FindWindowEx( taskbarWnd, NULL, L"TrayNotifyWnd", NULL );
    if ( trayWnd == NULL )
        return;

    ::GetWindowRect( trayWnd, &rectSysTray );
    ::DrawAnimatedRects( (HWND)winId(), IDANI_CAPTION, &rectFrame, &rectSysTray );

    // Make it release memory as when minimised
    HANDLE h = ::GetCurrentProcess();
    SetProcessWorkingSetSize( h, -1 ,-1 );
  #endif // WIN32
}


void
Container::dragMoveEvent( QDragMoveEvent* event )
{
  #ifndef HIDE_RADIO
    QString url = event->mimeData()->urls().value( 0 ).toString();
    if ( url.startsWith( "lastfm://" ) )
    {
        event->acceptProposedAction();
    }
    else
        event->ignore();
  #endif // HIDE_RADIO
}


void
Container::dragEnterEvent( QDragEnterEvent* event )
{
  #ifndef HIDE_RADIO
    QString url = event->mimeData()->urls().value( 0 ).toString();
    if ( url.startsWith( "lastfm://" ) )
    {
        event->acceptProposedAction();
    }
    else
        event->ignore();
  #endif // HIDE_RADIO
}


void
Container::dropEvent( QDropEvent* event )
{
  #ifndef HIDE_RADIO
    QString url = event->mimeData()->urls().value( 0 ).toString();
    if ( url.startsWith( "lastfm://" ) )
    {
        The::radio().playStation( StationUrl( url ) );
    }
  #endif // HIDE_RADIO
}


bool
Container::event( QEvent* e )
{
    #ifdef Q_WS_MAC
    //TODO remove when Qt is fixed!
    if ( e->type() == QEvent::Resize )
    {
        // hack as Qt 4.3.1 is b0rked for unified toolbars
        ui.toolbar->setMaximumWidth( width() );
    }
    #endif

    if ( e->type() == QEvent::Move || e->type() == QEvent::Resize )
    {
        // Again, Qt is broken, if maximised the saveGeometry function fails to
        // save the geometry for the non-maximised state. So instead we must save it
        // for *every* resize and move event. Yay!

        if (windowState() != Qt::WindowMaximized)
            // Such frequent calls to setContainerGeometry() are very cpu intensive.
            // Just dump to a QByteArray and save it in the destructor.
            //The::settings().setContainerGeometry( saveGeometry() );
            m_geometry = saveGeometry();
    }

    if ( e->type() == QEvent::Show )
    {
        emit becameVisible();
    }

    return QMainWindow::event( e );
}


void
Container::toggleDiscoveryMode()
{
    bool enabled = ui.actionToggleDiscoveryMode->isChecked();
    The::radio().setDiscoveryMode( enabled );
}


void
Container::displayUrlInStatusBar( const QUrl& url )
{
    statusBar()->showMessage( UnicornUtils::urlDecodeItem( url.toString() ) );
}


void
Container::onRadioError( RadioError error, const QString& message )
{
  #ifndef HIDE_RADIO

    QString msgboxTitle = tr( "Error" );

    removeDMCAWarnings();

    switch( error )
    {
        // TODO: do we really want a statusbar message for all of these?
        case Request_Aborted:
        case Request_BadResponseCode:
        case Request_HostNotFound:
        case Request_NoResponse:
        {
            statusBar()->showMessage( message );
        }
        break;

        case Request_ProxyAuthenticationRequired:
        {
            //TEMP:
            FailedLoginDialog( this ).exec();
        }
        break;

        case Request_WrongUserNameOrPassword:
        {
            statusBar()->showMessage( message );
            showSettingsDialog();
        }
        break;

        case Request_Gone:
        {
            LastMessageBox::critical( tr( "This station is no longer available" ), message, QMessageBox::Ok, QMessageBox::Ok, QStringList( tr("Dismiss")) );

            if( The::radio().stationUrl().endsWith( "/loved" )) {
                QTimer::singleShot( 0, ui.sidebar->model(), SLOT( removeLovedTracks()));
            }
            
            if( The::radio().stationUrl().startsWith( "lastfm://usertags/" )) {
                The::user().settings().setUserTagsRadioHidden( true );
                The::radio().playStation( The::radio().stationUrl().replace(QRegExp("lastfm://usertags/[^/]*/"), "lastfm://globaltags/"));
            }
        }
        break;

        case Handshake_Banned:
        {
            //TODO make a nicer shutdown (or start upgrade process) via a signal to the container
            LastMessageBox::critical( tr( "Bad Version" ), message );
            qApp->quit();
        }
        break;

        // Critical box
        case Handshake_SessionFailed:
        case Radio_PluginLoadFailed:
        case Radio_NoSoundcard:
        case Radio_PlaybackError:
        case Radio_UnknownError:
        {
            statusBar()->showMessage( "" );
            LastMessageBox::critical( msgboxTitle, message );
        }
        break;

        // Info box
        case ChangeStation_SubscribersOnly:
            msgboxTitle = ""; // can't translate yet
            // FALL THROUGH
        
        case Radio_SkipLimitExceeded:
        case ChangeStation_NotEnoughContent:
        case ChangeStation_TooFewGroupMembers:
        case ChangeStation_TooFewFans:
        case ChangeStation_TooFewNeighbours:
        case ChangeStation_Unavailable:

        case ChangeStation_StreamerOffline:
        case ChangeStation_UnknownError:
        case Radio_BadPlaylist:
        case Radio_IllegalResume:
        case Radio_InvalidUrl:
        case Radio_InvalidAuth:
        case Radio_TrackNotFound:
        case Radio_OutOfPlaylist:
        case Radio_TooManyRetries:
        case Radio_ConnectionRefused:
        case Playlist_RecSysDown:
        {
            statusBar()->clearMessage();
            LastMessageBox::information( msgboxTitle, message );
        }
        break;
        
        case Radio_FreeTrialExpired:
        {
            ui.restStateWidget->setFreeTrialStatus( 2 );
            
            int const r = LastMessageBox::information( 
                tr("Did you enjoy it?"),
                tr("Your free trial has expired. <a href='http://last.fm/subscribe'>Subscribe</a> to keep listening to non-stop, personalised radio!"),
                QMessageBox::Ok | QMessageBox::Cancel,
                QMessageBox::Ok,
                QStringList() << tr("Subscribe") << tr("Cancel") );

            if (QMessageBox::Ok == r)
            {
                QDesktopServices::openUrl( QUrl("http://last.fm/subscribe") );
            }
        }
        break;
            
        default:
            Q_ASSERT( !"Is it correct to leave some unhandled?" );
    }

    LOGL( 2, "Radio error " << error << ": " << message );

  #endif // HIDE_RADIO
}


void
Container::toggleSidebar()
{
    QIcon icon;
    m_sidebarEnabled = !m_sidebarEnabled;

    if ( !m_sidebarEnabled )
    {
        icon = IconShack::instance().GetGoodUserIconExpanded( The::currentUser().icon() );

      #ifndef Q_WS_MAC
        centralWidget()->setContentsMargins( 5, 0, 5, 0 ); //aesthetics
      #endif
    }
    else
    {
      #ifndef Q_WS_MAC
        centralWidget()->setContentsMargins( 0, 0, 5, 0 ); //aesthetics
      #endif

        icon = IconShack::instance().GetGoodUserIconCollapsed( The::currentUser().icon() );
        ui.splitter->setSizes( QList<int>() << m_sidebarWidth );
    }

    ui.actionMyProfile->setChecked( m_sidebarEnabled );
    ui.actionMyProfile->setIcon( icon );
    ui.sidebar->parentWidget()->setVisible( m_sidebarEnabled );
}


void
Container::getPlugin()
{
    ConfigWizard( this, ConfigWizard::Plugin ).exec();
    ui.restStateWidget->updatePlayerNames();
}


void
Container::checkForUpdates( bool invokedByUser )
{
    m_userCheck = invokedByUser;
    if ( m_userCheck )
    {
        QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
    }

    m_updater->checkForUpdates();
}


void
Container::updateCheckDone( bool updatesAvailable, bool error, QString errorMsg )
{
    QApplication::restoreOverrideCursor();

    if ( error )
    {
        // Can't connect to the internet
        LOG( 2, "Update check failed. Error: " << errorMsg << "\n" );
        if ( m_userCheck )
        {
            LastMessageBox::critical( tr( "Connection Problem" ),
                tr( "Last.fm couldn't connect to the Internet to check "
                    "for updates.\n\nError: %1" ).arg( errorMsg ) );
        }
        return;
    }

    // No connection error, let's see if we have updates
    if ( !updatesAvailable )
    {
        LOG( 3, "Update check said no updates available.\n" );
        if ( m_userCheck )
        {
            LastMessageBox::information( tr( "Up To Date" ),
                tr( "No updates available. All your software is up to date!\n" ) );
        }
        return;
    }

    // Go ahead and launch update wizard
    LOG( 3, "New updates available. Launching update wizard.\n" );

    UpdateWizard* wizard = new UpdateWizard( *m_updater, this );
    if( wizard->shouldShow()) {
        wizard->exec();
        wizard->deleteLater();
    }

#ifdef WIN32
    // this is really only for beta testers,
    // ie. User installs 1.5 via installer, not upgrade path, but they had the
    // itunes plugin version 2 installed already, so upgrade wizard wasn't run
    // in previous instantiation. So we run configwizard to bootstrap twiddly
    //NOTE some of this code is duplicated in LastFmApplication::init()
    if (The::settings().weWereJustUpgraded())
        ConfigWizard( NULL, ConfigWizard::MediaDevice ).exec();
#endif
}


void
Container::showFAQ()
{
    QDesktopServices::openUrl( "http://" + UnicornUtils::localizedHostName( The::settings().appLanguage() ) + "/help/faq/" );
}


void
Container::showForums()
{
    QDesktopServices::openUrl( "http://" + UnicornUtils::localizedHostName( The::settings().appLanguage() ) + "/forum/34905/" );
}


void
Container::inviteAFriend()
{
    QByteArray user = QUrl::toPercentEncoding( The::settings().currentUsername() );
    QDesktopServices::openUrl( "http://" + UnicornUtils::localizedHostName( The::settings().appLanguage() ) + "/user/" + user + "/inviteafriend/" );
}



void
Container::about()
{
    AboutDialog* d = findChild<AboutDialog*>();
    if ( !d )
    {
        (d = new AboutDialog( this ))->show();
        d->setAttribute( Qt::WA_DeleteOnClose );
    }
    else
        d->raise();
}


void
Container::onAboutToShowUserMenu()
{
    Q_DEBUG_BLOCK;

    // Work out whether we have old items to delete
    QList<QAction*> actions = ui.menuUser->actions();
    if ( actions.size() != 3 )
    {
        // Delete old items
        for ( int i = actions.size() - 1; i >= 3; --i )
        {
            ui.menuUser->removeAction( actions.at( i ) );
        }
    }

    // For each user, add them and put a check next to the current
    QString currentUser = The::settings().currentUsername();
    QStringList users = The::settings().allUsers();
    for ( int i = 0; i < users.size(); ++i )
    {
        QString username = users.at( i );
        QAction* action = new QAction( ui.menuUser );
        action->setData( "user" );
        action->setText( username );
        action->setCheckable( true );
        action->setChecked( username == currentUser );

        ui.menuUser->addAction( action );
    }
}


void
Container::onUserSelected( QAction* action )
{
    The::settings().currentUser().setSidebarEnabled( m_sidebarEnabled );

    if ( action->data().toString() == "user" )
    {
        QString username = action->text();
        LOGL( 3, "Switching user to: " << username );

        if ( !The::settings().user( username ).rememberPass() )
        {
            LoginWidget( this, LoginWidget::LOGIN, username ).createDialog().exec();
        }
        else
        {
            The::app().setUser( username );
        }
    }
}


void
Container::addUser()
{
    LoginWidget( this, LoginWidget::ADD_USER ).createDialog().exec();
}


void
Container::deleteUser()
{
    DeleteUserDialog( this ).exec();
}


void
Container::toggleScrobbling()
{
    The::settings().currentUser().toggleLogToProfile();
}


void
Container::updateWindowTitle( const MetaData& data )
{
    QString title;

    // on mac it is atypical to show document info for non multi document UIs
    #ifndef Q_OS_MAC
        QString track = data.toString();
    #else
        QString track;
    #endif

    if ( track.isEmpty() )
        title += "Last.fm";
    else
        title += track;

    if ( The::settings().allUsers().count() > 1 )
        title += " | " + The::currentUsername();

    if ( qApp->arguments().contains( "--debug" ) )
        title += " [debug]";

    setWindowTitle( title );
}


void
Container::updateUserStuff( LastFmUserSettings& user )
{
    Q_DEBUG_BLOCK;

    updateWindowTitle( The::app().currentTrack() );

    QPixmap pix = m_sidebarEnabled
            ? IconShack::instance().GetGoodUserIconCollapsed( user.icon() )
            : IconShack::instance().GetGoodUserIconExpanded( user.icon() );
    ui.actionMyProfile->setIcon( QIcon( pix ) );

    bool const enabled = user.isLogToProfile();
    ui.songTimeBar->setScrobblingEnabled( enabled );
    ui.actionToggleScrobbling->setChecked( enabled );
    ui.actionToggleDiscoveryMode->setChecked( user.isDiscovery() );

    ui.scrobbleLabel->setEnabled( enabled );
}


void
Container::updateAppearance()
{
    Q_DEBUG_BLOCK;

    if ( !The::settings().showTrayIcon() )
        m_trayIcon->hide();
    else
        m_trayIcon->show();
}


void
Container::onRadioBuffering( int size, int total )
{
  #ifndef HIDE_RADIO

    bool finished = size == total;

    if ( finished )
    {
        statusBar()->clearMessage();
    }
    else
    {
        int percent = (int)( ( (float)size / total ) * 100 );
        statusBar()->showMessage( tr( "Buffering... (%1%)", "%1 is the percentage buffering is complete" ).arg( percent ) );
    }

  #endif // HIDE_RADIO
}


void
Container::onScrobblerStatusChange( int code, const QVariant& data )
{
    switch ( code )
    {
        case Scrobbler::ErrorBadAuthorisation:
            //NOTE radio is likely to fail auth too, so don't show 2 dialogs
            statusBar()->showMessage( tr( "Your username and password are incorrect" ) );
        break;

        case Scrobbler::Connecting:
            //TODO status system with temp messages or queue or something
            //statusBar()->showMessage( tr( "Connecting to Last.fm..." ) );
        break;

        case Scrobbler::Handshaken:
        break;

        case Scrobbler::Scrobbling:
            // we say submitting because we say "scrobbled" at 50%
            statusBar()->showMessage( tr( "Submitting %n scrobbles...", "", data.toInt() ) );
        break;

        case Scrobbler::TracksScrobbled:
            // we say submited because we say "scrobbled" at 50%
            statusBar()->showMessage( tr( "%n scrobbles submitted", "", data.toInt() ) );
        break;

        case Scrobbler::TracksNotScrobbled:
            statusBar()->showMessage( tr( "%n scrobbles will be submitted later", "", data.toInt() ) );
        break;

        case Scrobbler::ErrorBannedClient:
            LastMessageBox::critical( tr( "Old Version" ), tr( "This software is too old, please upgrade." ) );
        break;

        case Scrobbler::ErrorBadTime:
            LastMessageBox::critical( tr( "Error" ),
                tr( "<p>Last.fm cannot authorise any scrobbling! :("
                    "<p>It appears your computer disagrees with us about what the time is."
                    "<p>If you are sure the time is right, check the date is correct and check your "
                       "timezone is not set to something miles away, like Mars." 
                    "<p>We're sorry about this restriction, but we impose it to help prevent "
                       "scrobble spamming." ) );
        break;
    }
}


void
Container::showTagDialog( int defaultTagType )
{
    TagDialog d( The::app().currentTrack(), this );
    if ( defaultTagType >= 0 )
        d.setTaggingType( defaultTagType );

    d.exec();
}


void
Container::showTagDialogMD()
{
    showTagDialog( 0 );
}


void
Container::play()
{
    The::radio().resumeStation();
}


void
Container::stop()
{
    The::radio().stop();
}


void
Container::volumeUp()
{
    if ( ui.playcontrols.volume->value() != 100 )
    {
        if ( ui.playcontrols.volume->value() + 5 > 100 )
            ui.playcontrols.volume->setValue( 100 );
        else
            ui.playcontrols.volume->setValue( ui.playcontrols.volume->value() + 5 );
    }
}


void
Container::volumeDown()
{
    if ( ui.playcontrols.volume->value() != 0 )
    {
        if ( ui.playcontrols.volume->value() - 5 < 0 )
            ui.playcontrols.volume->setValue( 0 );
        else
            ui.playcontrols.volume->setValue( ui.playcontrols.volume->value() - 5 );
    }
}


void
Container::mute()
{
    if ( ui.playcontrols.volume->value() != 0 )
    {
        m_lastVolume = ui.playcontrols.volume->value();
        ui.playcontrols.volume->setValue( 0 );
    }
    else
        ui.playcontrols.volume->setValue( m_lastVolume );
}


void
Container::addToMyPlaylist()
{
    // Make copy in case dialog stays on screen across track change
    MetaData track = The::app().currentTrack();

    if ( ConfirmDialog::playlist( track, this ) )
    {
        ui.actionPlaylist->setEnabled( false );
        (new AddToMyPlaylistRequest( track ))->start();
    }
}


void
Container::love()
{
    // Make copy in case dialog stays on screen across track change
    MetaData track = The::app().currentTrack();

    if ( ConfirmDialog::love( track, this ) )
    {
        ui.actionLove->setEnabled( false );

        (new LoveRequest( track ))->start();
    }

    // HACK: Only reason it's OK to do this is because the scrobbler will
    // make sure to not scrobble duplicates.
    track.setRatingFlag( TrackInfo::Loved );
    The::app().scrobbler().scrobble( track );
}


void
Container::ban()
{
    // Make copy in case dialog stays on screen across track change
    MetaData track = The::app().currentTrack();

    if ( ConfirmDialog::ban( track, this ) )
    {
        ui.actionSkip->setEnabled( false );
        ui.actionLove->setEnabled( false );
        ui.actionBan->setEnabled( false );
        ui.actionPlaylist->setEnabled( false );

        (new BanRequest( track ))->start();

        // Listener needs to know about banned tracks so that we can prevent
        // them from getting submitted.
        CPlayerConnection* player = The::app().listener().GetActivePlayer();
        if ( player != NULL )
        {
            player->ban();
        }

        // HACK: Only reason it's OK to do this is because the scrobbler will
        // make sure to not scrobble duplicates.
        track.setRatingFlag( TrackInfo::Banned );
        The::app().scrobbler().scrobble( track );

        The::radio().skip();
    }
}


void
Container::skip()
{
    ui.actionSkip->setEnabled( false );
    ui.actionLove->setEnabled( false );
    ui.actionBan->setEnabled( false );
    ui.actionPlaylist->setEnabled( false );

    ui.songTimeBar->clear();
    ui.songTimeBar->setEnabled( false );
    ui.songTimeBar->setText( tr( "Skipping..." ) );

    // HACK: Only reason it's OK to do this is because the scrobbler will
    // make sure to not scrobble duplicates.
    MetaData track = The::app().currentTrack();
    track.setRatingFlag( TrackInfo::Skipped );
    The::app().scrobbler().scrobble( track );

    The::radio().skip();
}


void
Container::gotoProfile()
{
    QByteArray user = QUrl::toPercentEncoding( The::settings().currentUsername() );
    QDesktopServices::openUrl( "http://" + UnicornUtils::localizedHostName( The::settings().appLanguage() ) + "/user/" + user );
}


void
Container::crash()
{
    delete ui.actionPlay;
    ui.actionPlay->trigger();
}


void
Container::showShareDialog()
{
    m_shareDialog->setSong( The::app().currentTrack() );
    m_shareDialog->exec();
}


void
Container::showSettingsDialog( int startPage )
{
    SettingsDialog settingsDialog( this );
    settingsDialog.exec( startPage );
}


void
Container::showDiagnosticsDialog()
{
    m_diagnosticsDialog->show();
}


void 
Container::showNotification( const QString& title, const QString& message )
{
    m_trayIcon->showMessage( title, message );
}


void
Container::showRestState()
{
    ui.stack->setCurrentIndex( 0 );
    
  #ifdef HIDE_RADIO
    // DO NOT REMOVE, this is needed for JP build
    ui.metaDataWidget->displayNotListening();
  #endif
}


void
Container::scrobbleManualIpod()
{
    QObject* o = QPluginLoader( MooseUtils::servicePath( "Ipod_device" ) ).instance();
    MyMediaDeviceInterface* plugin = qobject_cast<MyMediaDeviceInterface*>(o);

    if ( plugin )
    {
        QString path;

        QString const settings_path = "IpodDevice/" + plugin->uniqueId() + "/MountLocation";
        path = QFileDialog::getExistingDirectory(
                                     this,
                                     tr( "Where is your iPod mounted?" ),
                                     QSettings().value( settings_path, "/" ).toString() );
        if ( path.isEmpty() )
            return; // user pressed cancel

        QSettings().setValue( settings_path, path );

        plugin->setMountPath( path );

        qApp->setOverrideCursor( Qt::WaitCursor );
        QList<TrackInfo> tracks = plugin->tracksToScrobble();
        qApp->restoreOverrideCursor();

        qDebug() << "Manual iPod scrobbling found" << tracks.count();

        if ( tracks.count() )
        {
            IPodScrobbler s( The::user().name(), this );
            s.setTracks( tracks );
            s.setAlwaysConfirm( true );
            s.exec();
        }
        else if ( plugin->error().isEmpty() )
        {
            LastMessageBox::information(
                    tr( "Nothing to Scrobble" ),
                    tr( "You have not played anything since you last scrobbled this iPod." ) );
        }
        else
            LastMessageBox::information( tr( "Plugin Error" ), plugin->error() );
    }
    else
        LastMessageBox::warning(
                tr( "Warning" ),
                tr( "There was an error loading the IpodDevice plugin." ) );

    delete o;
}


void
Container::onTrayIconActivated( QSystemTrayIcon::ActivationReason reason )
{
    // typical linux behavior is single clicking tray icon toggles the main window

    #ifdef Q_WS_X11
    if (reason == QSystemTrayIcon::Trigger)
        toggleWindowVisibility();
    #else
    if (reason == QSystemTrayIcon::DoubleClick)
        toggleWindowVisibility();
    #endif
}


#ifdef Q_WS_X11
    // includes only relevent to this function - please leave here :)
    #include <QX11Info>
    #include <X11/Xlib.h>
    #include <X11/Xatom.h>
#endif

void
Container::toggleWindowVisibility()
{
    //TODO really we should check to see if the window itself is obscured?
    // hard to say as exact desire of user is a little hard to predict.
    // certainly we should raise the window if it isn't active as chances are it
    // is behind other windows

    if ( isVisible() )
        hide();
    else
    {
        #ifndef Q_WS_X11
            show(), activateWindow(), raise();
        #else
            show();

            //NOTE don't raise, as this won't work with focus stealing prevention
            //raise();

            QX11Info const i;
            Atom const _NET_ACTIVE_WINDOW = XInternAtom( i.display(), "_NET_ACTIVE_WINDOW", False);

            // this sends the correct demand for window activation to the Window 
            // manager. Thus forcing window activation.
            ///@see http://standards.freedesktop.org/wm-spec/wm-spec-1.3.html#id2506353
            XEvent e;
            e.xclient.type = ClientMessage;
            e.xclient.message_type = _NET_ACTIVE_WINDOW;
            e.xclient.display = i.display();
            e.xclient.window = winId();
            e.xclient.format = 32;
            e.xclient.data.l[0] = 1; // we are a normal application
            e.xclient.data.l[1] = i.appUserTime();
            e.xclient.data.l[2] = qApp->activeWindow() ? qApp->activeWindow()->winId() : 0;
            e.xclient.data.l[3] = 0l;
            e.xclient.data.l[4] = 0l;

            // we send to the root window per fdo NET spec
            XSendEvent( i.display(), i.appRootWindow(), false, SubstructureRedirectMask | SubstructureNotifyMask, &e );
        #endif
    }
}


//TODO mxcl, would be better to have this handled by the Requests themselves in some generic error handler
void
Container::webServiceSuccess( Request *r )
{
    switch ( r->type() )
    {
        case TypeSetTag:
            statusBar()->showMessage( tr( "You tagged %1 successfully" ).arg( static_cast<SetTagRequest*>(r)->title() ) );
            break;

        case TypeRecommend:
        {
            RecommendRequest* request = static_cast<RecommendRequest*>(r);
            QString recommendationType;
            switch( request->type() )
            {
                case 1:
                    recommendationType = tr( "artist" );
                break;

                case 2:
                    recommendationType = tr( "track" );
                break;

                case 3:
                    recommendationType = tr( "album" );
                break;

                default:
                break;
            }

            statusBar()->showMessage( tr( "This %1 has been shared with %2." ).arg( recommendationType ).arg( request->targetUsername() ) );
        }
        break;

        case TypeLove:
            statusBar()->showMessage( tr( "You added this track to your Loved tracks." ) );
        break;

        case TypeUnLove:
            statusBar()->showMessage( tr( "You removed this track from your Loved tracks." ) );
        break;

        case TypeBan:
            statusBar()->showMessage( tr( "You banned this track." ) );
        break;

        case TypeChangeStation:
            // disable discovery mode if we aren't actually being discoverable
            // this isn't really perfect since really we should just alert the user
            // that it isn't working, but hell that's hard to do right
            ui.actionToggleDiscoveryMode->setEnabled( static_cast<ChangeStationRequest*>(r)->discoverable()
                                                      && The::user().isSubscriber() );
        break;

        case TypeUnBan:
            statusBar()->showMessage( tr( "You unbanned this track." ) );
        break;

        case TypeAddToMyPlaylist:
            statusBar()->showMessage( tr( "This track has been added to your playlist." ) );
        break;

        default:
        break;
    }
}


void
Container::webServiceFailure( Request *r )
{
    switch ( r->type() )
    {
        case TypeSkip:
            LastMessageBox::information( tr( "Sorry" ), tr( "We couldn't skip this track." ) );
            break;

        case TypeSetTag:
            statusBar()->showMessage( tr( "Tagging %1 failed" ).arg( static_cast<SetTagRequest*>(r)->title() ) );
            break;

        default:
            break;
    }
}


void
Container::onAltShiftL()
{
    Logger* thelog = Logger::the();
    if (!thelog) return;

    #ifdef WIN32
        // The QDesktopServices call doesn't work on Windows
        QString file = QString::fromStdWString( thelog->GetFilePath() );
        ShellExecuteW( 0, 0, (TCHAR*)file.utf16(), 0, 0, SW_SHOWNORMAL );
    #else
        QDesktopServices::openUrl( QUrl::fromLocalFile( QString::fromStdString( thelog->GetFilePath() ) ) );
    #endif
}


void
Container::onAltShiftF()
{
    #ifdef WIN32
        // The QDesktopServices call doesn't work on Windows
        QString file = MooseUtils::logPath( "" );
        ShellExecuteW( 0, 0, (TCHAR*)file.utf16(), 0, 0, SW_SHOWNORMAL );
    #else
        QDesktopServices::openUrl( QUrl::fromLocalFile( MooseUtils::logPath( "" ) ) );
    #endif
}


#ifdef WIN32
void
Container::onAltShiftP()
{
    ShellExecuteW( 0, 0, (TCHAR*)UnicornUtils::globalAppDataPath().utf16(), 0, 0, SW_SHOWNORMAL );
}
#endif


void
Container::onAppEvent( int event, const QVariant& data )
{
    //Do not respond to any events if there is no user logged in
    if( !&The::user() )
        return;
        
    switch ( event )
    {
        case Event::UserChanged:
        {
            // we must restore state here as we save it in toggleSidebar in order to get
            // round the bug in Qt where saveState for the splitter is lost for hidden widgets
            m_sidebarEnabled = !The::user().settings().sidebarEnabled();
            toggleSidebar();

            ui.restStateWidget->clear();
            ui.restStateWidget->updatePlayerNames();

            // this call is redundant. Settings's userSettingsChanged will be emitted when switching the user!
//             updateUserStuff( The::user().settings() );
            #ifndef HIDE_RADIO
            statusBar()->showMessage( tr( "Contacting radio service..." ) );
            #endif
        }
        break;

        case Event::UserHandshaken:
        {
            ui.actionToggleDiscoveryMode->setEnabled( The::user().isSubscriber() );
            ui.restStateWidget->setPlayEnabled( true );

            #ifndef HIDE_RADIO
            statusBar()->showMessage( tr( "Radio service initialised" ) );
            #endif
            
            #ifndef Q_WS_X11
            if ( !The::settings().isFirstRun() )
                // Don't do this until here as we won't have received a base host earlier.
                checkForUpdates( false );
            #endif
        }
        break;

        case Event::TuningIn:
        {
            statusBar()->showMessage( tr( "Starting station %1..." ).arg( The::radio().stationUrl() ) );

            showMetaDataWidget();
            ui.metaDataWidget->displayTuningIn();

            ui.restStateWidget->clear();

            if ( The::radio().stationUrl().isPlaylist() )
                ui.stationTimeBar->setText( tr( "Connecting to playlist..." ) );
            else
                ui.stationTimeBar->setText( tr( "Starting station..." ) );

            ui.songTimeBar->clear();
            ui.stationTimeBar->setClockText( "" );
            ui.stationTimeBar->setEnabled( true );
            ui.stationTimeBar->setVisible( true );
        }
        break;

        case Event::PlaybackStarted:
        {
            TrackInfo track = data.value<TrackInfo>();

            if ( track.source() != TrackInfo::Radio )
            {
                // with radio we already changed to this, and there is a noticeable
                // gap between tuning in and starting playback so the user may have
                // already switched back to the change station tab
                showMetaDataWidget();
            }
        }
        // continue!!

        case Event::TrackChanged:
        {
            TrackInfo metadata = data.value<TrackInfo>();

            LOGL( 4, "Event::TrackChanged " << metadata.toString() << " " << metadata.durationString() );
            ui.songTimeBar->setTrack( metadata );
            m_trayIcon->setTrack( metadata );

            if ( MooseUtils::scrobblableStatus( metadata ) != MooseEnums::OkToScrobble &&
                 metadata.source() != TrackInfo::Radio )
            {
                ui.actionTag->setEnabled( false );
                ui.actionShare->setEnabled( false );
                ui.actionLove->setEnabled( false );
            }
            else
            {
                // The reason for these extra checks is that one user had
                // crashes here (bug #56). This could happen if the player listener
                // receives a START, emits a queued signal, then immediately
                // receives a STOP. If the first queued signal doesn't get here
                // until after the STOP, GetActivePlayer will return NULL.
                CPlayerConnection* plyr = The::app().listener().GetActivePlayer();
                if ( !plyr )
                {
                    LOG( 1, "Caught a NULL player in setNewSong, not setting stopwatch" );
                }
                else
                {
                    StopWatch& watch = plyr->GetStopWatch();
                    ui.songTimeBar->setStopWatch( &watch );
                    ui.songTimeBar->setReverse( true );

                    if ( plyr->IsScrobbled() )
                        ui.songTimeBar->pushClockText( tr( "scrobbled" ), 5 );
                    else
                        ui.songTimeBar->setClockText( "" );
                }

                ui.actionShare->setEnabled( true );
                ui.actionTag->setEnabled( true );
                ui.actionPlaylist->setEnabled( true );
                ui.actionLove->setEnabled( true );
                ui.songTimeBar->setEnabled( true );

                // Clear any status messages on the start of a new track or it will look odd
                if ( metadata.source() != TrackInfo::Radio )
                {
                    statusBar()->clearMessage();
                }
            }

            ui.metaDataWidget->setTrackInfo( metadata );
            updateWindowTitle( metadata );
        }
        break;

        case Event::TrackMetaDataAvailable:
        {
            MetaData metadata = data.value<MetaData>();

            updateWindowTitle( metadata );
            ui.metaDataWidget->setTrackMetaData( metadata );

            LOGL( 4, "Event::TrackMetaDataAvailable " << metadata.toString() << " " << metadata.durationString() );
            ui.songTimeBar->setTrack( metadata );
        }
        break;

        case Event::ArtistMetaDataAvailable:
        {
            ui.metaDataWidget->setArtistMetaData( data.value<MetaData>() );
        }
        break;

        case Event::PlaybackEnded:
        {
            // we disable this sometimes from ChangeStationRequest results
            // but enable it again when we stop
            ui.actionToggleDiscoveryMode->setEnabled( The::user().isSubscriber() );

            statusBar()->clearMessage();
            showRestState();

            ui.actionShare->setEnabled( false );
            ui.actionTag->setEnabled( false  );
            ui.actionPlaylist->setEnabled( false );
            ui.actionLove->setEnabled( false );

            ui.songTimeBar->setEnabled( false );
            ui.songTimeBar->setClockText( "" );
            ui.songTimeBar->setClockEnabled( false );
            ui.songTimeBar->setText( "" );

            updateWindowTitle( MetaData() );
            m_trayIcon->setTrack( TrackInfo() );
        }
        break;

        case Event::ScrobblePointReached:
        {
            TrackInfo const track = data.value<TrackInfo>();

            ui.songTimeBar->pushClockText( tr( "scrobbled" ), 5 );

            Track t;
            t.setArtist( track.artist() );
            t.setTitle( track.track() );

            ui.sidebar->addRecentlyPlayedTrack( t );
        }
        break;

        case Event::MediaDeviceTrackScrobbled:
        {
            TrackInfo const track = data.value<TrackInfo>();

            Track t;
            t.setArtist( track.artist() );
            t.setTitle( track.track() );

            ui.sidebar->addRecentlyPlayedTrack( t );
        }
        break;

        case Event::PlaybackPaused:
        {
            qDebug() << "Paused";
        }
        break;

        case Event::PlaybackUnpaused:
        {
            qDebug() << "Unpaused";
        }
        break;
    }
}


void
Container::restoreWindow()
{
    showNormal();
    activateWindow();
    raise();
}


/** here we control the radio specific ui components */
void
Container::onRadioStateChanged( RadioState newState )
{
    int const state = (int)newState; //use an int so gcc doesn't give us a warning

#ifndef HIDE_RADIO
    switch ( state )
    {
        case State_Stopped:
        {
            removeDMCAWarnings();
            ui.stationTimeBar->clear();
            ui.stationTimeBar->setEnabled( false );
            ui.stationTimeBar->hide();
        }
        break;

        case State_FetchingStream:
        {
            // We now have a station name.
            ui.stationTimeBar->setText( tr( "Station: %1" ).arg( The::radio().stationName() ) );
            ui.stationTimeBar->setEnabled( true );
            ui.stationTimeBar->show();
            statusBar()->showMessage( tr( "Retrieving stream..." ) );
        }
        break;

        case State_FetchingPlaylist:
        {
            ui.metaDataWidget->displayTuningIn();
            statusBar()->showMessage( tr( "Retrieving playlist..." ) );
        }
        break;

        case State_ChangingStation:
        {
            if( !The::radio().stationUrl().isDMCACompatible() && //show message when station is non-DMCA compatible
                ui.stack->findChildren<RestStateMessage*>( "DMCA" ).isEmpty() ) //Don't show > 1 message
            {
                RestStateMessage* dmcaMsg = new RestStateMessage( ui.stack );
                dmcaMsg->setObjectName( "DMCA" );
                dmcaMsg->setFaqVisible( false );
                dmcaMsg->setMessage( tr( "This station will soon be discontinued due to changes coming to Last.fm radio." ) +  " <a href='http://www.last.fm/stationchanges2010'>" +
                                 tr( "Find out more" ) + "</a>." );
                dmcaMsg->show();
            } else if( The::radio().stationUrl().isDMCACompatible()) {
                removeDMCAWarnings();
            }

        }
    }

    /** skip/ban */
    switch ( state )
    {
        case State_Streaming:
        {
            ui.actionSkip->setEnabled( true );
            ui.actionBan->setEnabled( true );
        }
        break;

        default:
        {
            ui.actionSkip->setEnabled( false );
            ui.actionBan->setEnabled( false );
        }
        break;
    }

    // disable actions during certain transitionary, etc. states
    switch ( state )
    {
        case State_Uninitialised:
        case State_Skipping:
        case State_Stopping:
        case State_Handshaking:
        {
            ui.actionPlay->setEnabled( false );
            ui.actionStop->setEnabled( false );
        }
        break;

        default:
        {
            ui.actionPlay->setEnabled( true );
            ui.actionStop->setEnabled( true );
        }
        break;
    }

    // set the play/stop actions to the correct states
    switch ( state )
    {
        case State_Uninitialised:
        case State_Handshaking:
        case State_Handshaken:
        case State_Stopping:
        case State_Stopped:
        {
            ui.actionPlay->setVisible( true );
            ui.actionPlay->setShortcut( Qt::Key_Space );
            ui.actionStop->setVisible( false );
            ui.actionStop->setShortcut( QKeySequence() );
        }
        break;

        default:
        {
            ui.actionPlay->setVisible( false );
            ui.actionPlay->setShortcut( QKeySequence() );
            ui.actionStop->setVisible( true );
            ui.actionStop->setShortcut( Qt::Key_Space );
        }
        break;
    }

  #endif // HIDE_RADIO
}


std::vector<CPluginInfo>&
Container::getPluginList()
{ 
    return m_updater->getPluginList();
}


void 
Container::removeDMCAWarnings()
{
    QList<RestStateMessage*> currentMessages = ui.stack->findChildren<RestStateMessage*>( "DMCA" );
    foreach( RestStateMessage* dmcaMsg, currentMessages ) {
        dmcaMsg->hide();
        dmcaMsg->deleteLater();
    }

}


///////////////////////////////////////////////////////////////////////////////>
ScrobbleLabel::ScrobbleLabel()
{
    QHBoxLayout *hbox = new QHBoxLayout( this );
    hbox->setMargin( 0 );
    hbox->setSpacing( 6 );

    hbox->addWidget( m_label = new QLabel );
  #ifdef Q_WS_MAC
    QLabel* l;
    hbox->addWidget( l = new QLabel );
    l->setPixmap( QPixmap( MooseUtils::savePath( "icons/scrobbling_graphic.png" ) ) );
  #endif
    hbox->addWidget( m_image = new QLabel );

    setEnabled( false );
    setToolTip( tr( "Click to enable/disable scrobbling" ) );
    setAutoFillBackground( false );
}


void
ScrobbleLabel::setEnabled( bool const on )
{
    QIcon icon( MooseUtils::dataPath( "icons/scrobble16.png" ) );

    m_label->setText( ' ' + tr( "Scrobbling %1" ).arg( on ? tr( "on" ) : tr( "off" ) ) + ' ' );
    m_image->setPixmap( icon.pixmap( 16, 16, on ? QIcon::Normal : QIcon::Disabled ) );
}


///////////////////////////////////////////////////////////////////////////////>

namespace The
{
    ShareDialog& shareDialog()
    {
        return The::container().shareDialog();
    }
}
