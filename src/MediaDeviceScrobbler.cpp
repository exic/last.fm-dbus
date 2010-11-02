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

#include "MediaDeviceScrobbler.h"

#include "container.h"
#include "MooseCommon.h"
#include "lastfmapplication.h"
#include "logger.h"
#include "Scrobbler-1.2.h"
#include "User.h"

#include <QString>
#include <QDebug>
#include <QDesktopWidget>
#include <QDir>


class TrackSortItem : public QTreeWidgetItem
{
public:
    TrackSortItem() : QTreeWidgetItem()
    {
    }

    bool operator< ( const QTreeWidgetItem & other ) const
    {
        int column = treeWidget() ? treeWidget()->sortColumn() : 0;
        QVariant l = data( column, Qt::DisplayRole );
        QVariant r = other.data( column, Qt::DisplayRole );

        switch ( l.type() )
        {
        case QVariant::DateTime:
            return l.toDateTime() < r.toDateTime();
        case QVariant::Int:
            return l.toInt() < r.toInt();
        case QVariant::String:
        default:
            return l.toString().compare( r.toString(), Qt::CaseInsensitive ) < 0;
        }
        return false;
    }
};


IPodScrobbler::IPodScrobbler( const QString& username, QWidget* parent )
              :m_username( username ),
               m_parent( parent ),
               m_confirm( false )
{}


void
IPodScrobbler::exec()
{
     Q_DEBUG_BLOCK;

     if (!The::settings().isIPodScrobblingEnabled())
         return;

     QStringList deviceCaches;
     const QDateTime now = QDateTime::currentDateTime(); 
     bool insane = false;

     if (m_tracks.isEmpty())
     {
         foreach (QString uid, The::settings().iPodIdsForUsername( m_username ))
         {
             QDir d = Moose::savePath( "devices/" + uid + "/scrobbles" );

             foreach (QString path, d.entryList( QStringList() << "*.xml", QDir::Files ))
             {
                 QString const filePath = d.filePath( path );
                 deviceCaches += filePath;

                 QList<TrackInfo> tracks = ScrobbleCache::tracksForPath( filePath );
                 m_tracks += tracks;
                 
                 foreach (const TrackInfo& t, tracks)
                     if (t.timeStamp() < The::settings().lastIPodScrobbleTime( uid ))
                         insane = true;
             }
         }
    }
    
    QMutableListIterator<TrackInfo> i( m_tracks );
    while (i.hasNext())
        if (Moose::scrobblableStatus( i.next() ) != Moose::OkToScrobble)
            i.remove();

    if (insane) qDebug() << "We're insane!";

    m_confirm |= insane || The::user().settings().isAlwaysConfirmIPodScrobbles();
    bool remove_caches = scrobble();

    foreach (QString uid, The::settings().iPodIdsForUsername( m_username ))
    {
         The::settings().setLastIPodScrobbleTime( uid, now.toTime_t() );
    }

    if (remove_caches)
         foreach (QString path, deviceCaches)
             QFile::remove( path );
}


bool
IPodScrobbler::scrobble()
{
     if (m_tracks.isEmpty())
         return true;
    
     if (m_confirm)
     {
         static bool b = false;
         if (b) 
             return false;

         // if a second request for a dialog comes in, ignore it, since this is
         // a real edge case, and confirm dialogs are off by default, and the 
         // the scrobbles will not be lost, but presented next time the user
         // scrobbles their iPod
         b = true;
         MediaDeviceConfirmDialog d( m_tracks, m_username, m_parent );
         int const r = d.exec();
         b = false;

         if ( r != QDialog::Accepted )
             return true;

         m_tracks = d.tracks();
     }

     // copy tracks playCount() times so we submit the correct number of plays
     QList<TrackInfo> tracksToScrobble;
     foreach (TrackInfo t, m_tracks)
     {
         int const playCount = t.playCount();
         t.setPlayCount( 1 );
         for (int y = 0; y < playCount; ++y)
             tracksToScrobble += t;

         // will add each track (but only once, not playCount() times!) to
         // the recently listened tracks
         emit The::app().event( Event::MediaDeviceTrackScrobbled, QVariant::fromValue( t ) );
     }

     qDebug() << "TRACKS:" << tracksToScrobble.count();

     ScrobbleCache cache( m_username );
     cache.append( tracksToScrobble );

     if (The::scrobbler().canScrobble( m_username ))
     {
         The::scrobbler().scrobble( cache );
     }
     else {
         Scrobbler::Init init;
         init.username = m_username;
         init.password = The::settings().user( m_username ).password();
         init.client_version = The::settings().version();

         The::scrobbler().handshake( init );

         // the cache is automatically scrobbled once the scrobbler is handshaken
     }

     return true;
}


////////////////////////////////////////////////////////////////////////////////

MediaDeviceConfirmDialog::MediaDeviceConfirmDialog( const QList<TrackInfo>& tracks, 
                                                    const QString& username, 
                                                    QWidget* parent )
        : QDialog( parent, Qt::Sheet ),
          m_username( username ),
          m_tracks( tracks )
{
    Q_ASSERT( !m_username.isEmpty() );
    setupUi();
}


void
MediaDeviceConfirmDialog::setupUi()
{
    ui.setupUi( this );

    addTracksToView();
    
    // work around qt 4.3.x bug where on mac the window control buttons look
    // bizarre
    setWindowModality( Qt::ApplicationModal );

////// This section sizes the dialog as best as possible, it is a shame but
    // Qt sizeHints usually ignore the view contents size, so we do it manually.

    //this magnificent hack bought to you by teh mxcl
    // @short allow access to this protected function so we can determine the
    // ideal width for the confirm dialog
    struct NoProtection : QAbstractItemView
    {
        using QAbstractItemView::sizeHintForColumn;
    };

    int w = 0, desiredwidth = 0;
    for (int x = 0; x < ui.tracksWidget->columnCount(); ++x) {
        desiredwidth = reinterpret_cast<NoProtection*>(ui.tracksWidget)->sizeHintForColumn( x );
        w += desiredwidth;
    }

    ui.tracksWidget->setMinimumWidth( w );
    if (m_tracks.count() > 10)
        ui.tracksWidget->setMinimumHeight( ui.tracksWidget->sizeHint().height() * 2 );

    //make us always the right size
    layout()->setSizeConstraint( QLayout::SetMinimumSize );

    int const W = QDesktopWidget().availableGeometry().width();
    if (sizeHint().width() > W)
        ui.tracksWidget->setMinimumWidth( W
                                          - (sizeHint().width() - ui.tracksWidget->width()) 
                                          - 10 /*small aesthetic gap*/ );

    ui.tracksWidget->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

    ui.buttonBox->button( QDialogButtonBox::Ok )->setText( tr("Scrobble") );

///////
    connect( ui.buttonBox, SIGNAL( accepted() ), SLOT( accept() ) );
    connect( ui.buttonBox, SIGNAL( rejected() ), SLOT( reject() ) );
    connect( ui.toggle,    SIGNAL( clicked() ),  SLOT( toggleChecked() ) );

///////
    activateWindow();
}


void
MediaDeviceConfirmDialog::addTracksToView()
{
    QList<QTreeWidgetItem*> items;
    int index = 0;
    int total = 0;
    foreach (TrackInfo t, m_tracks)
    {
        QDateTime dt = QDateTime::fromTime_t( t.timeStamp() );
        TrackSortItem* widget = new TrackSortItem;
        widget->setData( 0, Qt::DisplayRole, t.artist() );
        widget->setData( 1, Qt::DisplayRole, t.track() );
        widget->setData( 2, Qt::DisplayRole, dt );
        widget->setData( 3, Qt::DisplayRole, t.playCount() );

        widget->setFlags( widget->flags() | Qt::ItemIsUserCheckable );
        widget->setCheckState( 0, Qt::Checked );

        widget->setData( 0, Qt::UserRole, index++ );

        items += widget;
        
        total += t.playCount();
    }

    ui.tracksWidget->insertTopLevelItems( 0, items );
    ui.tracksWidget->resizeColumnToContents( 0 );
    ui.tracksWidget->resizeColumnToContents( 1 );
    ui.tracksWidget->resizeColumnToContents( 2 );
    ui.tracksWidget->resizeColumnToContents( 3 );

    ui.tracksWidget->setSortingEnabled( true );
    ui.tracksWidget->sortByColumn( 2, Qt::DescendingOrder );

    QString text = tr( "<p>Last.fm found %n scrobbles on your iPod.",
                       "", total );

    // avoid lawsuits
    if (The::user().name() != m_username)
        text += "<p><b>" + tr("This iPod scrobbles to %1's profile!").arg( m_username );
    
    ui.messageLabel->setText( text );
}


void
MediaDeviceConfirmDialog::toggleChecked()
{
    for (int x = 0; x < ui.tracksWidget->topLevelItemCount(); ++x) 
    {
        QTreeWidgetItem* i = ui.tracksWidget->topLevelItem( x );
        i->setCheckState( 0, i->checkState( 0 ) == Qt::Checked ? Qt::Unchecked : Qt::Checked );
    }
}


QList<TrackInfo>
MediaDeviceConfirmDialog::tracks() const
{
    QList<TrackInfo> tracks;
    for (int x = 0; x < ui.tracksWidget->topLevelItemCount(); ++x)
    {
        QTreeWidgetItem* item = ui.tracksWidget->topLevelItem( x );
        if (item->checkState( 0 ) == Qt::Checked)
        {
            int const index = item->data( 0, Qt::UserRole ).toInt();
            tracks += m_tracks[index];
        }
    }
    return tracks;
}


void
MediaDeviceConfirmDialog::showEvent( QShowEvent* e )
{
    if (!parentWidget()->isActiveWindow()) 
        // qApp alert is bugged, it shouldn't bounce if we're already active!
        //FIXME I couldn't do this->isActiveWindow() as we aren't active yet
        //      so this code breaks if we're the active application on osx
        //      but the window is hidden :(
        qApp->alert( this );
        
    QDialog::showEvent( e );
}
