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

#include "wizardbootstrappage.h"
#include "simplewizard.h"
#include "logger.h"

WizardBootstrapPage::WizardBootstrapPage(
    SimpleWizard*   wizard,
    const QString&  info ) :
        QWidget( wizard )
{
    uiProgress.setupUi( this );
    uiProgress.progressBar->setRange( 0, 100 );

    uiProgress.topLabel->setText( info );

    // Hook up our signals to the widgets
    connect( this,                   SIGNAL( infoChanged( QString ) ),
             uiProgress.topLabel,    SLOT( setText( const QString& ) ) );

    wizard->enableNext( false );
    wizard->enableBack( false );
}


void
WizardBootstrapPage::onTrackFound( int percentage, const TrackInfo& track )
{
    uiProgress.progressBar->setValue( percentage );
    
    if ( !track.isEmpty() )
        uiProgress.bottomLabel->setText( tr( "Found" ) + QString( ": %1 - %2" )
            .arg( track.artist() ).arg( track.track() ) );

    qApp->processEvents();
}


void
WizardBootstrapPage::onUploadProgress( int percentage )
{
    uiProgress.progressBar->setValue( percentage );
    uiProgress.bottomLabel->setText( tr("Sending your listening history to Last.fm") );
}


void
WizardBootstrapPage::setInfo( QString info )
{
    emit infoChanged( info );
}

