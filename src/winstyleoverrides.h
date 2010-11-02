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

#ifndef WINSTYLEOVERRIDES_H
#define WINSTYLEOVERRIDES_H

#ifdef WIN32
#   include <QWindowsXPStyle>
#   include <QWindowsStyle>
#elif defined Q_WS_X11
    // we like this on Linux too, so hack it to work
#   include <QPlastiqueStyle>
#   define QWindowsStyle QPlastiqueStyle
#   define QWindowsXPStyle QPlastiqueStyle
#endif

#ifndef Q_WS_MAC

// Need to have one class for XP and a different for pre-XP

class WinXPStyleOverrides : public QWindowsXPStyle
{
    virtual void drawPrimitive( PrimitiveElement pe,
                                const QStyleOption *opt,
                                QPainter *p,
                                const QWidget *w = 0) const;
};

class WinStyleOverrides : public QWindowsStyle
{
    virtual void drawPrimitive( PrimitiveElement pe,
                                const QStyleOption *opt,
                                QPainter *p,
                                const QWidget *w = 0) const;
};

#endif // !Q_WS_MAC

#endif // WINSTYLEOVERRIDES_H
