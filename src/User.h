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

#ifndef LASTFM_USER_H
#define LASTFM_USER_H

#include <QObject>

class Scrobbler;
class Radio;
class LastFmUserSettings;
class LastFmApplication;


class User : public QObject
{
    Q_OBJECT

    // the code is neater, and it suits the design to friend the App object 
    friend class LastFmApplication;

    /** creation of the user object automatically logs all services on */
    User( const QString& name, LastFmApplication* parent );

    /** null user object */
    User( LastFmApplication* parent );

    void shutdownThenDelete();

public:
    QString name() const { return m_name; }

    LastFmUserSettings& settings() const { return *m_settings; }

    bool isSubscriber() const { return m_isSubscriber; }

private:
    QString const m_name;
    LastFmUserSettings* m_settings;
    bool m_isSubscriber;
};


namespace The
{
    User& user(); //defined in LastFmApplication
}

#endif
