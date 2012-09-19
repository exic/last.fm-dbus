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

#include <QtPlugin>
#include <QDBusConnection>
#include <QDBusMetaType>
#include "DBusExtension.h"
#include "lastfmapplication.h"

DBusExtension::DBusExtension()
  : ExtensionInterface(), 
    root_holder(new QObject(this)),
    tracklist_holder(new QObject(this)),
    player_holder(new QObject(this)),
    rootadaptor(new RootAdaptor(root_holder, this)),
    tracklistadaptor(new TrackListAdaptor(tracklist_holder, this)),
    playeradaptor(new PlayerAdaptor(player_holder, this)) {

  // This is ugly
  // The plugin can't call back into the app because it isn't linked to its symbols
  // It CAN, however, send signals to the app's slots.

  LastFmApplication * app = (LastFmApplication *)qApp;
  radio = &app->radio();
  connect(this, SIGNAL(s_playStation(QUrl)), radio, SLOT(playStation(QUrl)));
  connect(this, SIGNAL(s_resumeStation()), radio, SLOT(resumeStation()));
  connect(this, SIGNAL(s_stop()), radio, SLOT(stop()));
  connect(this, SIGNAL(s_skip()), radio, SLOT(skip()));
  connect(this, SIGNAL(s_setVolume(int)), radio, SLOT(setVolume(int)));

  connect(qApp, SIGNAL(event(int, QVariant)), this, SLOT(onAppEvent(int, QVariant)));

  qDBusRegisterMetaType<struct mpris_version_t>();
  qDBusRegisterMetaType<struct mpris_status_t>();

  QDBusConnection::sessionBus().registerService("org.mpris.lastfm");
  QDBusConnection::sessionBus().registerObject("/", root_holder);
  QDBusConnection::sessionBus().registerObject("/TrackList", tracklist_holder);
  QDBusConnection::sessionBus().registerObject("/Player", player_holder);

  connect(this, SIGNAL(s_trackChanged(QVariantMap)),
	  playeradaptor, SIGNAL(TrackChange(QVariantMap)));
  connect(this, SIGNAL(s_statusChanged(struct mpris_status_t)),
	  playeradaptor, SIGNAL(StatusChange(struct mpris_status_t)));
  connect(this, SIGNAL(s_trackListChanged(int)),
	  tracklistadaptor, SIGNAL(TrackListChange(int)));
}

void DBusExtension::playStation( QUrl url ) {
  emit s_playStation(url);
  emit s_trackListChanged(1);
}

void DBusExtension::resumeStation() {
  emit s_resumeStation();
}

void DBusExtension::stop() {
  emit s_stop();
}

void DBusExtension::skip() {
  emit s_skip();
}

void DBusExtension::setVolume( int vol ) {
  // Last.fm doesn't update the volume slider,
  // but the change does take effect.
  // Scale is 0-100, just like MPRIS
  emit s_setVolume(vol);
}

void DBusExtension::quit() {
  qApp->quit();
}

void DBusExtension::onAppEvent(int e, QVariant data) {

  switch (e)
    {
    case Event::PlaybackEnded:
      status = {2,0,0,0};
      emit s_statusChanged(status);
      break;

    case Event::PlaybackStarted:
    case Event::TrackChanged:
      status = {0,0,0,0};
      setMetadata(data.value<TrackInfo>());
      emit s_trackChanged(metadata);
      // This is a lie, status doesn't change just because of track change.
      // But last.fm has a bug that prevents PlaybackStarted from being emitted
      emit s_statusChanged(status);
      break;

    default:
      return;
    }
}

void DBusExtension::setMetadata(const TrackInfo &trackinfo) {
  metadata = QVariantMap();

  // This is a tiny cheat, but works because stationUrl is an inline fn.
  // Tempted to rip it out, but MPRIS says location is mandatory
  metadata["location"] = radio->stationUrl();

  // Commented lines are ones that the LastFm client leaves blank right now
  metadata["title" ] = trackinfo.track();
  metadata["artist"] = trackinfo.artist();
  metadata["album" ] = trackinfo.album();
  //metadata["tracknumber"] = (QString)(trackinfo.trackNr());
  metadata["time"] = (uint)(trackinfo.duration());
  //metadata["mb track id"] = trackinfo.mbId();
}

Q_EXPORT_PLUGIN( DBusExtension )
