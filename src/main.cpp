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

#include "lastfmapplication.h"
#include "breakpad/BreakPad.h"
#include "MooseCommon.h"
#include "logger.h"
#include "version.h"

// for Mac, see getURLEvent() below
static QString openUrl;


/******************************************************************************
    getURLEvent (Apple's way to tell us about cli-args)
******************************************************************************/

#ifdef Q_WS_MAC
#include <ApplicationServices/ApplicationServices.h>

static pascal OSErr
getURLEvent( const AppleEvent* appEvent, AppleEvent* /*reply*/, long /*handlerRefcon*/ )
{
    qDebug() << "Receiving apple url event";

    DescType type;
    Size size;

    char buf[1024];
    AEGetParamPtr( appEvent, keyDirectObject, typeChar, &type, &buf, 1023, &size );
    buf[size] = '\0';

    openUrl = QString::fromUtf8( buf );

    // Send to currently running instance, otherwise variable openUrl
    // will be picked up by the main function
    Moose::sendToInstance( openUrl, Moose::DontStartNewInstance );

    return noErr;
}


static pascal OSErr
getOpenEvent( const AppleEvent* /*appEvent*/, AppleEvent* /*reply*/, long /*handlerRefcon*/ )
{
    qDebug() << "Receiving apple open event";

    Moose::sendToInstance( "container://show", Moose::DontStartNewInstance );

    return noErr;
}

#endif //Q_WS_MAC


/******************************************************************************
    main
******************************************************************************/
int main( int argc, char *argv[] )
{
    #ifndef NBREAKPAD
    BreakPad breakpad( MooseUtils::savePath() );
    breakpad.setProductName( "Moose" );
    #endif

    // used by some Qt stuff, eg QSettings
    // leave first! As Settings object is created quickly
    QCoreApplication::setApplicationName( "Last.fm" );
    QCoreApplication::setOrganizationName( "Last.fm" );
    QCoreApplication::setOrganizationDomain( "last.fm" );

#ifdef WIN32
    QString logPath( MooseUtils::logPath( "Last.fm.log" ) );
    const COMMON_CHAR* logfilename = logPath.utf16();
#else
    QByteArray logPath( QFile::encodeName( MooseUtils::logPath( "Last.fm.log" ) ) );
    const COMMON_CHAR* logfilename = logPath.data();
#endif
    new Logger( logfilename, Logger::Debug );
    LOGL( 3, "App version: " << LASTFM_CLIENT_VERSION );
    LOGL( 3, "OS version: " << UnicornUtils::getOSVersion() );

    #ifdef Q_WS_MAC
    AEInstallEventHandler( 'GURL', 'GURL', NewAEEventHandlerUPP( getURLEvent ), 0, false );
    AEInstallEventHandler( kCoreEventClass, kAEReopenApplication, NewAEEventHandlerUPP( getOpenEvent ), 0, false );
    #endif

    // Check for commandline parameters
    // we support one URL passed, only one. If there is more we use the last one
    bool openToTray = false;
    for ( int i = 1; i < argc; i++ )
    {
        QString const arg = argv[i];

        if ( arg.startsWith( "lastfm://" ) || arg.startsWith( "container://" ) )
            openUrl = QString( argv[i] );

        if ( arg == "-tray" || arg == "--tray" )
            openToTray = true;
    }

    #ifndef LASTFM_MULTI_PROCESS_HACK
        // Creates the application mutex, so don't ever remove this call!
        if ( Moose::isAlreadyRunning() )
        {
            if ( !openToTray )
                Moose::sendToInstance( "container://show" );

            if ( !openUrl.isEmpty() )
                Moose::sendToInstance( openUrl );

            return 0;
        }
    #endif

    #ifdef Q_WS_MAC
        // on mac we do this because mac arguments with urls are handled weirdly
        // ask muesli/chris as he is our expert on this matter
        if ( !openUrl.isEmpty() )
        {
            argc++;
            char** argv_cp = new char*[argc];
            for ( int i = 0; i < argc - 1; ++i )
                argv_cp[i] = argv[i];

            argv_cp[argc - 1] = openUrl.toLocal8Bit().data();
            argv = argv_cp;
        }
    #endif

    LastFmApplication app( argc, argv );
    return app.exec();
}
