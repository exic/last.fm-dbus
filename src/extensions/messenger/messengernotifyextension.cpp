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

#include "windows.h"

#include <QtGui>

#include "messengernotifyextension.h"
#include "logger.h"
#include "lastfmapplication.h"

#define MESSENGER_SEND_URL (0)

static const QString kSetMoodMsgCmd = "SET PROFILE MOOD_TEXT ";
static const QString kGetMoodMsgCmd = "GET PROFILE MOOD_TEXT";

// Note about saving of mood messages
//
// We're trying best we can to store the user's previous mood message before
// we start sending notifications through so that we can restore it when
// the user disables our plugin. This involves reading it at startup and
// also watching for manual modifications of it while we're active.
// There is however one sequence of events where a user's mood message might
// get lost but it's an edge case and it would require more effort than
// it's worth to fix. It goes a little something like:
//     Enable plugin in Options
//     Quit app
//     Change mood msg in Skype
//     Launch Last.fm and disable plugin straight away
//     Whatever old mood message we had stored would be restored

MessengerNotifyExtension::MessengerNotifyExtension() :
    m_parent( NULL ),
    m_settingsPanel( NULL ),
    m_enabledByDefault( true ),
    m_config( 0 )
{
    LOG( 3, "Initialising Messenger Extension\n" );

    connect( qApp, SIGNAL(event( int, QVariant )), SLOT(onAppEvent( int, QVariant )) );
}


QString
MessengerNotifyExtension::name() const
{
    return QString( "Messenger Notifier Extension" );
}


QString
MessengerNotifyExtension::version() const
{
    return QString( "1.0.0.0" );
}

QWidget*
MessengerNotifyExtension::settingsPane()
{

    initSettingsPanel();

    return m_settingsPanel;
}

void
MessengerNotifyExtension::initSettingsPanel()
{
    LOG( 3, "Initialising Messenger GUI\n" );

    m_settingsPanel = new QWidget( m_parent );
    ui.setupUi( m_settingsPanel );

    // Loaded from resources to avoid having to ship a separate file
    m_icon.load( ":/options_messenger.png" );

    connect( ui.enabledCheck,     SIGNAL( stateChanged( int ) ),
             this,                SIGNAL( settingsChanged() ) );
}

QPixmap*
MessengerNotifyExtension::settingsIcon()
{
    Q_ASSERT( !m_icon.isNull() );

    return &m_icon;
}

void
MessengerNotifyExtension::populateSettings()
{
    // This won't get called until the options pane is displayed for 
    // the first time.

    Q_ASSERT( m_config != 0 );

    m_config->beginGroup( name() );

    ui.enabledCheck->setChecked(
        m_config->value( "Enabled", m_enabledByDefault ).toBool() );
    m_config->endGroup();
}

void
MessengerNotifyExtension::saveSettings()
{
    Q_ASSERT( m_config != 0 );

    m_config->beginGroup( name() );

    bool switchedOff =
        !ui.enabledCheck->isChecked() &&
        m_config->value( "Enabled", m_enabledByDefault ).toBool();
    bool switchedOn =
        ui.enabledCheck->isChecked() &&
        !m_config->value( "Enabled", m_enabledByDefault ).toBool();

    m_config->setValue( "Enabled", ui.enabledCheck->isChecked() );

    if ( switchedOff )
    {
        LOGL( 3, "Disabled Messenger plugin, restoring old mood message" );

		MetaData metaData;
		sendToMessenger( metaData, false );
    }

    // Must call endGroup before we do the notify below, otherwise the newly
    // enabled state won't be persisted in time.
    m_config->endGroup();

    if ( switchedOn )
    {
        LOGL( 3, "Enabled Messenger plugin, sending current track");

        sendToMessenger( m_cachedMetadata, true );
    }
}


void
MessengerNotifyExtension::onAppEvent( int e, const QVariant& data )
{    
    TrackInfo metaData;
    
    switch (e)
    {
        case Event::PlaybackStarted:
        case Event::TrackChanged:
            metaData = data.value<TrackInfo>();
            break;
            
        case Event::PlaybackEnded:
            break;
        
        default:
            return;
    }

	m_cachedMetadata = metaData;

    if ( !isEnabled() )
    {
        return;
    }

    sendToMessenger( metaData, true );

    LOGL( 4, "Sent metadata to Messenger" );
}


bool
MessengerNotifyExtension::isEnabled()
{
    Q_ASSERT( m_config != 0 );

    m_config->beginGroup( name() );
    bool enabled = m_config->value( "Enabled", m_enabledByDefault ).toBool();
    m_config->endGroup();
    
    return enabled;
}

#define MSNMusicString L"\\0Music\\01\\0{0} - {1}\\0%s\\0%s\\0%s\\0\\0"
#define MSNEmptyString L"\\0Music\\00\\0{0} - {1}\\0\\0\\0\\0\\0"

void
MessengerNotifyExtension::sendToMessenger( MetaData metaData, bool enable )
{
    QString cmd; 

	COPYDATASTRUCT msndata;
	//WCHAR buffer[500];
	
    // Syntax:
    // "\0Music\0" & Abs(r_bShow) & "\0"
    //             & r_sFormat & "\0"
    //             & r_sArtist & "\0"
    //             & r_sTitle & "\0"
    //             & r_sAlbum & "\0"
    //             & r_sWMContentID & "\0"
    //             & vbNullChar

	if (enable && !metaData.track().isEmpty())
        cmd = "\\0Music\\01\\0{0} - {1}\\0%1\\0%2\\0%3\\0\\0";
	else
        cmd = "\\0Music\\01\\0\\0\\0\\0\\0\\0";

    cmd = cmd.replace( QString("%1"), metaData.artist() )
             .replace( QString("%2"), metaData.track() )
             .replace( QString("%3"), metaData.album() );

	msndata.dwData = 0x547;
	msndata.lpData = (PVOID)cmd.unicode();
	msndata.cbData = (cmd.size() * 2) + 2; // QChars are 2 bytes, cbData is size in bytes

	HWND msnui = NULL;
	while (msnui = FindWindowEx(NULL, msnui, L"MsnMsgrUIManager", NULL))
	{
		SendMessage(msnui, WM_COPYDATA, NULL, (LPARAM)&msndata);
	}
}

Q_EXPORT_PLUGIN2( extension_notify_messenger, MessengerNotifyExtension )
