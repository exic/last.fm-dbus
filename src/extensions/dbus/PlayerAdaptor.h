#ifndef PLAYERADAPTOR_H
#define PLAYERADAPTOR_H

#include <QDBusAbstractAdaptor>
#include <QDBusArgument>

class DBusExtension;

struct mpris_status_t {
  int playing;    //0 playing, 1 paused, 2 stopped
  int random;     //0 linear, 1 random
  int repeat_one; //0 go on, 1 repeat current element
  int repeat_all; //0 stop after last, 1 never stop
};
Q_DECLARE_METATYPE(mpris_status_t);

QDBusArgument &operator<<(QDBusArgument &, const struct mpris_status_t &);
const QDBusArgument &operator>>(const QDBusArgument &, struct mpris_status_t &);

class PlayerAdaptor : public QDBusAbstractAdaptor {
  Q_OBJECT;
  Q_CLASSINFO("D-Bus Interface", "org.freedesktop.MediaPlayer");

public:
  PlayerAdaptor(QObject *, DBusExtension *);

public Q_SLOTS:
  void Next();
  void Prev();
  void Pause();
  void Stop();
  void Play();
  void Repeat(bool);
  struct mpris_status_t GetStatus();
  QVariantMap GetMetadata();
  int GetCaps();
  void VolumeSet(int);
  int VolumeGet();
  void PositionSet(int);
  int PositionGet();

 Q_SIGNALS:
  void TrackChange(QVariantMap);
  void StatusChange(struct mpris_status_t);
  void CapsChange(int);

protected:
  DBusExtension *lastfm;  
};

#endif
