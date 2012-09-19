#include "TrackListAdaptor.h"
#include "DBusExtension.h"

TrackListAdaptor::TrackListAdaptor(QObject *parent, DBusExtension *lfm)
  : lastfm(lfm), QDBusAbstractAdaptor(parent)
{
}

QVariantMap TrackListAdaptor::GetMetadata(int track) {
  if (track != 0) return QVariantMap();
  else return lastfm->getMetadata();
}

int TrackListAdaptor::AddTrack(QString uri, bool playImmediately) {
  //Can't enqueue, but assume they really wanted their new station.
  //if (!playImmediately) return 1;

  lastfm->playStation(uri);
  return 0; //success
}
