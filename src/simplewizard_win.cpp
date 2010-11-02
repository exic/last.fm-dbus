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

#include <QtGui>

#include "simplewizard_win.h"
#include "MooseCommon.h"

SimpleWizardWin::SimpleWizardWin(
    QWidget *parent) :
        SimpleWizard(parent),
        m_ExtShell(this),
        m_IntShell(this),
        m_LastShell(NULL)
{
    // Draw our widgets for later use
    uiExt.setupUi(&m_ExtShell);
    m_ExtShell.setAutoFillBackground(true);

#ifdef WIN32
    QPixmap watermark( MooseUtils::dataPath( "wizard.png" ) );
#endif
#ifdef Q_WS_MAC
    QPixmap watermark( MooseUtils::dataPath( "wizard_mac.png" ) );
#endif
#ifdef Q_WS_X11
    QPixmap watermark( MooseUtils::dataPath( "wizard_generic.png" ) );
#endif

    uiExt.watermarkLabel->setPixmap( watermark );
    uiInt.setupUi(&m_IntShell);
    uiInt.line->setFrameShadow(QFrame::Sunken);

    QPixmap corner( MooseUtils::dataPath( "app_55.png" ) );
    uiInt.cornerLabel->setPixmap( corner );
    
    // Add shells to base class ui initially hidden
    m_IntShell.hide();
    m_ExtShell.hide();
    ui.vboxLayout->insertWidget(0, &m_ExtShell);
    ui.vboxLayout->insertWidget(0, &m_IntShell);
}

void
SimpleWizardWin::switchPage(
    QWidget* oldPage,
    QWidget* newPage)
{
    // Currently some discrepancy between how we add pages to the ext and
    // internal shell because of a stupd bug in Qt 4.2.0 which made the
    // external dialog jump when using the frame.layout()->add method.

    if (oldPage) {
        oldPage->hide();
        uiExt.vboxLayout->removeWidget(oldPage);
        uiInt.pageFrame->layout()->removeWidget(oldPage);
        m_LastShell->hide();
    }

    // Pick the correct shell
    if ( currentPage() == 0 || currentPage() == numPages() - 1 )
    {
        // It's an external page
        uiExt.vboxLayout->addWidget(newPage);
        uiExt.headerLabel->setText(headerForPage(currentPage()));
        m_LastShell = &m_ExtShell;
    }
    else
    {
        // It's internal
        uiInt.pageFrame->layout()->addWidget(newPage);
        uiInt.headerLabel->setText(headerForPage(currentPage()));
        m_LastShell = &m_IntShell;
    }

    m_LastShell->show();
    newPage->show();
    newPage->setFocus();

    updateButtons();
}
