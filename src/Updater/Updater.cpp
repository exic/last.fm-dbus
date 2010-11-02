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

// Updater.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace std;

bool
ValidateArgs(
    int argc,
    _TCHAR* argv[])
{
    if (argc < 4 || argc > 5) return false;
    if (argv[1][0] != _T('R') && argv[1][0] != _T('C')) return false;
    if (argv[1][0] == _T('R') && argc != 4) return false;
    if (argv[1][0] == _T('C') && argc != 5) return false;
    
    return true;
}

bool
Run(
    _TCHAR* pcPath)
{
    cout << "Will execute " << pcPath << endl;

    // Fire and forget. Installer will launch AS again on completion.
    HINSTANCE h = ShellExecute(
        NULL, _T("open"), pcPath, _T("/SILENT"), _T("C:\\"), SW_SHOWNORMAL);

    cout << "Handle returned " << h << endl;

    // A handle above 32 is returned on success
    return h > reinterpret_cast<HINSTANCE>(32);
}

bool
Copy(
    _TCHAR* pcSrcPath,
    _TCHAR* pcDestPath)
{
    // Not yet implemented
    return true;
}

// Usage:
// Updater R <mutex name> <install exe> 
// Updater C <mutex name> <source> <destination> 
int _tmain(int argc, _TCHAR* argv[])
{
    // Check command line
    if (!ValidateArgs(argc, argv))
    {
        cout << "Invalid arguments" << endl;
        return 1;
    }

    HANDLE hMutex = CreateMutex(NULL, FALSE, argv[2]);
    if (hMutex == NULL)
    {
        cout << "Couldn't create mutex. Error: " << GetLastError() << endl;

        // Sleep for a while until the app has had enough time to shut down
        Sleep(3000);
    }
    else
    {
        // Wait for mutex
        cout << "Waiting on mutex. Time: " << (long)time(NULL) << endl;
        
        DWORD dwWaitResult = WaitForSingleObject(hMutex, INFINITE);
        
        if (dwWaitResult == WAIT_ABANDONED)
        {
            cout << "WAIT_ABANDONED" << endl;
        }
        else if (dwWaitResult == WAIT_OBJECT_0)
        {
            cout << "WAIT_OBJECT_0" << endl;
        }
        else if (dwWaitResult == WAIT_TIMEOUT)
        {
            cout << "WAIT_TIMEOUT" << endl;
        }

        cout << "Mutex released. Time: " << (long)time(NULL) << endl;
    }

    bool bRun = argv[1][0] == _T('R');
    int nRet = 0;
    if (bRun)
    {
        cout << "Will call Run" << endl;

        nRet = Run(argv[3]) ? 0 : 1;
    }
    else
    {
        nRet = Copy(argv[3], argv[4]) ? 0 : 1;
    }

    /*
    cout << "Waiting at getchar" << endl;
    getchar();
    */

	return nRet;
}

