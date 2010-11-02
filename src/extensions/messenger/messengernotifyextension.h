/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
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

#ifndef MESSENGERNOTIFY_EXTENSION_H
#define MESSENGERNOTIFY_EXTENSION_H

#include "interfaces/ExtensionInterface.h"

#include "metadata.h"

#include "ui_settingsdialog_messenger.h"

class MessengerNotifyExtension : public ExtensionInterface
{
    Q_OBJECT
    Q_INTERFACES( ExtensionInterface )

    public:
        MessengerNotifyExtension();
        virtual ~MessengerNotifyExtension() { };

        virtual QString name() const;
        virtual QString version() const;
        virtual QString tabCaption() const { return "Messenger"; }

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

    public slots:
        void onAppEvent( int, const QVariant& );

    signals:
        void settingsChanged();

    protected:
        bool isEnabled();
        void notifyWaiting();
        
        void sendToMessenger( MetaData metaData, bool enable );

        Ui::SettingsDialogMessenger ui;

    private:
        void initSettingsPanel();

        QWidget* m_settingsPanel;
        QWidget* m_parent;

        QSettings* m_config;

        QPixmap m_icon;

		MetaData m_cachedMetadata;

        const bool m_enabledByDefault;
};

#endif
