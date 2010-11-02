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

#ifndef CLEANER_H
#define CLEANER_H

#include <QStringList>
#include <QDir>

/**
 * @author <erik@last.fm>
 *
 * Small tool for clearing out user-space settings for the Last.fm client
 * on Windows.
 *
 * Because of Vista's security design, we can't clear user settings from
 * an admin-run uninstaller, so need this separate exe that we run in user-
 * space to clean the installation.
 *
 * The locations it clears are HKEY_CURRENT_USER/Software/Last.fm/Client
 * in the registry and CSIDL_LOCAL_APPDATA, e.g. C:\<username>\AppData\Local\
 * Last.fm\Client on the hard drive.
 */
class Cleaner
{
public:
    Cleaner( const QStringList& argv );

    // Returns true on success
    bool
    clean();

private:
    void
    cleanRegistry();
    
    bool
    cleanFiles();
};

#endif // CLEANER_H
