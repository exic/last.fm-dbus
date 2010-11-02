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

#ifndef SKYPENOTIFY_EXTENSION_H
#define SKYPENOTIFY_EXTENSION_H

#include "interfaces/ExtensionInterface.h"

#include "metadata.h"

#include "ui_settingsdialog_skype.h"

class SkypeNotifyExtension : public ExtensionInterface
{
    Q_OBJECT
    Q_INTERFACES( ExtensionInterface )

    public:
        SkypeNotifyExtension();
        virtual ~SkypeNotifyExtension() { };

        virtual QString name() const;
        virtual QString version() const;
        virtual QString tabCaption() const { return "Skype"; }

        virtual bool hasGui() { return false; }
        virtual QWidget* gui() { return NULL; }
        virtual bool guiEnabled() { return false; }

        virtual bool hasSettingsPane() { return true; }
        virtual QWidget* settingsPane();
        virtual bool settingsPaneEnabled() { return ( m_settingsPanel != 0 ); }

        virtual void setSettings( QSettings* settings ) { m_config = settings; }
        virtual void populateSettings();
        virtual void saveSettings();

        virtual QPixmap* settingsIcon();

        virtual QWidget* owner() { return m_parent; }
        virtual void setOwner( QWidget* parent ) { m_parent = parent; }

        virtual bool isSkypeAttached() = 0;

    public slots:
        void onAppEvent( int, const QVariant& );

    signals:
        void settingsChanged();

    protected:
        bool isEnabled();
        QString nowPlayingFormat();
        void notifyWaiting();
        
        virtual void sendToSkype( QByteArray utfCmd ) = 0;
        void receiveFromSkype( QByteArray utfResp );

        Ui::SettingsDialogSkype ui;

    private:
        void initSettingsPanel();
        virtual void initSkypeCommunication() = 0;

        QWidget* m_settingsPanel;
        QWidget* m_parent;
        QSettings* m_config;

        QPixmap m_icon;

        MetaData m_cachedMetadata;
        bool m_haveWaiting;

        // The user's mood message prior to enabling our plugin.
        // We restore this again on disabling.
        QString m_savedMoodMessage; 

        // Most recently sent track notification.
        QString m_lastNotification;

        const bool m_enabledByDefault;
        const QString m_defaultFormat;
};

#endif
