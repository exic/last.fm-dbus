#ifndef TRACKLISTADAPTOR_H
#define TRACKLISTADAPTOR_H

#include <QDBusAbstractAdaptor>
#include <QDBusVariant>

class DBusExtension;

class TrackListAdaptor : public QDBusAbstractAdaptor {
  Q_OBJECT;
  Q_CLASSINFO("D-Bus Interface", "org.freedesktop.MediaPlayer");

public:
  TrackListAdaptor(QObject *, DBusExtension *);
				   
public Q_SLOTS:
  QVariantMap GetMetadata(int);
  int GetCurrentTrack() {return 0;}
  int GetLength() {return 1;}
  int AddTrack(QString, bool);
  void DelTrack(int) {}
  void SetLoop(bool) {}
  void SetRandom(bool) {}

Q_SIGNALS:
  void TrackListChange(int);

protected:
  DBusExtension *lastfm;
};

#endif
