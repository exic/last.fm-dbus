/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jalevik, Last.fm Ltd <erik@last.fm>                           *
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

#ifndef METADATA_EXTENSION_H
#define METADATA_EXTENSION_H

#include <QScrollArea>
#include <QStackedWidget>

#include "metadata.h"
#include "URLLabel.h"
#include "watermarkwidget.h"
#include "WebService/fwd.h"

#include "ui_MetaDataWidget.h"
#include "ui_MetaDataWidgetTuningIn.h"

class MetaDataScrollArea;
class CachedHttp;


enum MetaDataFetchedState
{
    NothingFetched = 0,
    AlbumPicFetched,
    ArtistPicFetched
};


class MetaDataWidget : public QStackedWidget
{
    Q_OBJECT

    friend class MetaDataScrollArea;
    friend class MetaDataWatermark;

    public:
        MetaDataWidget( QWidget* parent = 0 );

    signals:
        void tagButtonClicked();
        void urlHovered( const QUrl& url );

    public slots:
        /** at first we only have track information, we then request the artist
          * and track metadata separately */
        void setTrackInfo( const TrackInfo& );
        void setArtistMetaData( const MetaData& );
        void setTrackMetaData( const MetaData& );

        /** shows the tuning in display */
        void displayTuningIn();

        /** shows the listen in your media player display */
        void displayNotListening();

        /** shows the not playing screen */
        void clear();

    private:
        void adjustTopLabels();
        void adjustTrackLabels();
        void adjustArtistLabels();
        int adjustLabelWidth( QWidget* label, int y = -1, bool adaptToViewport = true );
        int adjustLabelSize( QWidget* label, int y = -1 );
        void moveLabel( QWidget* label, int y );
        void adjustWidgetSize();
        int viewWidth();

        void applyMarginAndSpacing();
        void translateItem( QWidget* item, int xOffset, int yOffset );

        virtual void resizeEvent( QResizeEvent* event );

        bool isVisible();

        void clearArtist();
        void clearAlbum();
        void clearTrack();

        void setDefaultAlbumCover();
        void setDefaultArtistPic();
        void downloadPic( CachedHttp* loader, const QUrl& url );
        bool render( const QByteArray& from, ImageButton* to );

        void buildHomeBrewedUrls( const TrackInfo& );
        void updateDragData( const TrackInfo& );
        void populateWiki( const MetaData& );

        void buildTooltips();

        void fixDragLabelSizes();
        void resizeBottomSpacers();

        Ui::MetaDataWidget ui;
        Ui::NotPlayingWidget ui_notPlaying;

        MetaDataScrollArea* m_scrollArea;

        CachedHttp* m_coverArtistLoader;
        CachedHttp* m_coverAlbumLoader;

        // Calculated positions of where the wiki link should go depending on
        // whether the Write icon is visible or not.
        QPoint m_wikiLinkReadPos;
        QPoint m_wikiLinkWritePos;

        QString m_urlBase; // eg. http://www.last.fm, http://www.lastfm.fr etc.

        class QTimer *m_tuning_in_timer;

    private slots:
        void coverArtistLoaded( const QByteArray& to );
        void coverAlbumLoaded( const QByteArray& to );

        void urlLeft(); /// no longer hovering over a URLLabel/imageButton

        void onDelayedTuningIn();
};


class MetaDataScrollArea : public QScrollArea
{
public:
    MetaDataScrollArea( MetaDataWidget* parent )
            : QScrollArea( parent )
    {}

protected:
    virtual void resizeEvent( QResizeEvent* event )
    {
        ((MetaDataWidget*)parentWidget())->resizeEvent( event );
        QScrollArea::resizeEvent( event );
    }

    virtual void paintEvent( QPaintEvent * event );
};

#endif
