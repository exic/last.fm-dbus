/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
 *      Jono Cole, Last.fm Ltd <jono@last.fm>                              *
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

#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

// we do this because qt 4.3.x uic is b0rked and removes spacing with ifdefs
#ifdef Q_OS_MAC
    #undef Q_OS_MAC
    #include "ui_loginwidget.h"
    #define Q_OS_MAC
#else
    #include "ui_loginwidget.h"
#endif

#include "WebService/fwd.h"


class LoginWidget : public QWidget
{
    Q_OBJECT

    public:

        enum Mode
        {
            ADD_USER,
            LOGIN,
            CHANGE_PASS 
        };

        LoginWidget( QWidget* parent, Mode mode = LOGIN, QString  defaultUser = "" );
        ~LoginWidget();

        void verify();

        QDialog& createDialog();

        void resetWidget( QString defaultUser="" );
        bool detailsChanged();

    signals:
        void verifyResult( bool valid, bool bootstrap );
        void verifySuccess();
        void verifyFail();
        void widgetChanged();

    protected:
        Ui::LoginWidget ui;

    private:
        Mode m_Mode;
        bool m_saveLowerPass;
        bool m_detailsChanged;


        const QString m_badUserError;
        const QString m_badPassError;

        bool verifyLocally();


    public slots:
        void save( bool reconnect = true );

    private slots:
        void userComboChanged( QString user );
        void verifyResult( Request* );
        void onDialogOk();
};

#endif // LOGINWIDGET_H
