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

#include "wizardloginpage.h"
#include "simplewizard.h"

WizardLoginPage::WizardLoginPage(
    SimpleWizard* wizard) :
        LoginWidget(wizard, ADD_USER),
        m_Wizard(wizard)
{
    wizard->enableNext(false);
    wizard->enableBack(true);

    connect(ui.userEdit, SIGNAL(textChanged(const QString &)),
            this,        SLOT(pageTouched()));
    connect(ui.userCombo, SIGNAL(editTextChanged(const QString &)),
            this,         SLOT(pageTouched()));
    connect(ui.passwordEdit, SIGNAL(textChanged(const QString &)),
            this,            SLOT(pageTouched()));

}

void
WizardLoginPage::pageTouched()
{
    if ( (ui.userCombo->currentText().size() != 0 ||
          ui.userEdit->text().size() != 0) && 
        ui.passwordEdit->text().size() != 0)
    {
        m_Wizard->enableNext(true);
    }
    else
    {
        m_Wizard->enableNext(false);
    }
}
