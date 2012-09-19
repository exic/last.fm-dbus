#include "RootAdaptor.h"
#include "DBusExtension.h"
#include "version.h"

QDBusArgument &operator<<(QDBusArgument &argument, const struct mpris_version_t &version) {
  argument.beginStructure();
  argument << version.major << version.minor;
  argument.endStructure();
  return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, struct mpris_version_t &version) {
  argument.beginStructure();
  argument >> version.major >> version.minor;
  argument.endStructure();
  return argument;
}

RootAdaptor::RootAdaptor(QObject *parent, DBusExtension *lfm)
  : lastfm(lfm), QDBusAbstractAdaptor(parent)
{
}

QString RootAdaptor::Identity() {
  return QString("Last.fm ") + LASTFM_CLIENT_VERSION;
}

void RootAdaptor::Quit() {
  lastfm->quit();
}

struct mpris_version_t RootAdaptor::MprisVersion() {
  struct mpris_version_t version = {1, 0};
  return version;
}
