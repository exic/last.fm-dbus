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

#ifndef ShareDialog_H
#define ShareDialog_H

#include "metadata.h"
#include "LastFmSettings.h"

#include "ui_ShareDialog.h"

class ShareDialog : public QDialog
{
    Q_OBJECT

public:
    ShareDialog( QWidget* parent );

    int exec();

    void setSong( const MetaData& );
    void setSong( const Track& );

private:
    Ui::ShareDialog ui;
    MetaData m_metaData;

private slots:
    void onRecipientChanged( const QString& recpipient );
    void onUserChanged( LastFmUserSettings& );
    void onFriendsReturn( class Request* );

private:
    virtual void accept();
};


namespace The
{
    ShareDialog &shareDialog(); /// defined in Container.cpp
}

#endif // ShareDialog_H
