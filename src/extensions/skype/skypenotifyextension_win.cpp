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

#ifdef WIN32

#include "windows.h"

#include <QtGui>

#include "skypenotifyextension_win.h"
#include "logger.h"

// These are statically scoped because they need to be shared by the two classes
static int skypeAttachId = 0;
static int skypeDiscoverId = 0;
static int skypeWindowId = 0;

SkypeNotifyExtensionWin::SkypeNotifyExtensionWin() :
    m_skypeTargetWin( this )
{
}

SkypeNotifyExtensionWin::~SkypeNotifyExtensionWin()
{
}

bool SkypeNotifyExtensionWin::isSkypeAttached()
{
    return skypeWindowId != 0;
}

void
SkypeNotifyExtensionWin::initSkypeCommunication()
{
    // Get IDs of unique Skype messages
    skypeAttachId = RegisterWindowMessageA("SkypeControlAPIAttach");
	skypeDiscoverId = RegisterWindowMessageA("SkypeControlAPIDiscover");

    if ( skypeAttachId == 0 || skypeDiscoverId == 0 ) 
    {
        LOGL( 1, "RegisterWindowMessage failed for skype messages. GetLastError: " <<
            GetLastError() );
        return;
    }

    discoverSkypeWindow();

    // Now, Skype will return its window handle in the event handler if it's
    // running.
}

void
SkypeNotifyExtensionWin::sendToSkype( QByteArray cmd )
{
    // Send new track to Skype
	COPYDATASTRUCT data;
	data.dwData = 0;
	data.lpData = reinterpret_cast<void*>( cmd.data() );
	data.cbData = cmd.size() + 1;

	LRESULT res = SendMessageA( (HWND)skypeWindowId, WM_COPYDATA,
		                        (WPARAM)m_skypeTargetWin.winId(), (LPARAM)&data );
	if ( res == FALSE )
	{
		skypeWindowId = 0;
		LOGL( 2, "Failed to send notification to Skype. GetLastError: " << GetLastError() );
	}

}

void
SkypeNotifyExtensionWin::discoverSkypeWindow()
{
    // Ask Skype to identify itself
    LRESULT res = SendMessageTimeout(
        HWND_BROADCAST, skypeDiscoverId,
        (WPARAM)m_skypeTargetWin.winId(), 0,
        SMTO_ABORTIFHUNG, 1000, NULL );
    
    if ( res == 0 )
    {
        LOGL( 1, "Skype returned 0 in response to discover broadcast message." );
        return;
    }

    LOGL( 3, "Skype discover message sent" );
}

bool
SkypeNotifyExtensionWin::SkypeTargetWindow::winEvent( MSG* message, long* result )
{
    if ( message->message == skypeAttachId )
    {
		switch( message->lParam )
		{
			case SKYPECONTROLAPI_ATTACH_SUCCESS:
			{
				LOGL( 3, "Attached to Skype successfully" );
				skypeWindowId = static_cast<int>( message->wParam );

                // Let base class transmit potential waiting notification
                m_parent->notifyWaiting();
            }
			break;
			
			case SKYPECONTROLAPI_ATTACH_PENDING_AUTHORIZATION:
			{
				LOGL( 1, "Pending authorization, must wait for attach_success." );
				skypeWindowId = 0;
            }
			break;
			
			case SKYPECONTROLAPI_ATTACH_REFUSED:
            {
				LOGL( 3, "Attach refused" );
				skypeWindowId = 0;
            }
            break;
            
			case SKYPECONTROLAPI_ATTACH_NOT_AVAILABLE:
			{
				LOGL( 3, "Attach not available" );
				skypeWindowId = 0;
			}
			break;
			
			case SKYPECONTROLAPI_ATTACH_API_AVAILABLE:
			{
                // Get this message if Skype starts up after us
                LOGL( 3, "Attach available" );
                
                // Rebroadcast here to get window id
                m_parent->discoverSkypeWindow();
			}
			break;
		}
    
    }

	else if ( message->message == WM_COPYDATA )
    {
		if( message->wParam == skypeWindowId )
		{
			PCOPYDATASTRUCT data = (PCOPYDATASTRUCT)message->lParam;
            QByteArray resp( (const char*)data->lpData, data->cbData );
            m_parent->receiveFromSkype( resp );
        }
	}
		
    *result = 1;
    return true;   
}

Q_EXPORT_PLUGIN2( extension_notify_skype_win, SkypeNotifyExtensionWin )

#endif // WIN32
