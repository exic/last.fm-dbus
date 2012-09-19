#ifndef ROOTADAPTOR_H
#define ROOTADAPTOR_H

#include <QDBusAbstractAdaptor>
#include <QDBusArgument>

class DBusExtension;

struct mpris_version_t {
  ushort major;
  ushort minor;
};
Q_DECLARE_METATYPE(mpris_version_t);

QDBusArgument &operator<<(QDBusArgument &, const struct mpris_version_t &);
const QDBusArgument &operator>>(const QDBusArgument &, struct mpris_version_t &);

class RootAdaptor : public QDBusAbstractAdaptor {
  Q_OBJECT;
  Q_CLASSINFO("D-Bus Interface", "org.freedesktop.MediaPlayer");

public:
  RootAdaptor(QObject *, DBusExtension *);

public Q_SLOTS:
  QString Identity();
  Q_NOREPLY void Quit();
  struct mpris_version_t MprisVersion();

protected:
  DBusExtension *lastfm;
};

#endif
