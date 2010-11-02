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

#include <QtGui>
#include "mediadevicewatcher.h"

#include "MooseCommon.h"
#include "logger.h"
#include "controlinterface.h"
#include "breakpad/BreakPad.h"
#include "LastFmSettings.h"


#ifdef WIN32
    #include <windows.h>
    #include <dbt.h>

    char FirstDriveFromMask (ULONG unitmask);

    /*------------------------------------------------------------------
        FirstDriveFromMask (unitmask)

        Description
        Finds the first valid drive letter from a mask of drive letters.
        The mask must be in the format bit 0 = A, bit 1 = B, bit 3 = C,
        etc. A valid drive letter is defined when the corresponding bit
        is set to 1.

        Returns the first drive letter that was found.
    --------------------------------------------------------------------*/

    char FirstDriveFromMask( ULONG unitmask )
    {
        char i;

        for ( i = 0; i < 26; ++i )
        {
            if ( unitmask & 0x1 )
                break;
            unitmask = unitmask >> 1;
        }

        return (i + 'A');
    }


    class DeviceTargetWindow : public QWidget
    {
        public:
            DeviceTargetWindow( MediaDeviceWatcher* parent );

            protected:
                virtual bool winEvent( MSG* message, long* result );

            private:
                MediaDeviceWatcher* m_parent;
    };


    DeviceTargetWindow::DeviceTargetWindow( MediaDeviceWatcher* parent ) :
        QWidget( NULL )
    {
        m_parent = parent;

        DEV_BROADCAST_HDR hdr;
        ZeroMemory( &hdr, sizeof(hdr) );
        hdr.dbch_size = sizeof(DEV_BROADCAST_HDR);
        hdr.dbch_devicetype = DBT_DEVTYP_VOLUME;

        HDEVNOTIFY hd = RegisterDeviceNotification( winId(), reinterpret_cast<void*>(&hdr), DEVICE_NOTIFY_WINDOW_HANDLE );
    }
#endif


//#ifdef WIN32
    #define Q_APPLICATION QApplication
//#else
//     #define Q_APPLICATION QCoreApplication
//#endif


bool
isAlreadyRunning()
{
  #ifdef WIN32
    QString id( "LastfmHelper-F396D8C8-9595-4f48-A319-48DCB827AD8F" );
    ::CreateMutexA( NULL, false, id.toAscii() );

    // The call fails with ERROR_ACCESS_DENIED if the Mutex was 
    // created in a different users session because of passing
    // NULL for the SECURITY_ATTRIBUTES on Mutex creation);
    if ( ::GetLastError() == ERROR_ALREADY_EXISTS || ::GetLastError() == ERROR_ACCESS_DENIED )
    {
        return true;
    }
    else
    {
        // If we didn't find another mutex, it could still be the case that an instance
        // of an older version (prior to 1.4.0) is running so fall back to the old method
        return ControlInterface::sendToInstance();
    }
  #else
    return ControlInterface::sendToInstance();
  #endif
}


class ListenerApplication : public Q_APPLICATION
{
public:
    ListenerApplication( int argc, char **argv );
    ~ListenerApplication();

    bool quit() const { return m_quit; }

private:
    bool m_quit;
    ControlInterface* m_controlInterface;
    MediaDeviceWatcher* m_mdw;

  #ifdef WIN32
    DeviceTargetWindow* m_wnd;
  #endif
};


ListenerApplication::ListenerApplication( int argc, char **argv )
        : Q_APPLICATION( argc, argv ),
          m_quit( false ),
          m_controlInterface( 0 ),
          m_mdw( 0 )
{
  #ifdef WIN32
    m_wnd = 0;
  #endif

    Logger& logger = Logger::GetLogger();
    logger.Init( MooseUtils::logPath( "LastFmHelper.log" ), false );
    logger.SetLevel( Logger::Debug );

    QStringList argList = arguments();
    argList.removeAt( 0 );

    if ( isAlreadyRunning() )
    {
        QString params = argList.join( " " );
        if ( !params.isEmpty() )
        {
            ControlInterface::sendToInstance( params );
        }
        m_quit = true;
    }
    else if ( argList.contains( "--quit" ) )
    {
        m_quit = true;
    }

    if ( !m_quit )
    {
        LOGL( 1, "App version: " << The::settings().version() );

        m_controlInterface = new ControlInterface( this );
        m_mdw = new MediaDeviceWatcher();

      #ifdef WIN32
        m_wnd = new DeviceTargetWindow( m_mdw );
      #endif
    }
}


ListenerApplication::~ListenerApplication()
{
    if ( m_quit ) return;

  #ifdef WIN32
    delete m_wnd;
  #endif
    delete m_mdw;
    delete m_controlInterface;
}


#ifdef WIN32
bool
DeviceTargetWindow::winEvent( MSG * msg, long * result )
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

    if ( msg->message != WM_DEVICECHANGE )
        return false;

    PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)msg->lParam;

    switch( msg->wParam )
    {
        case DBT_DEVICEARRIVAL:
        {
            if (lpdb -> dbch_devicetype == DBT_DEVTYP_VOLUME)
            {
                PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
                //if (lpdbv -> dbcv_flags & DBTF_MEDIA)
                    qDebug() << "Drive" << FirstDriveFromMask(lpdbv ->dbcv_unitmask) << " media has arrived.";

                m_parent->forceDetection( QString( FirstDriveFromMask(lpdbv ->dbcv_unitmask)) + ":\\" );

                return true;
            }
        }
        break;

        case DBT_DEVICEREMOVECOMPLETE:
        {
            if (lpdb -> dbch_devicetype == DBT_DEVTYP_VOLUME)
            {
                PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
                //if (lpdbv -> dbcv_flags & DBTF_MEDIA)
                    qDebug() << "Drive" << FirstDriveFromMask(lpdbv ->dbcv_unitmask) << " media is gone.";

                return true;
            }
        }
        break;
   }

    return false;
}
#endif


int main( int argc, char **argv )
{
    #ifndef NBREAKPAD
    BreakPad breakpad( MooseUtils::savePath( "" ) );
    breakpad.setProductName( "Last.fm Helper" );
    #endif

    // used by some Qt stuff, eg QSettings
    // leave first! As Settings object is created quickly
    QCoreApplication::setApplicationName( "Last.fm" );
    QCoreApplication::setOrganizationName( "Last.fm" );
    QCoreApplication::setOrganizationDomain( "last.fm" );

    ListenerApplication app( argc, argv );
    int r = 0;

    if ( !app.quit() )
    {
        qDebug() << "Running n waiting!";
        r = app.exec();
    }

    qDebug() << "Quitting helper";

    return r;
}
