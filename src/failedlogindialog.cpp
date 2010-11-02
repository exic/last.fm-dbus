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

#include "MooseCommon.h"

#include "failedlogindialog.h"
#include "LastFmSettings.h"
#include "logger.h"


FailedLoginDialog::FailedLoginDialog( QWidget* parent ) :
    QDialog( parent )
{
    LOGL( 3, "Launching FailedLoginDialog" );

    ui.setupUi(this);

    // Want etched, not flat
    ui.line->setFrameShadow( QFrame::Sunken );
    ui.proxyLine->setFrameShadow( QFrame::Sunken );

    QPixmap cross( MooseUtils::dataPath( "icons/qt_cross.png" ) );
    ui.imgLabel->setPixmap( cross );
    ui.errorLabel->setText(
        tr("Couldn't connect to the internet to verify your user details.\n\n"
           "If you use a proxy to connect to the internet, please click\nthe "
           "button below to enter your proxy details."));

    // Remove and hide the proxy box, and display it when the button is clicked
    ui.proxyFrame->hide();
    ui.proxyLine->hide();
    adjustSize();

    // Populate proxy settings
    ui.proxyHostEdit->setText( The::settings().getProxyHost() );
    ui.proxyPortEdit->setText( QString::number( The::settings().getProxyPort() ) );
    ui.proxyUsernameEdit->setText( The::settings().getProxyUser() );
    ui.proxyPasswordEdit->setText( The::settings().getProxyPassword() );

    connect( ui.showProxyButton, SIGNAL(clicked()),
             this,                 SLOT(showProxyFrame()) );
}

void
FailedLoginDialog::showProxyFrame()
{
    if (ui.proxyFrame->isHidden())
    {
        ui.proxyLine->show();
        ui.proxyFrame->show();
        ui.showProxyButton->setEnabled( false );
        adjustSize();
    }
}

void
FailedLoginDialog::accept()
{
    LOGL(4, "FailedLoginDialog::accept()");

    if (!ui.proxyFrame->isHidden())
    {
        LastFmSettings& s = The::settings();
        
        s.setProxyHost( ui.proxyHostEdit->text() );
        s.setProxyPort( ui.proxyPortEdit->text().toInt() );
        s.setProxyUser( ui.proxyUsernameEdit->text() );
        s.setProxyPassword( ui.proxyPasswordEdit->text() );
        s.setUseProxy( !s.getProxyHost().isEmpty() );
    }

    LOGL(4, "FailedLoginDialog calling QDialog::accept()");

    QDialog::accept();
}
