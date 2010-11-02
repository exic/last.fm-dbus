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

#include "simplewizard.h"

#include <QDebug>


SimpleWizard::SimpleWizard(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    // This stretch pushes all wizard pages upward, we don't want that
    //ui.vboxLayout->insertStretch(0, 1);
    #ifdef WIN32
    ui.line->setFrameShadow(QFrame::Sunken);
    #endif

    connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui.backButton, SIGNAL(clicked()), this, SLOT(backButtonClicked()));
    connect(ui.nextButton, SIGNAL(clicked()), this, SLOT(nextButtonClicked()));
}

void
SimpleWizard::enableNext(
    bool enable)
{
    ui.nextButton->setEnabled(enable);
}

void
SimpleWizard::enableBack(
    bool enable)
{
    ui.backButton->setEnabled(enable);
}

void SimpleWizard::setNumPages(int n)
{
    mNumPages = n;
    //mHistory.append(createPage(0));
    //switchPage(NULL, mHistory.last());
}

void SimpleWizard::backButtonClicked()
{
    ui.nextButton->setEnabled(true);

    QWidget *oldPage = mHistory.takeLast();
    switchPage(oldPage, mHistory.last());
    delete oldPage;
}

void SimpleWizard::nextButtonClicked()
{
    ui.nextButton->setEnabled(true);

    if (mHistory.size() == mNumPages)
    {
        // We're on last page and Finish was just clicked
        accept();
    }
    else
    {
        QWidget *oldPage = mHistory.isEmpty() ? NULL : mHistory.last();
        QWidget* newPage = createPage( mHistory.size() );
        if ( newPage )
        {
            mHistory += newPage;
            switchPage(oldPage, mHistory.last());
        }
        else
            nextButtonClicked();
    }
}

void SimpleWizard::switchPage(QWidget* oldPage, QWidget* newPage)
{
    if (oldPage) {
        oldPage->hide();
        ui.vboxLayout->removeWidget(oldPage);
    }

    ui.vboxLayout->insertWidget(0, newPage);
    newPage->show();
    newPage->setFocus();

    updateButtons();
}

void
SimpleWizard::updateButtons()
{
    if (mHistory.size() == 1)
    {
        // Always disable Back on first page
        ui.backButton->setEnabled(false);
    }

    if (mHistory.size() == mNumPages)
    {
        // Change Next into Finish
        ui.nextButton->setText(tr("Finish"));
    }
    else
    {
        #ifdef Q_WS_MAC
            ui.nextButton->setText(tr("Continue"));
        #else
            ui.nextButton->setText(tr("Next >"));
        #endif
        ui.nextButton->setDefault(true);
    }

    setWindowTitle( mTitle );
}
