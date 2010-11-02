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

#include "wizardselectpluginpage.h"
#include "simplewizard.h"
#include "addplayerdialog.h"

#include <string>

using namespace std;

/******************************************************************************
    WizardSelectPluginPage
******************************************************************************/
WizardSelectPluginPage::WizardSelectPluginPage(
    SimpleWizard*   wizard) :
        QWidget( wizard )
{
    ui.setupUi( this );

    // Hook up widget signals to our slots
    connect(ui.addButton,  SIGNAL( clicked() ),
            this,          SLOT  ( addPlayer()) );
    connect(ui.pluginList, SIGNAL( itemChanged( QListWidgetItem* ) ),
            this,          SLOT  ( pageTouched()) );

    wizard->enableNext( true );
    wizard->enableBack( false );

    m_Wizard = wizard;
}

/******************************************************************************
    addPlayer
******************************************************************************/
void
WizardSelectPluginPage::addPlayer()
{
    Q_ASSERT(mpPlayerVector != NULL);

    AddPlayerDialog dlg(*mpPlayerVector, this);
    if (dlg.exec() == QDialog::Accepted)
    {
        int nIdx = dlg.GetSelected();
        if (nIdx == -1)
        {
            return;
        }

        CPluginInfo& added = mpPlayerVector->at(nIdx);

        QString name = added.GetPlayerName();
        //name += " " + tr("(plugin not installed)");

        QListWidgetItem* item = new QListWidgetItem(name, ui.pluginList);
        item->setCheckState(Qt::Checked);

        // Store its position in players vector
        item->setData(1, nIdx);
    }

}

/******************************************************************************
    Populate
******************************************************************************/
void
WizardSelectPluginPage::Populate(
    vector<CPluginInfo>& plugins)
{
    ui.pluginList->clear();

    for (int i = 0; i < static_cast<int>(plugins.size()); ++i)
    {
        CPluginInfo& current = plugins.at(i);

        if (current.IsPlayerInstalled())
        {
            QString name = current.GetPlayerName();
            int nCheck;
            if (!current.IsInstalled())
            {
                //name += " " + tr("(plugin not installed)");
                nCheck = 1;
            }
            else if (current.IsInstalled() && current.IsVersionNewer())
            {
                name += " " + tr("(newer version available)");
                nCheck = 1;
            }
            else
            {
                name += " " + tr("(plugin installed)");
                nCheck = 0;
            }

            QListWidgetItem* item = new QListWidgetItem(name, ui.pluginList);
            item->setCheckState(nCheck == 1 ? Qt::Checked : Qt::Unchecked);

            // Store its position in players vector along with string
            item->setData(1, i);
        }
    }

    mpPlayerVector = &plugins;

    pageTouched();
}

/******************************************************************************
    pageTouched
******************************************************************************/
void
WizardSelectPluginPage::pageTouched()
{
    // Don't want to disable Next here because then you can't complete
    // a first run config with plugins already installed.

/*
    // Only enable Next if at least one entry is checked
    for (int i = 0; i < ui.pluginList->count(); ++i)
    {
        if (ui.pluginList->item(i)->checkState() == Qt::Checked)
        {
            m_Wizard->enableNext(true);
            return;
        }
    }
    m_Wizard->enableNext(false);
*/
}

/******************************************************************************
    GetChecked
******************************************************************************/
void
WizardSelectPluginPage::GetChecked(
    std::vector<int>& indices)
{
    // Step through list box and store the indices of the checked ones
    for (int i = 0; i < ui.pluginList->count(); ++i)
    {
        if (ui.pluginList->item(i)->checkState() == Qt::Checked)
        {
            int nPlayerIdx = ui.pluginList->item(i)->data(1).toInt();
            indices.push_back(nPlayerIdx);
        }
    }
}
