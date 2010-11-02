/***************************************************************************
*   Copyright (C) 2005 - 2007 by                                          *
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

#include "PluginBootstrapper.h"
#include "MooseCommon.h"
#include "LastFmSettings.h"
#include "LastMessageBox.h"

#include <QSettings>
#include <QFile>


PluginBootstrapper::PluginBootstrapper( QString pluginId, QObject* parent )
                   :AbstractBootstrapper( parent ),
                    m_pluginId( pluginId )
{
    connect( this, SIGNAL( done( int ) ),
             this, SLOT( onUploadCompleted( int ) ) );
}


PluginBootstrapper::~PluginBootstrapper(void)
{
}


void
PluginBootstrapper::bootStrap()
{
    #undef QSettings
    QSettings bootstrap( QSettings::NativeFormat, QSettings::UserScope, "Last.fm", "Bootstrap", this );

    bootstrap.setValue( m_pluginId, The::currentUser().username() );
    bootstrap.setValue( "data_path",  MooseUtils::savePath( "" ) );

    bootstrap.setValue( "Strings/progress_label",       tr("Last.fm is importing your current media library...") );
    bootstrap.setValue( "Strings/complete_label",       tr("Last.fm has imported your media library.\n\n Click OK to continue.") );
    bootstrap.setValue( "Strings/progress_title",       tr("Last.fm Library Import") );
    bootstrap.setValue( "Strings/cancel_confirmation",  tr("Are you sure you want to cancel the import?") );
    bootstrap.setValue( "Strings/no_tracks_found",      tr("Last.fm couldn't find any played tracks in your media library.\n\n Click OK to continue.") );

    The::currentUser().setBootStrapPluginId( m_pluginId );
}


void
PluginBootstrapper::submitBootstrap()
{
    QString savePath = MooseUtils::savePath( The::currentUsername() + "_" + m_pluginId + "_bootstrap.xml" );
    QString zipPath = savePath + ".gz";

    zipFiles( savePath, zipPath );
    sendZip( zipPath );
}


void
PluginBootstrapper::onUploadCompleted( int status )
{
    QString savePath = MooseUtils::savePath( The::currentUsername() + "_" + m_pluginId + "_bootstrap.xml" );
    QString zipPath = savePath + ".gz";

    if( status == Bootstrap_Ok  )
    {
        QFile::remove( savePath );

        LastMessageBox::information( tr("Media Library Import Complete"),
            tr( "Last.fm has submitted your listening history to the server.\n"
            "Your profile will be updated with the new tracks in a few minutes.") );

    }
    else if( status == Bootstrap_Denied )
    {
        LastMessageBox::warning( tr("Library Import Failed"),
            tr( "Sorry, Last.fm was unable to import your listening history. "
                "This is probably because you've already scrobbled too many tracks. "
                "Listening history can only be imported to brand new profiles.") );
        QFile::remove( savePath );
    }
}
