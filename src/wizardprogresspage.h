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

#ifndef WIZARDPROGRESSPAGE_H
#define WIZARDPROGRESSPAGE_H

#include "wizardinfopage.h"
#include "ui_progresswidget.h"

class SimpleWizard;

class WizardProgressPage : public QWidget
{
    Q_OBJECT

public:
    WizardProgressPage(
        SimpleWizard*   wizard,
        const QString&  info,
        const QString&  detailedInfo);

public slots:
    void
    setProgress(
        int percentage,
        int total);

    void
    setInfo(
        QString info);

signals:
    void
    infoChanged(
        QString info);

    void
    detailedInfoChanged(
        QString info);

private:
    Ui::ProgressWidget uiProgress;

    friend class SimpleWizard;
};

#endif // WIZARDPROGRESSPAGE_H
