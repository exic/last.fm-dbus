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

#include "addplayerdialog.h"

#include "container.h"  
#include "logger.h"

#include "LastFmSettings.h"
#include "MooseCommon.h"
#include "LastMessageBox.h"
#include "UnicornCommon.h"

#include <QFileDialog>

using namespace std;

/******************************************************************************
    AddPlayerDialog
******************************************************************************/
AddPlayerDialog::AddPlayerDialog(
    vector<CPluginInfo>& plugins,
    QWidget*             pParent) :
    	QDialog(pParent)
{
    ui.setupUi(this);

    // Want etched, not flat
    ui.line->setFrameShadow(QFrame::Sunken);

    connect(ui.browseButton, SIGNAL(clicked()),
            this,            SLOT(browse()));

    mpPlayerVector = &plugins;
    mnSelected = -1;

    // Fill drop down with supported players
    for (size_t i = 0; i < mpPlayerVector->size(); ++i)
    {
        CPluginInfo& current = mpPlayerVector->at(i);
        ui.playerCombo->addItem(current.GetPlayerName());
    }

}

/******************************************************************************
    ~AddPlayerDialog
******************************************************************************/
AddPlayerDialog::~AddPlayerDialog()
{
}

/******************************************************************************
    accept
******************************************************************************/
void
AddPlayerDialog::accept()
{
    // Need to read the path and add it to the selected player struct
    int nSelected = ui.playerCombo->currentIndex();
    QString path = ui.locationEdit->text();

    // Validate fields
    if (nSelected == -1 || path == "")
    {
        LastMessageBox::critical( tr("Field Empty", "An input field is empty"),
            tr("Please select a player and enter a path.") );
        return;
    }

    // Check that file exists
    if (!QFile::exists(path))
    {
        LastMessageBox::critical( tr("Location Error"),
            tr("The file in the location field doesn't exist.") );
        return;
    }

    CPluginInfo& selected = mpPlayerVector->at(nSelected);

    // Need to persist this custom path in registry too
    The::settings().setPluginPlayerPath(selected.GetId(), path);

    selected.SetPlayerPath(path);
    mnSelected = nSelected;

    LOGL(3, "User added custom player path for " << selected.GetName() << ": " << path);

    QDialog::accept();
}

/******************************************************************************
    browse
******************************************************************************/
void
AddPlayerDialog::browse()
{
    QString defPath;

    #ifdef WIN32
        defPath = QString::fromStdString(UnicornUtils::programFilesPath());
    #endif

    QString filter = tr( "Executables" ) + " (*.exe);;" + tr( "All files" ) + " (*.*)";
    QString path = QFileDialog::getOpenFileName(
        this,                                       // parent
        tr("Locate the executable"),                // caption
        defPath,                                    // start dir
        filter);                                    // filter

    ui.locationEdit->setText(path);
}
