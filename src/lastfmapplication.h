/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd. <chris@last.fm>               *
 *      Erik Jaelevik, Last.fm Ltd. <erik@last.fm>                         *
 *      Max Howell, Last.fm Ltd. <max@last.fm>                             *
 *      Jono Cole, Last.fm Ltd <jono@last.fm>                              *
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

#ifndef LAST_FM_APPLICATION_H
#define LAST_FM_APPLICATION_H

#include "RadioEnums.h"
#include "metadata.h"
#include "StationUrl.h"

#include <QApplication>
#include <QTranslator>
#include <QPointer>
#include <QFile>

class CPlayerListener;
class User;
class Radio;
class ScrobblerManager;

/** Usage: connect to qApp->event( int, QVariant ), react to events in your slot
  * Note: if you depend on one object's events being handled before another, you
  * should accept the events at a higher level and control both objects from 
  * there, eg. Container controls all its widgets.
  * NOTE! events do not necessarily dictate state
  * NOTE: some events will always follow each other, see each enum value  for 
  * specification */
namespace Event
{
    enum App
    {
        /** happen just after startup, and for each user switch
          * State could be anything, although radio will not be playing */
        UserChanged,

        /** user object has handshaken with the radio, you'll not get this
          * before UserChanged */
        UserHandshaken,

        /** playback has started, data is the playing track 
          * State will be Playing 
          * You will always get a PlaybackEnded event before a non Playback 
          * event is sent, eg. TuningIn, UserChanged */
        PlaybackStarted,

        /** playback continues, data is the new track
          * State will be playing */
        TrackChanged,

        /** the application object requests metadata for all tracks that are played
          * the currentTrack() function will from this point onwards contain the 
          * data for the artist or track */
        ArtistMetaDataAvailable,
        TrackMetaDataAvailable,

        /** playback has ceased, this will not be sent for transitions, eg
          * buffering, retuning, radio playlist fetching, etc.
          * State will be stopped */
        PlaybackEnded,

        /** a player that we listen to paused
          * event will be followed by unpaused or playbackended, even in the case
          * of user-switching occuring or whatever
          * State will be paused */
        PlaybackPaused,
        /** state will be playing */
        PlaybackUnpaused,

        /** eg. Radio http buffer is empty, we are rebuffering 
          * you'll either get Unstalled, TrackChanged or Ended after this
          * State: TODO
          */
        PlaybackStalled,
        /** eg. Radio http rebuffering complete, playback has resumed */
        PlaybackUnstalled,

        /** PlaybackStarted will follow, unless an error occurs, in which case 
          * PlaybackEnded will be sent, despite the lack of playback, this allows
          * you to keep the UI in the right state */
        TuningIn,

        /** Note: scrobble submission will not be sent until the end of the
          * track */
        ScrobblePointReached,

        /** Will get emitted every time a mediadevice track gets scrobbled */
        MediaDeviceTrackScrobbled,

        TypeMax /** leave at end of enum, kthxbai */
    };
}


namespace State
{
    // All states can turn into other states
    // This is KEY there is no implied order
    // Do not add a state if it may have some implied dependence or order
    // instead make an Event
    //NOTE some state transitions may cause multiple events to be emitted
    // eg stopped -> paused will cause a PlaybackStarted, then PlaybackPaused to
    // be emitted
    enum App
    {
        Stopped,
        TuningIn,
        Playing,
        Paused,

        TypeMax // leave here pls, kthxbai
    };
}


class LastFmApplication : public QApplication
{
    Q_OBJECT

public:
    LastFmApplication( int& argc, char** argv );
    ~LastFmApplication();

    void setUser( const QString& username );

    QString languageCode() const { return m_lang.left( 2 ); }
    MetaData currentTrack() const { return m_currentTrack; }
    State::App state() const { return m_state; }

    CPlayerListener& listener() const { return *m_listener; }
    User& user() const { return *m_user; }
    Radio& radio() const { return *m_radio; }
    ScrobblerManager& scrobbler() const { return *m_scrobbler; }

public slots:
    // Switch the app to a different language
    void setLanguage( QString langCode );
    void onBootstrapReady( QString userName, QString pluginId );

signals:
    void endSession(); // emitted when Windows shuts the app down

    /** see the Event::Type enum, data is corresponding data described in the enum 
      * connect to this for all application level events, ie playback changes and 
      * scrobble point notification */
    void event( int event, const QVariant& data = QVariant() );

protected:
    #ifdef WIN32
    virtual bool winEventFilter( MSG* msg, long* result );
    #endif

private slots:
    void init();
    void fetchMetaData();
    void onListenerNewSong( const TrackInfo&, bool eriks_started_hack );
    void onScrobblePointReached( const TrackInfo& );
    void onListenerException( const QString& message );
    void onRadioStateChanged( RadioState );
    void onControlConnect();
    void onControlRequest();
    void onAppEvent( int event, const QVariant& );
    void onRequestReturned( class Request* request );
    void onScrobblerStatusUpdate( int, const QVariant& );
    void onFingerprintQueryDone( TrackInfo, bool fullFpRequested );
    void onNormanRequestDone( Request* r );
    void onPlaybackEndedTimerTimeout();

    void onProxyTestComplete( bool proxySet, WebRequestResultCode, bool authProxyTimerComplete = false );
    void onAuthenticatedProxyTimerTimeout();

private:
    void initTranslator();
    void registerMetaTypes();
    void loadExtensions();

    void parseCommand( const QString& request );
    void setState( State::App );

private:
    bool m_endSessionEmitted;
    bool m_handshaked; //FIXME remove, instead make User object aware if its services are ready
    QString m_lang;

    class User* m_user;
    class Container* m_container;
    class CPlayerListener* m_listener;
    class QTcpServer* m_control;
    class ScrobblerManager* m_scrobbler;
    class Radio* m_radio;
    class FingerprintCollector* m_fpCollector;
    class FingerprintQueryer* m_fpQueryer;
    class FrikkinNormanRequest* m_activeNorman;

    QPointer<class ArtistMetaDataRequest> m_activeArtistReq;
    QPointer<class TrackMetaDataRequest> m_activeTrackReq;

    MetaData m_currentTrack;
    State::App m_state;

    StationUrl m_preloadStation;

    QTranslator m_translatorApp;
    QTranslator m_translatorQt;

    bool m_proxyTestDone;
	QFile m_pidFile;
		
    class QTimer* m_playbackEndedTimer;
    
    bool m_extensionsLoaded;

    friend class DiagnosticsDialog;
    friend class IPodScrobbler; // needs access to emitting event
};


namespace The
{
    inline LastFmApplication& app() { return *(LastFmApplication*)qApp; }
}

#endif // LASTFMAPPLICATION_H
