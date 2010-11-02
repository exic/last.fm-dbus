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

#ifndef EXTENSION_INTERFACE_H
#define EXTENSION_INTERFACE_H

#include <QWidget>

/// where's the documentation? --mxcl


class ExtensionInterface : public QObject
{
    public:
        virtual ~ExtensionInterface() {}

        virtual QString name() const = 0;
        virtual QString version() const = 0;

        virtual bool hasGui() { return false; }
        virtual QWidget* gui() { return NULL; }
        virtual bool guiEnabled() { return false; }
        virtual QString tabCaption() const { return ""; }

        virtual bool hasSettingsPane() { return false; } //cool
        virtual QWidget* settingsPane() { return NULL; } //that
        virtual bool settingsEnabled() { return false; } //these
        virtual QPixmap* settingsIcon() { return NULL; } //line
        virtual QString settingsCaption() { return ""; } //up :)

        // The host for the extension MUST call this function during initialisation
        // to provide the extension with a valid QSettings object initialised to the
        // correct location.
        virtual void setSettings( class QSettings* ) {}

        /** does this mean the settings object or the settings gui pane? */
        virtual void populateSettings() {}
        virtual void saveSettings() {}

        virtual QWidget* owner() = 0;
        virtual void setOwner( QWidget* parent ) = 0;

        virtual void terminate() {}
};

Q_DECLARE_INTERFACE( ExtensionInterface, "fm.last.Extension.General/1.0" )

#endif
