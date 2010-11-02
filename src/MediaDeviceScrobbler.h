/***************************************************************************
 *   Copyright 2005 - 2008 Last.fm Ltd.                                    *
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

#ifndef MEDIA_DEVICE_CONFIRM_DIALOG_H
#define MEDIA_DEVICE_CONFIRM_DIALOG_H

#include <QPixmap>
#include <QDialog>
#include "libUnicorn/TrackInfo.h"

#include "ui_MediaDeviceConfirmDialog.h"

/** @author Christian Muehlhaeuser <chris@last.fm>
  * @maintainer Max Howell <max@last.fm> 
  * @contributor Erik Jaelevik <erik@last.fm>
  */

class IPodScrobbler
{
public:
    IPodScrobbler( const QString& username, QWidget* parent );
    
    /** if you set the tracks, we will not try to find all tracks for this user
      * for you */
    void setTracks( QList<TrackInfo> tracks ) { m_tracks = tracks; }
    
    /** there is also a moose setting the user can set for this
      * currently we always confirm on linux */
    void setAlwaysConfirm( bool b ) { m_confirm = b; }
    
    /** a confirm dialog will be shown if the scrobbles are deemed insane */
    void exec();
    
private:
    bool scrobble();
    
    QString m_username;
    QWidget* m_parent;
    QList<TrackInfo> m_tracks;
    bool m_confirm;
};


class MediaDeviceConfirmDialog : public QDialog
{
    Q_OBJECT

public:
    MediaDeviceConfirmDialog( const QList<TrackInfo>& tracks, 
                              const QString& username, 
                              QWidget* parent );

    QList<TrackInfo> tracks() const;

public slots:
    void toggleChecked();

private:
    void setupUi();
    void addTracksToView();

    virtual void showEvent( QShowEvent* );

    Ui::MediaDeviceConfirmDialog ui;

    QString const m_username;
    QList<TrackInfo> const m_tracks;
};

#endif // MEDIADEVICECONFIRMDIALOG_H
