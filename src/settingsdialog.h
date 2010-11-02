/***************************************************************************
 *   Copyright (C) 2005 - 2008 by                                          *
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

#ifndef __SETTINGSDIALOG_H__
#define __SETTINGSDIALOG_H__

#include "interfaces/ExtensionInterface.h"

#include "ui_settingsdialog.h"
#include "ui_settingsdialog_account.h"
#include "ui_settingsdialog_radio.h"
#include "ui_settingsdialog_scrobbling.h"
#include "ui_settingsdialog_connection.h"
#include "ui_settingsdialog_mediadevices.h"

class LoginWidget;

class SettingsDialog : public QDialog
{
    Q_OBJECT

    public:
        SettingsDialog( QWidget *parent = 0 );

        void addExtension( ExtensionInterface* ext );

        // not virtual unfortunately :(
        int exec( int startPage = 0 );

    public slots:
        void applyPressed();

    private slots:
        void configChanged();
        void pageSwitched( int currentRow );
        void clearCache();
        void verifiedAccount( bool verified, bool bootstrap );

        void pageSaved( int page );
        void onOkClicked();

        void clearUserIPodAssociations();

    private:
        void loadExtensions();

        void populateAccount();
        void populateRadio();
        void populateScrobbling();
        void populateConnection();
        void populateMediaDevices();

        void saveAccount();
        void saveRadio();
        void saveScrobbling();
        void saveConnection();
        void saveMediaDevices();

        Ui::SettingsDialog ui;
        Ui::SettingsDialogAccount ui_account;
        Ui::SettingsDialogRadio ui_radio;
        Ui::SettingsDialogScrobbling ui_scrobbling;
        Ui::SettingsDialogConnection ui_connection;
        Ui::SettingsDialogMediaDevices ui_mediadevices;

        LoginWidget* m_loginWidget;

        QList<ExtensionInterface*> m_extensions;
        QSet<int> m_pagesToSave;

        QString originalUsername;
        QString originalPassword;

        QString originalProxyHost;
        QString originalProxyUsername;
        QString originalProxyPassword;
        int originalProxyPort;
        bool originalProxyUsage;

        int originalSoundCard;
        int originalSoundSystem;

        bool m_reconnect;
        bool m_reAudio;
        bool m_populating;
        bool m_closeAfterVerification;
};

#endif
