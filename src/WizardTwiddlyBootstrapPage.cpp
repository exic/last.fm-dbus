/***************************************************************************
 *   Copyright 2008 Last.fm Ltd.                                           *
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

#include "WizardTwiddlyBootstrapPage.h"

#include "configwizard.h"

#include "UnicornCommon.h"
#include "libUnicorn/logger.h"
#include "libUnicorn/AppleScript.h"

#include "libMoose/LastFmSettings.h"

#include <QProcess>


WizardTwiddlyBootstrapPage::WizardTwiddlyBootstrapPage( QWidget* parent )
                           :QWidget( parent )
{
    ui.setupUi( this );
    ui.progressBar->setRange( 0, 0 );
    ui.topLabel->setText( tr("Please do not unplug your iPod or restart iTunes.") );
    ui.topLabel->setWordWrap( true );
    
    m_process = new QProcess( this );
    connect( m_process, SIGNAL(readyReadStandardOutput()), SLOT(onStdOut()) );
    connect( m_process, SIGNAL(finished( int, QProcess::ExitStatus )), SLOT(onFinished()) );
    connect( m_process, SIGNAL(error( QProcess::ProcessError )), SLOT(onError()) );
}


void
WizardTwiddlyBootstrapPage::showEvent( QShowEvent* )
{
    #ifdef Q_OS_MAC
        // we must quit before hand, then the new plugin is loaded, and this is best
        // launching twiddly will start iTunes again.
        if ( UnicornUtils::iTunesIsOpen() )
        {
            AppleScript( "tell application \"iTunes\" to quit" ).exec();
            UnicornUtils::msleep( 5000 ); //be sure iTunes has quit
        }

        // this symlink prevents a dock icon appearing on Leopard for twiddly
        #define TWIDDLY_EXECUTABLE_NAME "/../../Resources/iPodScrobbler"
    #else
        #define TWIDDLY_EXECUTABLE_NAME "/../iPodScrobbler.exe"
    #endif

    m_process->start( The::settings().path() + TWIDDLY_EXECUTABLE_NAME,
                      QStringList() << "--bootstrap" );
}


void
WizardTwiddlyBootstrapPage::onStdOut()
{
    //TODO possible for half of first line to come first? not likley as flush happens at \n and not before.
    
    QByteArray bytes = m_process->readAllStandardOutput();
    QStringList lines = QString::fromLocal8Bit( bytes ).split( '\n', QString::SkipEmptyParts );
    
    if (ui.progressBar->maximum() == 0 && lines.count())
    {
        ui.progressBar->setMaximum( lines[0].toInt() );
        
        if (lines.count() == 1)
            return;
    }
    
    if (lines.count())
        ui.progressBar->setValue( lines.last().toInt() );
}


void
WizardTwiddlyBootstrapPage::onFinished()
{
    emit done();
}


void
WizardTwiddlyBootstrapPage::onError()
{
    LOGL( 2, "Twiddly failed to run! Shock! Horror! How can this be?" );
    emit done();
}
