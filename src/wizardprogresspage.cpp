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

#include "wizardprogresspage.h"
#include "simplewizard.h"
#include "logger.h"

WizardProgressPage::WizardProgressPage(
    SimpleWizard*   wizard,
    const QString&  info,
    const QString&  detailedInfo ) :
        QWidget( wizard )
{
    uiProgress.setupUi( this );

    uiProgress.topLabel->setText( info );
    uiProgress.bottomLabel->setText( detailedInfo );

    // Hook up our signals to the widgets
    connect( this,                   SIGNAL( infoChanged( QString ) ),
             uiProgress.topLabel,    SLOT( setText( const QString& ) ) );

    connect( this,                   SIGNAL( detailedInfoChanged( QString ) ),
             uiProgress.bottomLabel, SLOT( setText( const QString& ) ) );

    wizard->enableNext( false );
    wizard->enableBack( false );
}


void
WizardProgressPage::setProgress(
    int percentage,
    int total )
{
    //LOGL(3, "New QHttp progress: " << percentage << "/" << total);

    uiProgress.progressBar->setRange( 0, total );
    uiProgress.progressBar->setValue( percentage );
}


void
WizardProgressPage::setInfo(
    QString info )
{
    emit infoChanged( info );
}


/*
void
WizardProgressPage::setDetailedInfo(
    const QString& info )
{
    uiProgress.bottomLabel->setText( info );
}
*/
