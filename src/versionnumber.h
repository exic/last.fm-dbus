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

#ifndef VERSIONNUMBER_H
#define VERSIONNUMBER_H

#include <string>

/*************************************************************************/ /**
    Helper class for dealing with version numbers represented as strings.
******************************************************************************/
class CVersionNumber
{
public:

    /*********************************************************************/ /**
        Ctor
    **************************************************************************/
    CVersionNumber(
        std::string sVersion);

    /*********************************************************************/ /**
        Dtor
    **************************************************************************/
    virtual
    ~CVersionNumber() { }

    /*********************************************************************/ /**
        GetVersion
    **************************************************************************/
    std::string
    GetVersion();

    /*********************************************************************/ /**
        operator==
    **************************************************************************/
    bool
    operator==(
        const CVersionNumber& right) const
    { return Compare(right) == 0; }

    /*********************************************************************/ /**
        operator<
    **************************************************************************/
    bool
    operator<(
        const CVersionNumber& right) const
    { return Compare(right) < 0; }

    /*********************************************************************/ /**
        operator<=
    **************************************************************************/
    bool
    operator<=(
        const CVersionNumber& right) const
    { return Compare(right) <= 0; }

    /*********************************************************************/ /**
        operator>
    **************************************************************************/
    bool
    operator>(
        const CVersionNumber& right) const
    { return Compare(right) > 0; }

    /*********************************************************************/ /**
        operator>=
    **************************************************************************/
    bool
    operator>=(
        const CVersionNumber& right) const
    { return Compare(right) >= 0; }

private:

    /*********************************************************************/ /**
        Parses a dot- or comma-separated string into its constituent integer
        values.
    **************************************************************************/
    void
    ParseVersionString(
        const std::string& sVersion);

    /*********************************************************************/ /**
        Returns a negative number if we are smaller, 0 if we are equal, and
        positive if we are bigger.
    **************************************************************************/
    int
    Compare(
        const CVersionNumber& right) const;

    // Represented backwards, w.x.y.z = [3].[2].[1].[0]
    int anVersion[4];
};

#endif // VERSIONNUMBER_H
