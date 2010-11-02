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

#include "deleteuserdialog.h"
#include "LastFmSettings.h"
#include "logger.h"
#include "LastMessageBox.h"


DeleteUserDialog::DeleteUserDialog( QWidget* parent )
        : QDialog( parent )
{
    ui.setupUi( this );

    connect( ui.buttonBox, SIGNAL( accepted() ), SLOT( accept() ) );
    connect( ui.buttonBox, SIGNAL( rejected() ), SLOT( reject() ) );
    
    setFixedSize( sizeHint() );

    // Want etched, not flat
    ui.line->setFrameShadow( QFrame::Sunken );

    // Fill drop down with users
    foreach (QString user, The::settings().allUsers())
        ui.userCombo->addItem( user );
        
    if (ui.userCombo->currentText() == The::settings().currentUsername())
        // current index is 0, there may be no '1' but Qt is bounds-safe
        ui.userCombo->setCurrentIndex( 1 );
}

void
DeleteUserDialog::accept()
{
    QString msgboxTitle = tr("Error");
    
    if ( ui.userCombo->count() <= 1 )
    {
        LastMessageBox::critical( msgboxTitle,
                tr( "There must be at least one user in the system." ),
                QMessageBox::Ok, QMessageBox::NoButton );
        return;
    }

    QString const userToDelete = ui.userCombo->currentText();

    if ( userToDelete == The::settings().currentUsername() )
    {
        LastMessageBox::critical( msgboxTitle,
                tr( "You can't delete the currently active user." ),
                QMessageBox::Ok, QMessageBox::NoButton );
        return;
    }

    int confirm = LastMessageBox::question( tr( "Confirm" ),
            tr( "This will remove the user from the application, "
                "it will not delete the profile from Last.fm.\n\n"
                "Do you want to delete user %1?" ).arg( userToDelete ),
            QMessageBox::Yes, QMessageBox::No, QStringList(), this );

    if ( confirm == QMessageBox::Yes )
    {
        The::settings().deleteUser( userToDelete );
        QDialog::accept();
    }
}
