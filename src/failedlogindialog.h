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

#ifndef FAILEDLOGINDIALOG_H
#define FAILEDLOGINDIALOG_H

#include <QPixmap>
#include <QDialog>

#include "ui_failedlogindialog.h"
#include "ui_settingsdialog_connection.h"

class FailedLoginDialog : public QDialog
{
    Q_OBJECT

    public:
        FailedLoginDialog( QWidget *parent = 0 );

    private:
        Ui::FailedLoginDialog ui;
        Ui::SettingsDialogConnection ui_connection;

    private slots:
        void showProxyFrame();
        void accept();
};

#endif // FAILEDLOGINDIALOG_H
