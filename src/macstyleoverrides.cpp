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

#include <QPainter>
#include <QStyleOption>
#include <QWidget>
#include "macstyleoverrides.h"


void
MacStyleOverrides::drawControl( ControlElement pe,
                                const QStyleOption* opt,
                                QPainter* p,
                                const QWidget* w ) const
{
    if ( pe == CE_Splitter )
    {
        p->setPen( QColor( 0x8c, 0x8c, 0x8c ) );
        p->drawLine( opt->rect.topLeft(), opt->rect.bottomLeft() );
    }
    else
        QMacStyle::drawControl( pe, opt, p, w );
}


void
MacStyleOverrides::drawPrimitive( PrimitiveElement pe,
                                  const QStyleOption* opt,
                                  QPainter* p,
                                  const QWidget* w ) const
{
    if ( pe != PE_FrameStatusBar )
        QMacStyle::drawPrimitive( pe, opt, p, w );
}

