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

#include <QtGlobal>
#include <QPainter>
#include <QStyleOption>
#include <QWidget>
#include "winstyleoverrides.h"


void
WinXPStyleOverrides::drawPrimitive( PrimitiveElement pe,
                                    const QStyleOption* opt,
                                    QPainter* p,
                                    const QWidget* w ) const
{
    if ( pe == PE_FrameStatusBar )
    {
        // We don't want no borders
    }
    else
    {
        QWindowsXPStyle::drawPrimitive( pe, opt, p, w );
    }
}

void
WinStyleOverrides::drawPrimitive( PrimitiveElement pe,
                                  const QStyleOption* opt,
                                  QPainter* p,
                                  const QWidget* w) const
{
    if ( pe == PE_FrameStatusBar )
    {
        // We don't want no borders
    }
    else
    {
        QWindowsStyle::drawPrimitive( pe, opt, p, w );
    }
}
