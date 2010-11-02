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

#include <QtGui>

#include "lastfmapplication.h"
#include "skypenotifyextension.h"
#include "logger.h"

#define SKYPE_SEND_URL (0)

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

SkypeNotifyExtension::SkypeNotifyExtension()
        : m_parent( NULL ),
          m_settingsPanel( NULL ),
          m_enabledByDefault( false ),
          m_defaultFormat( tr( "Currently listening to '%2' by %1" ) ),
          m_haveWaiting( false ),
          m_config( 0 )
{
    LOG( 3, "Initialising Skype Extension\n" );

    connect( qApp, SIGNAL(event( int, QVariant )), SLOT(onAppEvent( int, QVariant )) );
}


QString
SkypeNotifyExtension::name() const
{
    return QString( "Skype Notifier Extension" );
}


QString
SkypeNotifyExtension::version() const
{
    return QString( "1.0.0.0" );
}

QWidget*
SkypeNotifyExtension::settingsPane()
{
    
    initSettingsPanel();

    return m_settingsPanel;
}

void
SkypeNotifyExtension::initSettingsPanel()
{
    LOG( 3, "Initialising Skype GUI\n" );

    m_settingsPanel = new QWidget( m_parent );
    m_icon.load( ":/options_skype.png" );
    ui.setupUi( m_settingsPanel );

    connect( ui.enabledCheck, SIGNAL(stateChanged( int )), SIGNAL(settingsChanged()) );
    connect( ui.nowPlayingFormat, SIGNAL(textChanged( QString )), SIGNAL(settingsChanged()) );
}

QPixmap*
SkypeNotifyExtension::settingsIcon()
{
    Q_ASSERT( !m_icon.isNull() );

    return &m_icon;
}

void
SkypeNotifyExtension::populateSettings()
{
    // This won't get called until the options pane is displayed for 
    // the first time.
    Q_ASSERT( m_config != 0 );

    m_config->beginGroup( name() );

    ui.enabledCheck->setChecked(
        m_config->value( "Enabled", m_enabledByDefault ).toBool() );
    ui.nowPlayingFormat->setText(
        m_config->value( "Format", m_defaultFormat ).toString() );
    m_config->endGroup();
}

void
SkypeNotifyExtension::saveSettings()
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
    m_config->setValue( "Format", ui.nowPlayingFormat->text() );

    if ( switchedOff )
    {
        QString oldMsg = m_config->value( "SavedMoodMessage", "" ).toString();
        QString fullCmd = kSetMoodMsgCmd + oldMsg;

        LOGL( 3, "Disabled Skype plugin, restoring old mood message: " << oldMsg );

        m_lastNotification = oldMsg;
        sendToSkype( fullCmd.toUtf8() );
    }

    // Must call endGroup before we do the notify below, otherwise the newly
    // enabled state won't be persisted in time.
    m_config->endGroup();

    if ( switchedOn )
    {
        LOGL( 3, "Enabled Skype plugin, sending current track");

        TrackInfo info = m_cachedMetadata;
        onAppEvent( Event::PlaybackStarted, QVariant::fromValue( info ) );
    }
}

void
SkypeNotifyExtension::onAppEvent( int e, const QVariant& data )
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

    // Always store this so that we can send it immediately if we get switched
    // from disabled to enabled during a track.
    m_cachedMetadata = metaData;

    if (!isEnabled())
        return;    

    if ( !isSkypeAttached() )
    { 
        // This is asynchronous so we flag that we have waiting metadata
        // for transmission once we're connected.
        initSkypeCommunication();
        m_haveWaiting = true;
        return;
    }

    QString cmd = kSetMoodMsgCmd;
    
    QByteArray utfToSend;
    if ( metaData.isEmpty() )
    {
        m_lastNotification = "\t";
        QString nowPlaying = cmd + m_lastNotification;
        utfToSend = nowPlaying.toUtf8();
    }
    else
    {
        QString format = nowPlayingFormat();
        
        m_lastNotification = format
            .replace( QString("%1"), metaData.artist() )
            .replace( QString("%2"), metaData.track() )
            .replace( QString("%3"), metaData.album() );
        
        QString nowPlaying = cmd + m_lastNotification;

        #if SKYPE_SEND_URL
            QString url = metaData.trackPageUrl();
            if ( !url.isEmpty() )
            {
                if ( url.startsWith( "http://" ) ) { url.remove( 0, 7 ); }
                if ( url.endsWith( "/" ) ) { url.chop( 1 ); }
                url.append( "/?s" ); // identifies link as coming from Skype

                nowPlaying += " " + url;
            }
        #endif // SKYPE_SEND_URL                
        
        utfToSend = nowPlaying.toUtf8();
    }

    sendToSkype( utfToSend );

    LOGL( 4, "Sending to Skype: " << QString::fromUtf8( utfToSend ) );

    m_haveWaiting = false;
}

void
SkypeNotifyExtension::notifyWaiting()
{
    // This is always called by the subclass after a successful connect.

    // First, query for old mood message. As soon as we get the reply,
    // we'll send the waiting notification.
    QByteArray utfToSend = kGetMoodMsgCmd.toUtf8();
    sendToSkype( utfToSend );
}

void
SkypeNotifyExtension::receiveFromSkype( QByteArray utfResp )
{
    QString resp = QString::fromUtf8( utfResp );

    LOGL( 4, "Message from Skype: " << resp );

    // Parse response here and look for confirmation of mood message changes
    QString moodChangedCmd( "PROFILE MOOD_TEXT" );
    int cmdLen = moodChangedCmd.size();
    if ( resp.startsWith( moodChangedCmd ) )
    {
        QString msg = resp.right( resp.size() - ( cmdLen + 1 ) );
        if ( !msg.isEmpty() )
        {
            // Compare and store if different to last one sent
            if ( msg != m_lastNotification )
            {
                LOGL( 3, "Skype old mood message found, saving: " << msg );

                // Save out user's old message to config
                m_config->beginGroup( name() );
                m_config->setValue( "SavedMoodMessage", msg );
                m_config->endGroup();                
            }
        }

        if ( m_haveWaiting )
        {
            // Now that we've stored any existing mood msg, we can send the
            // waiting notification
            LOGL( 3, "Sending waiting notification to Skype" );
            TrackInfo info = m_cachedMetadata;
            onAppEvent( Event::PlaybackStarted, QVariant::fromValue( info ) );;
        }
    }
    
}
    
bool
SkypeNotifyExtension::isEnabled()
{
    Q_ASSERT( m_config != 0 );

    m_config->beginGroup( name() );
    bool enabled = m_config->value( "Enabled", m_enabledByDefault ).toBool();
    m_config->endGroup();
    
    return enabled;
}

QString
SkypeNotifyExtension::nowPlayingFormat()
{
    Q_ASSERT( m_config != 0 );

    m_config->beginGroup( name() );
    QString format = m_config->value( "Format", m_defaultFormat ).toString();
    m_config->endGroup();
    
    return format;
}


