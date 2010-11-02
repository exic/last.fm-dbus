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

#include "Cleaner.h"

#include "MooseCommon.h"

#include <QSettings>
#include <QFileInfoList>
#include <QString>
#include <QDebug>

#include <cstdlib> // for system

Cleaner::Cleaner( const QStringList& args )
{
}


bool
Cleaner::clean()
{
    cleanRegistry();
    return cleanFiles();
}


void
Cleaner::cleanRegistry()
{
    QSettings hkcu( "HKEY_CURRENT_USER\\Software\\Last.fm\\Client", QSettings::NativeFormat );
    hkcu.clear();
}


bool
Cleaner::cleanFiles()
{
    QDir userDir( MooseUtils::savePath( "" ) );

    Q_ASSERT( userDir.path().endsWith( "Client" ) || userDir.path().endsWith( "Client/" ) );
    
    // Only works on 2000/XP and upwards, but that's all we support anyway
    QString cmd( "rmdir /S /Q  \"" + QDir::toNativeSeparators( userDir.path() ) + "\"" );
    int retCode = _wsystem( reinterpret_cast<const wchar_t*>( cmd.utf16() ) );

    return retCode == 0;
}


