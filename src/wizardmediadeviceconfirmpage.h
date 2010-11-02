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

#ifndef WIZARDMEDIADEVICEASKPAGE_H
#define WIZARDMEDIADEVICEASKPAGE_H

#include "ui_mediaDeviceConfirmWidget.h"

class SimpleWizard;

class WizardMediaDeviceAskPage : public QWidget
{
    Q_OBJECT

    public:
        WizardMediaDeviceAskPage(
            SimpleWizard*   wizard,
            const QString&  info );

        Ui::MediaDeviceConfirmWidget uiWidget;

        friend class SimpleWizard;

    private slots:
        void toggled( bool enabled );
};

#endif // WIZARDBOOTSTRAPPAGE_H
