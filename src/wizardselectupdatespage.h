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

#ifndef WIZARDSELECTUPDATESPAGE_H
#define WIZARDSELECTUPDATESPAGE_H

#include "componentinfo.h"
#include "plugininfo.h"

#include "ui_selectupdateswidget.h"
#include "ui_majorupdate.h"

#include <vector>

class SimpleWizard;

class WizardSelectUpdatesPage : public QWidget
{
    Q_OBJECT

public:

    /*********************************************************************/ /**
        Ctor
    **************************************************************************/
    WizardSelectUpdatesPage(
        SimpleWizard* wizard);

    /*********************************************************************/ /**
        Populates the list box with the player names in the passed vector.
    **************************************************************************/
    bool
    Populate(
        std::vector<CComponentInfo*>& vecUpdatables);

    /*********************************************************************/ /**
        Fills the vector with the entries that are checked.
    **************************************************************************/
    void
    GetChecked(
        std::vector<CComponentInfo*>& vecChecked);
    
    void GetMajorUpdateComponent( std::vector<CComponentInfo*>& vecToUpdate );

public slots:

    /*********************************************************************/ /**
        Enables/disables Next button depending on checked options.
    **************************************************************************/
    void
    pageTouched();

private:

    Ui::SelectUpdatesWidget ui;
    Ui::MajorUpdateDialog majorUpdateUi;
    QDialog majorUpdateDialog;

    SimpleWizard* m_Wizard;

    friend class SimpleWizard;
    
    void MajorUpgrade( QString, QUrl );

    int m_imageRequestId;
    CComponentInfo* m_majorComponent;

private slots:
    void onImageRequestFinished( int, bool error );

};

#endif // WIZARDSELECTUPDATESPAGE_H
