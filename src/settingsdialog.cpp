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

#ifdef WIN32
    #include <windows.h>
#endif

#include "MooseCommon.h"
#include "UnicornCommon.h"
#include "logger.h"

#include <QHeaderView>
#include <QProcess>
#include <QPluginLoader>
#include <QLibrary>

#include "Radio.h"
#include "LastFmSettings.h"
#include "settingsdialog.h"
#include "lastfmapplication.h"
#include "container.h"
#include "LastMessageBox.h"
#include "loginwidget.h"

// This is a terrible way of entering strings but I couldn't get VS to recognise
// the Unicode chars in any other way.
static const unsigned char kChinese[] =
    { 0xE4, 0xB8, 0xAD, 0xE6, 0x96, 0x87, 0x0 };
static const unsigned char kRussian[] =
    { 0xD0, 0xA0, 0xD1, 0x83, 0xD1, 0x81, 0xD1, 0x81, 0xD0, 0xBA,
      0xD0, 0xB8, 0xD0, 0xB9, 0x0 };
static const unsigned char kJapanese[] =
    { 0xE6, 0x97, 0xA5, 0xE6, 0x9C, 0xAC, 0xE8, 0xAA, 0x9E, 0x0 };


SettingsDialog::SettingsDialog( QWidget *parent )
        : QDialog( parent ),
          m_reconnect( false ),
          m_reAudio( false ),
          m_populating( false ),
          m_closeAfterVerification( false )
{
    ui.setupUi( this );

    #ifdef Q_WS_MAC
        setWindowTitle( tr( "Last.fm Preferences" ) );
    #else
        setWindowTitle( tr( "Last.fm Options" ) );
    #endif

    // Create pages and add them to stack widget
    QWidget* accountWidget = new QWidget();
    ui_account.setupUi( accountWidget );
    ui.pageStack->addWidget( accountWidget );

    m_loginWidget = new LoginWidget( this, LoginWidget::CHANGE_PASS, The::settings().currentUsername() );
    ui_account.accountBox->layout()->addWidget( m_loginWidget );

    QWidget* radioWidget = new QWidget();
    ui_radio.setupUi( radioWidget );
    ui.pageStack->addWidget( radioWidget );

    QWidget* scrobWidget = new QWidget();
    ui_scrobbling.setupUi( scrobWidget );
    ui.pageStack->addWidget( scrobWidget );

    QWidget* connWidget = new QWidget();
    ui_connection.setupUi( connWidget );
    ui.pageStack->addWidget( connWidget );

#ifndef Q_WS_X11
    QWidget* mediadeviceWidget = new QWidget();
    ui_mediadevices.setupUi( mediadeviceWidget );
    ui_mediadevices.deviceWidget->header()->setResizeMode( QHeaderView::ResizeToContents );
    ui.pageStack->addWidget( mediadeviceWidget );
#endif

#ifdef NBREAKPAD
    ui_connection.crashReportCheck->hide();
#endif

    // Add icons to user icon dropdown
    QPixmap pixmap( MooseUtils::dataPath( "icons/user_red.png" ) );
    ui_account.colourCombo->setItemIcon( 0, pixmap );
    pixmap.load( MooseUtils::dataPath( "icons/user_blue.png" ) );
    ui_account.colourCombo->setItemIcon( 1, pixmap );
    pixmap.load( MooseUtils::dataPath( "icons/user_green.png" ) );
    ui_account.colourCombo->setItemIcon( 2, pixmap );
    pixmap.load( MooseUtils::dataPath( "icons/user_orange.png" ) );
    ui_account.colourCombo->setItemIcon( 3, pixmap );
    pixmap.load( MooseUtils::dataPath( "icons/user_black.png" ) );
    ui_account.colourCombo->setItemIcon( 4, pixmap );

    // Add languages to language drop-down
    ui_account.languageCombo->addItem( tr( "System Language" ), "" );
    ui_account.languageCombo->addItem( "English",
        UnicornUtils::qtLanguageToLfmLangCode( QLocale::English ) );
    ui_account.languageCombo->addItem( QString( "Fran" ) + QChar( 0xe7 ) + QString( "ais" ),
        UnicornUtils::qtLanguageToLfmLangCode( QLocale::French ) );
    ui_account.languageCombo->addItem( "Italiano",
        UnicornUtils::qtLanguageToLfmLangCode( QLocale::Italian ) );
    ui_account.languageCombo->addItem( "Deutsch",
        UnicornUtils::qtLanguageToLfmLangCode( QLocale::German ) );
    ui_account.languageCombo->addItem( QString( "Espa" ) + QChar( 0xf1 ) + QString( "ol" ),
        UnicornUtils::qtLanguageToLfmLangCode( QLocale::Spanish ) );
    ui_account.languageCombo->addItem( QString( "Portugu" ) + QChar( 0xea ) + QString( "s" ),
        UnicornUtils::qtLanguageToLfmLangCode( QLocale::Portuguese ) );
    ui_account.languageCombo->addItem( "Polski",
        UnicornUtils::qtLanguageToLfmLangCode( QLocale::Polish ) );
    ui_account.languageCombo->addItem( "Svenska",
        UnicornUtils::qtLanguageToLfmLangCode( QLocale::Swedish ) );
    ui_account.languageCombo->addItem( QString::fromUtf8( "Türkçe" ),
        UnicornUtils::qtLanguageToLfmLangCode( QLocale::Turkish ) );
    ui_account.languageCombo->addItem( QString::fromUtf8( (const char*) kRussian ),
        UnicornUtils::qtLanguageToLfmLangCode( QLocale::Russian ) );
    ui_account.languageCombo->addItem( QString::fromUtf8( (const char*) kChinese ),
        UnicornUtils::qtLanguageToLfmLangCode( QLocale::Chinese ) );
    ui_account.languageCombo->addItem( QString::fromUtf8( "日本語" ), 
        UnicornUtils::qtLanguageToLfmLangCode( QLocale::Japanese ) );

    // Add icons to sidebar
    pixmap.load( MooseUtils::dataPath( "/icons/options_account.png" ) );
    //pixmap.scaled( 48, 48, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    ui.pageList->item( 0 )->setIcon( pixmap );

    pixmap.load( MooseUtils::dataPath( "/icons/options_radio.png" ) );
    //pixmap.scaled( 48, 48, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    ui.pageList->item( 1 )->setIcon( pixmap );

    #ifdef HIDE_RADIO
        ui.pageList->setItemHidden( ui.pageList->item( 1 ), true );
    #endif // HIDE_RADIO

    pixmap.load( MooseUtils::dataPath( "/icons/options_scrobbling.png" ) );
    //pixmap.scaled( 48, 48, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    ui.pageList->item( 2 )->setIcon( pixmap );

    pixmap.load( MooseUtils::dataPath( "/icons/options_connection.png" ) );
    //pixmap.scaled( 48, 48, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    ui.pageList->item( 3 )->setIcon( pixmap );

    pixmap.load( MooseUtils::dataPath( "/icons/options_mediadevices.png" ) );
    //pixmap.scaled( 48, 48, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    ui.pageList->item( 4 )->setIcon( pixmap );

    #ifdef Q_WS_X11
    ui.pageList->setRowHidden( 4, true );
    #endif

    #ifdef HIDE_RADIO
        // Need to disable this as the language choice is hardcoded at startup
        // (see LastfmApp::initTranslator) for the jp version.
        ui_account.languageCombo->hide();
        ui_account.languageLabel->hide();
    #endif

    #ifdef Q_WS_MAC
        ui_account.colourCombo->hide();
        ui_account.colourLabel->hide();
    #else
        ui_account.showInDockCheck->hide();
    #endif

    ui_scrobbling.scrobblePointSlider->setMinimum( MooseConstants::kScrobblePointMin );
    ui_scrobbling.scrobblePointSlider->setMaximum( MooseConstants::kScrobblePointMax );

    // Fix width of scrobble point label so it doesn't make the slider jump about
    int maxWidth = ui_scrobbling.scrobblePointLabel->fontMetrics().width( "100" );
    ui_scrobbling.scrobblePointLabel->setFixedWidth( maxWidth );

    #ifdef Q_WS_MAC
        ui_scrobbling.launchWithMediaPlayerCheck->setText( tr( "Launch Last.fm with iTunes" ) );
    #else
        ui_scrobbling.launchWithMediaPlayerCheck->hide();
    #endif

    adjustSize();

    connect( ui.buttonBox, SIGNAL( accepted() ), SLOT( onOkClicked() ) );
    connect( ui.buttonBox, SIGNAL( rejected() ), SLOT( reject() ) );
    connect( ui.buttonBox->button( QDialogButtonBox::Apply ), SIGNAL( clicked() ), SLOT( applyPressed() ) );

    connect( ui.pageList, SIGNAL( currentRowChanged( int ) ), SLOT( pageSwitched( int ) ) );
    connect( ui_connection.clearCacheButton, SIGNAL( clicked() ), SLOT( clearCache() ) );

    connect( m_loginWidget, SIGNAL( verifyResult( bool, bool ) ), SLOT( verifiedAccount( bool, bool ) ) );
    connect( m_loginWidget, SIGNAL( widgetChanged() ), this, SLOT( configChanged() ) );
    connect( ui_account.colourCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( configChanged() ) );
    connect( ui_account.languageCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( configChanged() ) );
    connect( ui_account.showInTrayCheck, SIGNAL( toggled( bool ) ), this, SLOT( configChanged() ) );
    connect( ui_radio.resumeCheckBox, SIGNAL( toggled( bool ) ), this, SLOT( configChanged() ) );
    connect( ui_radio.cardBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( configChanged() ) );
    connect( ui_radio.systemBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( configChanged() ) );
    connect( ui_radio.automaticBufferCheck, SIGNAL( toggled( bool ) ), this, SLOT( configChanged() ) );
    connect( ui_radio.bufferEdit, SIGNAL( textChanged( QString ) ), this, SLOT( configChanged() ) );
    connect( ui_radio.musicProxyPort, SIGNAL( valueChanged( int ) ), this, SLOT( configChanged() ) );
    connect( ui_scrobbling.dirTree, SIGNAL( dataChanged() ), this, SLOT( configChanged() ) );
    connect( ui_scrobbling.scrobblePointSlider, SIGNAL( valueChanged( int ) ), this, SLOT( configChanged() ) );
    connect( ui_scrobbling.launchWithMediaPlayerCheck, SIGNAL( toggled( bool ) ), this, SLOT( configChanged() ) );
    connect( ui_scrobbling.fingerprintCheckBox, SIGNAL( stateChanged( int ) ), this, SLOT( configChanged() ) );
    connect( ui_connection.proxyBox, SIGNAL( toggled( bool ) ), this, SLOT( configChanged() ) );
    connect( ui_connection.proxyHostEdit, SIGNAL( textChanged( QString ) ), this, SLOT( configChanged() ) );
    connect( ui_connection.proxyPortEdit, SIGNAL( textChanged( QString ) ), this, SLOT( configChanged() ) );
    connect( ui_connection.proxyUsernameEdit, SIGNAL( textChanged( QString ) ), this, SLOT( configChanged() ) );
    connect( ui_connection.proxyPasswordEdit, SIGNAL( textChanged( QString ) ), this, SLOT( configChanged() ) );
    connect( ui_connection.automaticProxyButton, SIGNAL( toggled( bool ) ), this, SLOT( configChanged() ) );
    connect( ui_connection.manualProxyButton, SIGNAL( toggled( bool ) ), this, SLOT( configChanged() ) );
    connect( ui_connection.downloadMetadataCheck, SIGNAL( toggled( bool ) ), this, SLOT( configChanged() ) );
#ifndef NBREAKPAD
    connect( ui_connection.crashReportCheck, SIGNAL( toggled( bool ) ), this, SLOT( configChanged() ) );
#endif

    #ifndef Q_WS_X11
    connect( ui_mediadevices.enableIPodScrobbling, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );
    connect( ui_mediadevices.alwaysConfirmIPodScrobbles, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );
    connect( ui_mediadevices.giveMoreFeedback, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );
    connect( ui_mediadevices.clearUserAssociations, SIGNAL( clicked() ), SLOT( clearUserIPodAssociations() ) );
    #endif

    #ifdef Q_WS_MAC
        connect( ui_scrobbling.launchWithMediaPlayerCheck, SIGNAL( toggled( bool ) ), this, SLOT( configChanged() ) );
        connect( ui_account.showInDockCheck, SIGNAL( toggled( bool ) ), this, SLOT( configChanged() ) );
    #endif
}


int
SettingsDialog::exec( int startPage )
{
    m_reconnect = false;
    m_reAudio = false;

    m_loginWidget->resetWidget( The::settings().currentUsername() );

    originalUsername = The::settings().currentUser().username();
    originalPassword = The::settings().currentUser().password();
    originalProxyHost = The::settings().getProxyHost();
    originalProxyUsername = The::settings().getProxyUser();
    originalProxyPassword = The::settings().getProxyPassword();
    originalProxyPort = The::settings().getProxyPort();
    originalProxyUsage = The::settings().isUseProxy();
    originalSoundCard = The::settings().soundCard();
    originalSoundSystem = The::settings().soundSystem();

    // This bool is just to prevent signals emitted by the programmatic
    // populating of widgets from being mistakenly interpreted as user
    // interaction.
    m_populating = true;
    populateAccount();
    populateRadio();
    populateScrobbling();
    populateConnection();

    #ifndef Q_WS_X11
    populateMediaDevices();
    #endif

    loadExtensions();
    foreach ( ExtensionInterface* i, m_extensions )
        i->populateSettings();
    m_populating = false;

    pageSwitched( startPage );
    ui.buttonBox->button( QDialogButtonBox::Apply )->setEnabled( false );

    return QDialog::exec();
}


void
SettingsDialog::loadExtensions()
{
    foreach( QString path, Moose::extensionPaths() )
    {
        // NOTE we should test isLoaded() first really, but for some reason
        // this wasn't working on Windows with Qt 4.3.1

	    QObject* plugin = QPluginLoader( path ).instance();
	    if ( !plugin )
	    {
    		LOGL( 2, "Failed to retrieve instance from loaded plugin: " << path );
	    	continue;
	    }

	    ExtensionInterface* iExtension = qobject_cast<ExtensionInterface *>( plugin );
	    if ( iExtension && iExtension->hasSettingsPane() )
	    {
	        LOGL( 3, "Loaded a settings extension from: " << path );
	        addExtension( iExtension ); //adds page to dialog
	    }
    }
}


#ifdef Q_WS_MAC
static bool appHasDockIcon()
{
    QProcess p;
    p.start( "defaults", QStringList() << "read" << Moose::bundleDirPath() + "/Contents/Info" << "LSUIElement" );
    p.waitForFinished();
    
    if (p.exitCode() == 0)
        return p.readAllStandardOutput().trimmed() != "1";
    else
        // nonzero exit code means no LSUIElement, thus shown
        return true;
}
#endif


void
SettingsDialog::populateAccount()
{
    LastFmUserSettings* user = &The::settings().currentUser();

    ui_account.colourCombo->setCurrentIndex( user->icon() < ui_account.colourCombo->count() ? user->icon() : 0 );

    #ifdef Q_WS_MAC
        ui_account.showInTrayCheck->setText( tr( "Show application icon in menu bar" ) );

        ui_account.showInDockCheck->setChecked( appHasDockIcon() );
    #endif

    ui_account.showInTrayCheck->setChecked( The::settings().showTrayIcon() );


    #ifndef HIDE_RADIO
        QString langCode = The::settings().customAppLanguage();
        for( int i = 0; i < ui_account.languageCombo->count(); ++i )
        {
            QString code = ui_account.languageCombo->itemData( i ).toString();
            if ( code == langCode )
            {
                ui_account.languageCombo->setCurrentIndex( i );
            }
        }
    #endif // HIDE_RADIO
}


void
SettingsDialog::populateRadio()
{
    ui_radio.resumeCheckBox->setChecked( The::settings().currentUser().resumePlayback() );

    ui_radio.systemBox->clear();
    ui_radio.cardBox->clear();

    Radio& radio = The::radio();
    ui_radio.systemBox->addItems( radio.soundSystems() );
    ui_radio.cardBox->addItems( radio.devices() );

    ui_radio.systemBox->setCurrentIndex( The::settings().soundSystem() >= 0 ? The::settings().soundSystem() : 0 );
    ui_radio.cardBox->setCurrentIndex( The::settings().soundCard() >= 0 ? The::settings().soundCard() : 0 );

    ui_radio.automaticBufferCheck->setChecked( The::settings().isBufferManagedAutomatically() );
    ui_radio.bufferEdit->setText( QString::number( The::settings().httpBufferSize() / 1024 ) );

    ui_radio.musicProxyPort->setValue( The::settings().musicProxyPort() );
}


void
SettingsDialog::populateScrobbling()
{
    LastFmUserSettings& user = The::settings().currentUser();

    ui_scrobbling.scrobblePointSlider->setValue( user.scrobblePoint() );
    ui_scrobbling.scrobblePointLabel->setText( QString::number( user.scrobblePoint() ) );
    ui_scrobbling.launchWithMediaPlayerCheck->setChecked( The::settings().launchWithMediaPlayer() );
    ui_scrobbling.fingerprintCheckBox->setChecked( user.fingerprintingEnabled() );
    ui_scrobbling.dirTree->setExclusions( user.excludedDirs() );
}


void
SettingsDialog::populateConnection()
{
    ui_connection.proxyBox->setChecked( The::settings().isUseProxy() );
    ui_connection.proxyHostEdit->setText( The::settings().getProxyHost() );
    ui_connection.proxyPortEdit->setText( QString::number( The::settings().getProxyPort() ) );
    ui_connection.proxyUsernameEdit->setText( The::settings().getProxyUser() );
    ui_connection.proxyPasswordEdit->setText( The::settings().getProxyPassword() );
    ui_connection.downloadMetadataCheck->setChecked( The::settings().currentUser().isMetaDataEnabled() );
#ifndef NBREAKPAD
    ui_connection.crashReportCheck->setChecked( The::settings().currentUser().crashReportingEnabled() );
#endif
    ui_connection.manualProxyButton->setChecked( The::settings().isUseProxy() );
}


void
SettingsDialog::populateMediaDevices()
{
    ui_mediadevices.enableIPodScrobbling->setChecked( The::settings().isIPodScrobblingEnabled() );
    ui_mediadevices.alwaysConfirmIPodScrobbles->setChecked( The::currentUser().isAlwaysConfirmIPodScrobbles() );
    ui_mediadevices.giveMoreFeedback->setChecked( The::currentUser().giveMoreIPodScrobblingFeedback() );

    QList<QTreeWidgetItem*> items;
    foreach( QString device, The::settings().allMediaDevices() )
    {
        QString username = The::settings().usernameForDeviceId( device );
        items += new QTreeWidgetItem( QStringList() << device << username );
    }

    ui_mediadevices.deviceWidget->clear();
    ui_mediadevices.deviceWidget->insertTopLevelItems( 0, items );
    ui_mediadevices.clearUserAssociations->setEnabled( items.count() );

    //NOTE manual scrobbling is disabled on Windows and Mac and this option
    // is irrelevent on Linux since manual is the only supported option
    ui_mediadevices.deviceWidget->hideColumn( 2 );
}


void
SettingsDialog::pageSwitched( int currentRow )
{
    ui.pageList->setCurrentRow( currentRow );
    ui.pageStack->setCurrentIndex( currentRow + 1 );
}


void
SettingsDialog::applyPressed()
{
    // Do hardcoded sections
    if ( m_pagesToSave.contains( 1 ) ) saveRadio();
    if ( m_pagesToSave.contains( 2 ) ) saveScrobbling();
    if ( m_pagesToSave.contains( 3 ) ) saveConnection();
    if ( m_pagesToSave.contains( 4 ) ) saveMediaDevices();

    // Do extensions
    foreach ( int page, m_pagesToSave )
    {
        if( page < 5)
            continue;

        int idx = page - 5;
        m_extensions[idx]->saveSettings();
        m_pagesToSave.remove( page );
    }

    // Account has to be the last section we handle here:
    // It relies on signals being received and in case
    // the user pressed Ok, this will close and free the
    // dialog asap.
    if ( m_pagesToSave.contains( 0 ) )
        saveAccount();

    ui.buttonBox->button( QDialogButtonBox::Apply )->setEnabled( false );
}


void
SettingsDialog::saveAccount()
{
    m_loginWidget->verify();
}


void
SettingsDialog::verifiedAccount( bool verified, bool /* bootstrap */ )
{
    if ( verified )
        // FIXME: why specify false? Better specify true and cause a rehandshake straight away.
        m_loginWidget->save( false );
    else 
        m_loginWidget->resetWidget( The::settings().currentUsername() );

    LastFmUserSettings& user = The::settings().currentUser();
    int iconIndex = ui_account.colourCombo->currentIndex() >= 0 ? ui_account.colourCombo->currentIndex() : 0;
    user.setIcon( static_cast<MooseEnums::UserIconColour>( iconIndex ) );

    The::settings().setShowTrayIcon( ui_account.showInTrayCheck->isChecked() );

#ifdef Q_WS_MAC
    if (appHasDockIcon() != ui_account.showInDockCheck->isChecked())
    {
        QProcess p;
        p.start( "defaults", QStringList() << "write"
                                           << Moose::bundleDirPath() + "/Contents/Info"
                                           << "LSUIElement"
                                           << (ui_account.showInDockCheck->isChecked() ? "0" : "1") );
        p.waitForFinished();
    
        p.start( "touch", QStringList() << Moose::bundleDirPath() ); //no need to wait
        
        LastMessageBox::say( tr("You need to restart the application for the dock icon appearance change to take effect.") );
    }
#endif

#ifndef HIDE_RADIO
    // Store language in config
    int langIdx = ui_account.languageCombo->currentIndex();
    QString langCode = ui_account.languageCombo->itemData( langIdx ).toString();
    QString oldLang = The::settings().customAppLanguage();

    if ( langCode != oldLang )
    {
        LastMessageBox dlg( QMessageBox::Information,
            tr( "Restart needed" ),
            tr( "You need to restart the application for the language "
            "change to take effect." ),
            QMessageBox::Ok, this );
        dlg.exec();
    }

    The::settings().setAppLanguage( langCode );
#endif

    if( !verified )
    {
        //verification failed so don't close the dialog after saving.
        return;
    }
    else
    {
        if ( m_loginWidget->detailsChanged() )
            m_reconnect = true;
    }

    pageSaved( 0 );

    if ( m_closeAfterVerification )
        accept();
}


void
SettingsDialog::saveRadio()
{
    The::settings().currentUser().setResumePlayback( ui_radio.resumeCheckBox->isChecked() );
    The::settings().setSoundCard( ui_radio.cardBox->currentIndex() >= 0 ? ui_radio.cardBox->currentIndex() : 0 );
    The::settings().setSoundSystem( ui_radio.systemBox->currentIndex() >= 0 ? ui_radio.systemBox->currentIndex() : 0 );

    The::settings().setBufferManagedAutomatically( ui_radio.automaticBufferCheck->isChecked() );

    // Returns 0 if the conversion fails so is safe
    int bufSize = ui_radio.bufferEdit->text().toInt() * 1024;
    if ( bufSize <= 0 ) bufSize = MooseConstants::kHttpBufferMinSize;
    The::settings().setHttpBufferSize( bufSize );

    The::settings().setMusicProxyPort( ui_radio.musicProxyPort->value() );

    m_reAudio = ui_radio.cardBox->currentIndex()   != originalSoundCard   ||
                ui_radio.systemBox->currentIndex() != originalSoundSystem ||
                m_reAudio;

    pageSaved( 1 );
}


void
SettingsDialog::saveScrobbling()
{
    LastFmUserSettings& user = The::settings().currentUser();
    user.setScrobblePoint( ui_scrobbling.scrobblePointSlider->value() );
    user.setExcludedDirs( ui_scrobbling.dirTree->getExclusions() );
    The::settings().setLaunchWithMediaPlayer( ui_scrobbling.launchWithMediaPlayerCheck->isChecked() );
    user.setFingerprintingEnabled( ui_scrobbling.fingerprintCheckBox->checkState() == Qt::Checked ? true : false );

    pageSaved( 2 );
}


void
SettingsDialog::saveConnection()
{
    The::settings().setProxyUser( ui_connection.proxyUsernameEdit->text() );
    The::settings().setProxyPassword( ui_connection.proxyPasswordEdit->text() );
    The::settings().setProxyHost( ui_connection.proxyHostEdit->text() );
    The::settings().setProxyPort( ui_connection.proxyPortEdit->text().toInt() );
    The::settings().setUseProxy( ui_connection.manualProxyButton->isChecked() );
    The::settings().currentUser().setMetaDataEnabled( ui_connection.downloadMetadataCheck->isChecked() );
#ifndef NBREAKPAD
    The::settings().currentUser().setCrashReportingEnabled( ui_connection.crashReportCheck->isChecked() );
#endif

    m_reconnect =  ui_connection.proxyHostEdit->text()              != originalProxyHost     ||
                   ui_connection.proxyUsernameEdit->text()          != originalProxyUsername ||
                   ui_connection.proxyPasswordEdit->text()          != originalProxyPassword ||
                   ui_connection.proxyPortEdit->text().toInt()      != originalProxyPort     ||
                   ui_connection.manualProxyButton->isChecked()     != originalProxyUsage    ||
                   m_reconnect;

    pageSaved( 3 );
}


// EJ: Umm, surely this ought to be taken care of by the plugin itself,
// and not coded straight into the Container? Like how the Skype
// plugin does it.
void
SettingsDialog::saveMediaDevices()
{
    bool changed = ui_mediadevices.enableIPodScrobbling->isChecked() != The::settings().isIPodScrobblingEnabled();
    if (changed)
    {
        if (ui_mediadevices.enableIPodScrobbling->isChecked())
        {
            LastMessageBox::say( tr("You must restart iTunes before Last.fm can scrobble your iPod" ) );
        }
        else
        {
        #ifdef Q_OS_MAC
            // we delete the plugin db when iTunes restarts as we can't delete
            // it now, on Windows at least it's in use, but it's safe to leave
            // it, ie, there can be no miss-scrobbles, and we ignore any 
            // twiddles here in the client from now on
        #endif
        }
    }

    The::settings().setIPodScrobblingEnabled( ui_mediadevices.enableIPodScrobbling->isChecked() );
    The::currentUser().setAlwaysConfirmIPodScrobbles( ui_mediadevices.alwaysConfirmIPodScrobbles->isChecked() );
    The::currentUser().setGiveMoreIPodScrobblingFeedback( ui_mediadevices.giveMoreFeedback->isChecked() );

    for (int x = 0; x < ui_mediadevices.deviceWidget->topLevelItemCount(); ++x)
    {
        QTreeWidgetItem* item = ui_mediadevices.deviceWidget->topLevelItem( x );
        QString const uid = item->text( 0 );
        The::settings().setIsManualIpod( uid, item->checkState( 2 ) == Qt::Checked );
    }

    pageSaved( 4 );
}


void
SettingsDialog::onOkClicked()
{
    m_closeAfterVerification = true;

    if( m_pagesToSave.size() > 0 )
        applyPressed();

    if ( !m_pagesToSave.contains( 0 ) )
        accept();
}


void
SettingsDialog::pageSaved(int page)
{
    m_pagesToSave.remove( page );

    if ( m_pagesToSave.size() == 0 )
    {
        if ( m_reconnect )
            The::app().setUser( The::currentUsername() );

        if ( m_reAudio )
        {
            LastMessageBox dlg( QMessageBox::Information,
                                tr( "Restart needed" ),
                                tr( "You need to restart the application for the audio device "
                                    "change to take effect." ),
                                QMessageBox::Ok, this );
            dlg.exec();
        }

        m_reconnect = m_reAudio = false;
    }
}


void
SettingsDialog::configChanged()
{
    if ( m_populating ) return;

    if( !m_pagesToSave.contains( ui.pageList->currentRow() ) && ui.pageList->currentRow() >= 0 )
          m_pagesToSave.insert( ui.pageList->currentRow() );

    ui.buttonBox->button( QDialogButtonBox::Apply )->setEnabled( true );
}


void
SettingsDialog::addExtension( ExtensionInterface* ext )
{
    m_extensions.append( ext );

    ext->setOwner( this );

    ui.pageStack->addWidget( ext->settingsPane() );
    ui.pageList->addItem( ext->tabCaption() );

    int idx = ui.pageList->count() - 1;
    ui.pageList->item( idx )->setIcon( *ext->settingsIcon() );

    // God knows why I have to give it the name again, but I do
    ui.pageList->item( idx )->setText( ext->tabCaption() );

    connect( ext,  SIGNAL( settingsChanged() ),
             this, SLOT( configChanged() ) );
}


void
SettingsDialog::clearCache()
{
    // Doesn't really belong here but...

    int sure = LastMessageBox::question(
        tr( "Confirm" ),
        tr( "Are you sure you want to delete all\ncached images and bios?" ),
        QMessageBox::Yes,
        QMessageBox::No,
        QStringList(), this );

    if ( sure == QMessageBox::Yes )
    {
        QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );

        QDir dir( MooseUtils::cachePath() );

        QStringList files = dir.entryList( QDir::Files );
        bool success = true;
        int count = 0;
        foreach( QString filename, files )
        {
            bool deleted = dir.remove( filename );
            if ( deleted ) { count++; }
            success &= deleted;

            if ( count % 25 == 0 )
            {
                QApplication::processEvents();
            }
        }

        QApplication::restoreOverrideCursor();

        QString result;
        if ( success )
        {
            result = tr( "Cache emptied. %n files deleted.", "", count );
        }
        else
        {
            result = tr( "Not all items could be deleted. You might need "
                "to delete some items manually from\n'%1'." )
                .arg( dir.absolutePath() );
        }

        LastMessageBox::information(
            tr( "Finished" ),
            result,
            QMessageBox::Ok,
            QMessageBox::NoButton,
            QStringList(), this );

        LOGL( 3, "Emptied cache, files: " << count << ", success: " << success );
    }
}


void
SettingsDialog::clearUserIPodAssociations()
{
    // useful no?
    #define LOG_IF_FAIL( b, x ) bool b = x; if (!b) LOGL( 3, "Failed" #x )

    MediaDeviceSettings().remove( "" );
    ui_mediadevices.deviceWidget->clear();
    ui_mediadevices.clearUserAssociations->setEnabled( false );

    // On Windows QFile::remove() may always fail due to file locking.
    // XP users can uninstall to delete the cache, but Vista users are screwed.
    // Indeed, it is safe to delete it when iTunes is running, Twiddly handles
    // it robustly and even shows an error message.
    LOG_IF_FAIL( b, QFile::remove( Moose::savePath( "iTunesPlays.db" ) ) );

    if (b) LastMessageBox::information(
                tr("Additional Action Required"),
                tr("You must restart iTunes before you next try to scrobble your iPod.") );
}

