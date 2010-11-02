/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
 *      Max Howell, Last.fm Ltd <max@last.fm>                              *
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

#include "systray.h"
#include "iconshack.h"
#include "container.h"
#include "LastFmSettings.h"
#include "MooseCommon.h"


TrayIcon::TrayIcon( QObject* parent )
        : QSystemTrayIcon( parent ),
          m_currentAnimationFrame( 0 )
{
  #ifdef Q_WS_MAC
    m_pixmap = QPixmap( MooseUtils::dataPath( "icons/systray_mac.png" ) );
    for( int i = 0; i < 4; ++i )
    {
        m_animationPixmaps.push_back( MooseUtils::dataPath( QString( "icons/systray_ipod_mac%1.png" ).arg( i ) ) );
    }
  #endif

    setUser( The::currentUser() );

    connect( &m_animationTimer, SIGNAL( timeout() ),
             this,              SLOT( onAnimationTimer() ) );
}


void
TrayIcon::setUser( LastFmUserSettings& currentUser )
{
    MooseEnums::UserIconColour iconColour = currentUser.icon();
    QPixmap icon;
    if ( currentUser.isLogToProfile() )
    {
        #ifdef Q_WS_MAC
            icon = m_pixmap;
        #else
            icon = IconShack::instance().GetGoodUserIcon( iconColour );
        #endif
    }
    else
    {
        #ifdef Q_WS_MAC
            icon = QIcon( m_pixmap ).pixmap( 19, 12, QIcon::Disabled );
        #else
            icon = IconShack::instance().GetDisabledUserIcon( iconColour );
        #endif
    }

#ifdef Q_WS_WIN
    if( m_animationTimer.isActive() )
        m_animationPixmaps = IconShack::instance().GetIpodScrobblingIcons( iconColour );
#endif

    setIcon( icon );
    m_user = currentUser.username();
    refreshToolTip();
}


void
TrayIcon::setTrack( const MetaData &track )
{
    m_artist = track.artist();
    m_track = track.track();
    refreshToolTip();
}


void
TrayIcon::refreshToolTip()
{
    #ifndef Q_WS_MAC
    // causes issues with OS X menubar, also none
    // of the other OS X menubar icons have a tooltip

    QString tip;
    if ( m_artist != "" && m_track != "" )
    {
        tip = m_artist + " " + QChar(8211) /*en dash*/ + " " + m_track;

        if ( The::settings().allUsers().count() > 1 )
        {
            tip += " | " + m_user;
        }

    }
    else
    {
        tip = tr( "%1 is logged in" ).arg( m_user );
    }

    #ifdef WIN32
        // Good old crappy Win32
        tip.replace( "&", "&&&" );
    #endif

    setToolTip( tip );
    #endif
}

void
TrayIcon::onAnimationTimer()
{
    if( m_animationPixmaps.isEmpty() )
    {
        stopIpodScrobblingAnimation();
        Q_ASSERT( !"Animation should not be started without frames being loaded" );
        return;
    }
    
    m_currentAnimationFrame++;
    if( m_currentAnimationFrame >= m_animationPixmaps.count() )
        m_currentAnimationFrame = 0;

    setIcon( m_animationPixmaps.at( m_currentAnimationFrame ) );
}

void
TrayIcon::startIpodScrobblingAnimation()
{
    #ifdef Q_WS_WIN
    MooseEnums::UserIconColour iconColour = The::currentUser().icon();
    m_animationPixmaps = IconShack::instance().GetIpodScrobblingIcons( iconColour );
    #endif
    m_animationTimer.start( 100 );
}


void
TrayIcon::stopIpodScrobblingAnimation()
{
    m_animationTimer.stop();
}
