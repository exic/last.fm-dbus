/***************************************************************************
 *   Copyright 2007-2008 Last.fm Ltd.                     		           *
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

#include "growlextension.h"

#include "UnicornCommonMac.h"
#include "libUnicorn/logger.h"
#include "libUnicorn/AppleScript.cpp"

#include "lastfmapplication.h"

#include <QFileInfo>
#include <QSystemTrayIcon>
#include <QtPlugin>


GrowlNotifyExtension::GrowlNotifyExtension()
        : m_parent( 0 )
{
    connect( qApp, SIGNAL(event( int, QVariant )), SLOT(onAppEvent( int, QVariant )) );
}


void
GrowlNotifyExtension::onAppEvent( int e, const QVariant& data )
{
    switch (e) 
    {
        case Event::PlaybackStarted:
        case Event::TrackChanged:
            break;
        default:
            return;
    }

    // conveniently, this determines if Growl is installed for us
    if (UnicornUtils::isGrowlInstalled() == false)
        return;

    TrackInfo metaData = data.value<TrackInfo>();

    #define APPEND_IF_NOT_EMPTY( x ) if (!x.isEmpty()) description += x + '\n';
    QString description;
    APPEND_IF_NOT_EMPTY( metaData.artist() );
    APPEND_IF_NOT_EMPTY( metaData.album() );
    description += metaData.durationString();

    QString title = metaData.track();
    if (title.isEmpty())
        title = QFileInfo( metaData.path() ).fileName();

    AppleScript script;
    script << "tell application 'GrowlHelperApp'"
           <<     "register as application 'Last.fm'"
                          " all notifications {'Track Notification'}"
                          " default notifications {'Track Notification'}"
                          " icon of application 'Last.fm.app'"
           <<     "notify with name 'Track Notification'"
                          " title " + AppleScript::asUnicodeText( title ) +
                          " description " + AppleScript::asUnicodeText( description ) + 
                          " application name 'Last.fm'"
           << "end tell";
    script.exec();
}


Q_EXPORT_PLUGIN2( growl, GrowlNotifyExtension )
