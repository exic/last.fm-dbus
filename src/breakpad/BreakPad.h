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

#ifndef NBREAKPAD

#include "BreakpadDllExportMacro.h"
#include <QString>

#ifdef __APPLE__
#   include "external/src/client/mac/handler/exception_handler.h"
#   define HANDLER_ALL 1
#elif defined WIN32
#   include "external/src/client/windows/handler/exception_handler.h"
#elif defined __linux__
#   include "external/src/client/linux/handler/exception_handler.h"
#else
#   define NBREAKPAD
#endif

class BREAKPAD_DLLEXPORT BreakPad : public google_breakpad::ExceptionHandler
{
    const char* m_product_name; // yes! It MUST be const char[]

public:
    BreakPad( const QString &dump_write_dirpath );
    
    ~BreakPad() 
    {}

    void setProductName( const char* s ) { m_product_name = s; };
    const char* productName() const { return m_product_name; }
};
#endif


#undef char
