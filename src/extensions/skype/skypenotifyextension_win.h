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

#ifndef SKYPENOTIFY_EXTENSION_WIN_H
#define SKYPENOTIFY_EXTENSION_WIN_H

#ifdef WIN32

#include "skypenotifyextension.h"
#include "interfaces/ExtensionInterface.h"
#include "metadata.h"

class SkypeNotifyExtensionWin : public SkypeNotifyExtension
{
    Q_OBJECT
    Q_INTERFACES( ExtensionInterface )

    public:
        SkypeNotifyExtensionWin();
        virtual ~SkypeNotifyExtensionWin();

        virtual bool isSkypeAttached();

    protected:
        virtual void sendToSkype( QByteArray cmd );

    private:

        class SkypeTargetWindow : public QWidget
        {
            public:
            
                enum {
                    // Client is successfully attached and API window handle can be found in wParam parameter
	                SKYPECONTROLAPI_ATTACH_SUCCESS = 0,
    	             
                    // Skype has acknowledged connection request and is waiting for confirmation from the user.
	                SKYPECONTROLAPI_ATTACH_PENDING_AUTHORIZATION = 1,	
    												
	                // User has explicitly denied access to client
	                SKYPECONTROLAPI_ATTACH_REFUSED = 2,								

                    // API is not available at the moment. For example, this happens when no user is currently logged in.
				    // Client should wait for SKYPECONTROLAPI_ATTACH_API_AVAILABLE broadcast before making any further
				    // connection attempts.
	                SKYPECONTROLAPI_ATTACH_NOT_AVAILABLE = 3,					
	                SKYPECONTROLAPI_ATTACH_API_AVAILABLE = 0x8001
                };

                SkypeTargetWindow( SkypeNotifyExtensionWin* parent ) :
                    m_parent( parent ) { }
                
            protected:
                virtual bool winEvent( MSG* message, long* result );
                
            private:
                SkypeNotifyExtensionWin* m_parent;
                                
        };
        friend class SkypeTargetWindow;

        virtual void initSkypeCommunication();
        void discoverSkypeWindow();
        
        SkypeTargetWindow m_skypeTargetWin;
        
    private slots:
};

#endif // WIN32

#endif // SKYPENOTIFY_EXTENSION_WIN_H
