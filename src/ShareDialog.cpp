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

#include <QTimer>
#include <QPushButton>

#include "ShareDialog.h"
#include "Settings.h"
#include "WebService.h"
#include "WebService/Request.h"
#include "logger.h"


ShareDialog::ShareDialog( QWidget* parent )
        : QDialog( parent, Qt::Dialog | Qt::Window )
{
    ui.setupUi( this );

    connect( &The::settings(), SIGNAL( userSwitched( LastFmUserSettings& ) ), SLOT( onUserChanged( LastFmUserSettings& ) ) );
    connect( ui.userEdit, SIGNAL( editTextChanged( QString ) ), SLOT( onRecipientChanged( QString ) ) );

    connect( ui.buttonBox, SIGNAL( accepted() ), SLOT( accept() ) );
    connect( ui.buttonBox, SIGNAL( rejected() ), SLOT( reject() ) );

    onUserChanged( The::settings().currentUser() );

    ui.buttonBox->button( QDialogButtonBox::Ok )->setText( tr("Share") );

    #ifdef Q_WS_MAC
        ui.messageEdit->setCurrentFont( ui.recommendTypeBox->font() );
        ui.messageEdit->setFont ( ui.recommendTypeBox->font() ); // Removes Mac bug with too small messageEdit text display
    #endif
}


void
ShareDialog::setSong( const MetaData& metaData )
{
    m_metaData = metaData;

    ui.recommendTypeBox->setItemText( 0, tr( "Artist: %1" ).arg( metaData.artist() ) );
    ui.recommendTypeBox->setItemText( 1, tr( "Track: %1 - %2" ).arg( metaData.artist() ).arg( metaData.track() ) );

    if ( ui.recommendTypeBox->count() == 2 )
        ui.recommendTypeBox->addItem( tr( "Album: %1 - %2" ).arg( metaData.artist() ).arg( metaData.album() ) );
    else
        ui.recommendTypeBox->setItemText( 2, tr( "Album: %1 - %2" ).arg( metaData.artist() ).arg( metaData.album() ) );
}

void
ShareDialog::setSong( const Track& track )
{
    m_metaData.setTrack( track.title() );
    m_metaData.setArtist( track.artist() );

    ui.recommendTypeBox->setItemText( 0, tr( "Artist: %1" ).arg( track.artist() ) );
    ui.recommendTypeBox->setItemText( 1, tr( "Track: %1 - %2" ).arg( track.artist() ).arg( track.title() ) );
    if ( ui.recommendTypeBox->count() == 3 )
        ui.recommendTypeBox->removeItem( 2 );
}


void
ShareDialog::onRecipientChanged( const QString& recipient )
{
    ui.buttonBox->button( QDialogButtonBox::Ok )->setEnabled( !recipient.trimmed().isEmpty() );
}


void
ShareDialog::onUserChanged( LastFmUserSettings &user )
{
    ui.userEdit->clear(); // clear out previous user's friends
    ui.userEdit->addItem( tr("Loading...") ); // otherwise user will be confused by the empty list
    ui.userEdit->setEditText( user.lastRecommendee() );
    ui.recommendTypeBox->setCurrentIndex( user.lastRecommendType() );
}


int
ShareDialog::exec()
{
    Q_DEBUG_BLOCK;

    ui.messageEdit->clear();

    FriendsRequest *friends = new FriendsRequest;
    connect( friends, SIGNAL( result( Request* ) ), SLOT( onFriendsReturn( Request* ) ) );
    friends->start();

    return QDialog::exec();
}


void
ShareDialog::onFriendsReturn( Request *request )
{
    // preserve the editText as clear() clears that too
    QString const edit_text = ui.userEdit->currentText();
    QStringList const usernames = static_cast<FriendsRequest*>(request)->usernames();

    ui.userEdit->clear();
    ui.userEdit->addItems( usernames );
    ui.userEdit->setEditText( edit_text );
}


void
ShareDialog::accept()
{
    RecommendRequest *recommend = new RecommendRequest;
    recommend->setTargetUsername( ui.userEdit->currentText() );
    recommend->setMessage( ui.messageEdit->toPlainText() );
    recommend->setArtist( m_metaData.artist() );
    recommend->setLanguage( The::settings().appLanguage() );

    switch (ui.recommendTypeBox->currentIndex())
    {
        case 0:
            recommend->setType( UnicornEnums::ItemArtist );
            break;

        case 1:
            recommend->setType( UnicornEnums::ItemTrack );
            recommend->setToken( m_metaData.track() );
            break;

        case 2:
            recommend->setType( UnicornEnums::ItemAlbum );
            recommend->setToken( m_metaData.album() );
            break;
    }

    recommend->start();

    LastFmUserSettings &user = The::settings().currentUser();
    user.setLastRecommendee( ui.userEdit->currentText() );
    user.setLastRecommendType( ui.recommendTypeBox->currentIndex() );

    QDialog::accept();
}
