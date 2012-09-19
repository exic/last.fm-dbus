/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Ken Rossato <rossatok@umd.edu>                                     *
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

#ifndef DBUS_EXTENSION_H
#define DBUS_EXTENSION_H

#include <QUrl>
#include "interfaces/ExtensionInterface.h"
#include "libUnicorn/TrackInfo.h"
#include "TrackListAdaptor.h"
#include "RootAdaptor.h"
#include "PlayerAdaptor.h"
#include "Radio.h"

class DBusExtension : public ExtensionInterface
{
    Q_OBJECT;
    Q_INTERFACES( ExtensionInterface );

 public:
    DBusExtension();
    
    virtual QString name() const { return "DBusExtension"; }
    virtual QString version() const { return "2.0"; }
    
    virtual QWidget* owner() { return m_parent; }
    virtual void setOwner( QWidget* parent ) { m_parent = parent; setParent( parent ); }
    
    void playStation( QUrl );
    void resumeStation();
    void stop();
    void skip();
    void setVolume( int );
    void quit();

    QVariantMap           getMetadata() {return metadata;}
    struct mpris_status_t getStatus()   {return status;}

 protected Q_SLOTS:
    void onAppEvent( int e, QVariant data );

 Q_SIGNALS:
    // Signals into Last.fm
    void s_playStation( QUrl );
    void s_resumeStation();
    void s_stop();
    void s_skip();
    void s_setVolume( int );

    // Signals out to DBus
    void s_trackChanged( QVariantMap );
    void s_trackListChanged( int );
    void s_statusChanged(struct mpris_status_t);

 protected:
    void setMetadata(const TrackInfo &);

    QWidget* m_parent;

    QObject *root_holder, *tracklist_holder, *player_holder;

    RootAdaptor *rootadaptor;
    TrackListAdaptor *tracklistadaptor;
    PlayerAdaptor *playeradaptor;

    QVariantMap metadata;
    struct mpris_status_t status;
    Radio *radio;
};

#endif
