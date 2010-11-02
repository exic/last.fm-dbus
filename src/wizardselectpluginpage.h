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

#ifndef WIZARDSELECTPLUGINPAGE_H
#define WIZARDSELECTPLUGINPAGE_H

#include "plugininfo.h"

#include "ui_selectpluginwidget.h"

#include <vector>

class SimpleWizard;

class WizardSelectPluginPage : public QWidget
{
    Q_OBJECT

public:

    /*********************************************************************/ /**
        Ctor
    **************************************************************************/
    WizardSelectPluginPage(
        SimpleWizard* wizard);

    /*********************************************************************/ /**
        Populates the list box with the player names in the passed vector.
    **************************************************************************/
    void
    Populate(
        std::vector<CPluginInfo>& plugins);

    /*********************************************************************/ /**
        Fills the vector with the indices into the players vector of the
        entries that are checked.
    **************************************************************************/
    void
    GetChecked(
        std::vector<int>& indices);

public slots:

    /*********************************************************************/ /**
        Launches the Add Player dialog.
    **************************************************************************/
    void
    addPlayer();

    /*********************************************************************/ /**
        Enables/disables Next button depending on checked options.
    **************************************************************************/
    void
    pageTouched();

private:
    Ui::SelectPluginWidget ui;

    SimpleWizard* m_Wizard;
    std::vector<CPluginInfo>* mpPlayerVector;

    friend class SimpleWizard;
};

#endif // WIZARDSELECTPLUGINPAGE_H
