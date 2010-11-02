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

#include "stdafx.h"
#include "KillProcess.h"

using namespace std;

bool
ValidateArgs(
    int argc,
    _TCHAR* argv[])
{
    return argc == 2;
}

// Usage:
// killer <process name>
int _tmain( int argc, _TCHAR* argv[] )
{
    string usage = "Usage:\n\tkiller <process name>";

    // Check command line
    if ( !ValidateArgs( argc, argv ) )
    {
        cout << usage << endl;
        return 1;
    }

    CKillProcessHelper killer;
    bool success = killer.KillProcess( argv[1] );

	return success ? 0 : 1;
}

