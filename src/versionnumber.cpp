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

#include "versionnumber.h"
#include <cstdlib>
#include <sstream>

using namespace std;


/******************************************************************************
    CVersionNumber
******************************************************************************/
CVersionNumber::CVersionNumber( string sVersion )
{
    ParseVersionString( sVersion );
}

/******************************************************************************
    GetVersion
******************************************************************************/
string
CVersionNumber::GetVersion()
{
    ostringstream os;
    os << anVersion[3] << "." <<
          anVersion[2] << "." <<
          anVersion[1] << "." <<
          anVersion[0];
    return os.str();
}

/******************************************************************************
    ParseVersionString
******************************************************************************/
void
CVersionNumber::ParseVersionString( const string& sVersion )
{
    string sTmp = sVersion;
    string::size_type idx = 0;
    int i;
    for (i = 3; i >= 0 && idx != string::npos; --i)
    {
        idx = sTmp.find_first_of(".,");
        string sPart;
        if (idx == string::npos)
        {
            sPart = sTmp;
        }
        else
        {
            sPart = sTmp.substr(0, idx);
            sTmp.erase(0, idx + 1);
        }
        anVersion[i] = atoi(sPart.c_str());
    }

    // Pad with zeroes
    while (i >= 0)
    {
        anVersion[i] = 0;
        --i;
    }    

/* Neater version but only works with '.' separators
    istringstream is(sVersion);
    for (int i = 3; i >= 0; --i)
    {
        string sVerPart;
        if (getline(is, sVerPart, '.'))
        {
            // If atoi fails, we get 0 returned which makes sense
            panVersion[i] = atoi(sVerPart.c_str());
        }
        else
        {
            panVersion[i] = 0;
        }
    }
*/

}

/******************************************************************************
    Compare
******************************************************************************/
int
CVersionNumber::Compare( const CVersionNumber& right ) const
{
    // This is clunky but it works
    for (int i = 3; i >= 0; --i)
    {
        // A version component needs to be padded by zeroes on the right to
        // be at the same power of 10 for comparison. Misguided fool!

        int nThisVer = anVersion[i];
        int nRightVer = right.anVersion[i];
/*
        ostringstream osThis;
        ostringstream osRight;
        osThis << nThisVer;
        osRight << nRightVer;
        int nDiff = static_cast<int>(osThis.str().size() - osRight.str().size());
        
        // Difference will be positive if this has more digits than right
        if (nDiff > 0)
        {
            // Pad right
            while (nDiff-- > 0)
            {
                nRightVer *= 10;
            }
        }
        else if (nDiff < 0)
        {
            // Pad this
            nDiff *= -1;
            while (nDiff-- > 0)
            {
                nThisVer *= 10;
            }
        }
*/
        if (nThisVer > nRightVer)
        {
            // Our version higher than right
            return 1;
        }
        else if (nThisVer < nRightVer)
        {
            // Our version lower than right
            return -1;
        }
        else if (nThisVer == nRightVer)
        {
            // Don't know, need to look at next number
        }
    }
    
    // If we get here, versions are equal
    return 0;
}    
