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

#ifndef SIMPLEWIZARDWIN_H
#define SIMPLEWIZARDWIN_H

#include "simplewizard.h"

// we do this because qt 4.3.x uic is b0rked and removes spacing with ifdefs
#ifdef Q_OS_MAC
    #undef Q_OS_MAC
    #include "ui_wizardshell_mac.h"
    #define Q_OS_MAC
#else
    #include "ui_wizardshell_mac.h"
#endif


class SimpleWizardMac : public SimpleWizard
{
    Q_OBJECT

public:

    SimpleWizardMac(
        QWidget* parent = 0);

protected:

    virtual void
    switchPage(
        QWidget* oldPage,
        QWidget* newPage);

    virtual QString
    headerForPage(
        int index) = 0;


private:
    Ui::WizardShellMac uiInt;

    QWidget  m_IntShell;
};

#endif // SIMPLEWIZARDWIN_H
