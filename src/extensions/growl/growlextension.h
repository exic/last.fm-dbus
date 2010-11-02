/***************************************************************************
 *   Copyright 2007-2008 Last.fm Ltd.                     		           *
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

#ifndef GROWL_EXTENSION_H
#define GROWL_EXTENSION_H

#include "interfaces/ExtensionInterface.h"


/** @author Philipp Maihart <phil@last.fm>
  * @contributor Max Howell <max@last.fm>
  */

class GrowlNotifyExtension : public ExtensionInterface
{
    Q_OBJECT
    Q_INTERFACES( ExtensionInterface )

public:
    GrowlNotifyExtension();

    virtual QString name() const { return "Growl Notifier Extension"; }
    virtual QString version() const { return "1.2.0"; }
    virtual QWidget* owner() { return m_parent; }
    virtual void setOwner( QWidget* parent ) { m_parent = parent; setParent( parent ); }

private slots:
    void onAppEvent( int e, const QVariant& data );

private:
    QWidget* m_parent;
};

#endif
