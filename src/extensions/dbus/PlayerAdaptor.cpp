#include "PlayerAdaptor.h"
#include "DBusExtension.h"
#include <iostream>

QDBusArgument &operator<<(QDBusArgument &argument, const struct mpris_status_t &s) {
  argument.beginStructure();
  argument << s.playing << s.random << s.repeat_one << s.repeat_all;
  argument.endStructure();
  return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument,
				struct mpris_status_t &s) {
  argument.beginStructure();
  argument >> s.playing >> s.random >> s.repeat_one >> s.repeat_all;
  argument.endStructure();
  return argument;
}

PlayerAdaptor::PlayerAdaptor(QObject *parent, DBusExtension *lfm)
  : lastfm(lfm), QDBusAbstractAdaptor(parent)
{
}

void PlayerAdaptor::Next() {
  lastfm->skip();
}

void PlayerAdaptor::Prev() {
}

void PlayerAdaptor::Pause() {
}

void PlayerAdaptor::Stop() {
  lastfm->stop();
}

void PlayerAdaptor::Play() {
  lastfm->resumeStation();
}

void PlayerAdaptor::Repeat(bool) {
}

struct mpris_status_t PlayerAdaptor::GetStatus() {
  return lastfm->getStatus();
}

QVariantMap PlayerAdaptor::GetMetadata() {
  return lastfm->getMetadata();
}

int PlayerAdaptor::GetCaps() {
  return (1 | // can go next
	  1 << 3 | // can play
	  1 << 5 // can provide metadata
	  );
}

void PlayerAdaptor::VolumeSet(int vol) {
  lastfm->setVolume(vol);
}

int PlayerAdaptor::VolumeGet() {
  return 0;
}

void PlayerAdaptor::PositionSet(int) {
}

int PlayerAdaptor::PositionGet() {
  return 0;
}
