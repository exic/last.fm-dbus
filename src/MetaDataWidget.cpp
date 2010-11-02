/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
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

#include "MetaDataWidget.h"

#include "container.h"
#include "MooseCommon.h"
#include "Settings.h"
#include "CachedHttp.h"
#include "UnicornCommon.h"
#include "logger.h"
#include "SpinnerLabel.h"
#include "WebService/Request.h"

#include <QPixmap>
#include <QPainter>

using namespace MooseUtils;

#ifndef Q_WS_X11
    //keeps the watermark image rooted at the bottom right of the widget
    #define STATIONARY_WATERMARK
#endif

// These are pixel sizes
#ifdef Q_WS_MAC
    static const int k_trackFontSize = 18;
    static const int k_artistFontSize = 14;
    static const int k_standardFontSize = 12;
#else
    static const int k_trackFontSize = 17;
    static const int k_artistFontSize = 15;
    static const int k_standardFontSize = 11;
#endif

static const QColor k_greyFontColour = QColor( 0x98, 0x98, 0x98 );
static const QColor k_userActionFontColour = QColor( 0x8e, 0x9e, 0xb3 );
static const QColor k_notListeningFontColour = QColor( 0xAF, 0xAF, 0xAF );

static const int k_marginTop = 22;
static const int k_marginLeft = 22;
static const int k_marginRight = 22;
static const int k_marginBottom = 22;

static const int k_column = 16; // width between images and text
static const int k_row = 30; // height between track and artist sections

static const int k_spaceAfterCaption = 4; // between colon and label


MetaDataWidget::MetaDataWidget( QWidget* parent )
    : QStackedWidget( parent )
{
    m_urlBase = "http://" + UnicornUtils::localizedHostName( The::settings().appLanguage() );
    m_coverArtistLoader = new CachedHttp( this );
    m_coverAlbumLoader = new CachedHttp( this );

    connect( m_coverArtistLoader, SIGNAL( dataAvailable( QByteArray ) ),
             this,                SLOT( coverArtistLoaded( QByteArray ) ) );
    connect( m_coverAlbumLoader, SIGNAL( dataAvailable( QByteArray ) ),
             this,               SLOT( coverAlbumLoaded( QByteArray ) ) );

    m_tuning_in_timer = new QTimer( this );
    m_tuning_in_timer->setInterval( 6000 );
    m_tuning_in_timer->setSingleShot( true );
    connect( m_tuning_in_timer, SIGNAL( timeout() ), SLOT( onDelayedTuningIn() ) );

    #ifndef STATIONARY_WATERMARK
    // Watermark
    WatermarkWidget* scrollwidget = new WatermarkWidget( this );
    scrollwidget->setWatermark( dataPath( "watermark.png" ) );
    #else
    QWidget* scrollwidget = new QWidget( this );
    #endif

    ui.setupUi( scrollwidget );
    scrollwidget->ensurePolished();

    // Set default font on everything as it doesn't work on the Mac otherwise
    QFont defFont = scrollwidget->font();

    qDebug() << "Detected default font:" << defFont.toString();

    defFont.setPixelSize( k_standardFontSize );
    ui.buyTrackLabel->setFont( defFont );
    ui.releaseDateLabel->setFont( defFont );
    ui.recordLabelLabel->setFont( defFont );
    ui.totalCaption->setFont( defFont );
    ui.numTracksLabel->setFont( defFont );
    ui.releasedCaption->setFont( defFont );
    ui.buyAlbumLabel->setFont( defFont );
    ui.wikiLabel->setFont( defFont );
    ui.wikiLink->setFont( defFont );
    ui.tagNoTagsLabel->setFont( defFont );
    ui.tagLink->setFont( defFont );

    // Track font
    defFont.setPixelSize( k_trackFontSize );
    defFont.setBold( true );
    ui.trackLabel->setFont( defFont );

    // Artist, album & about font
    defFont.setPixelSize( k_artistFontSize );
    ui.byCaption->setFont( defFont );
    ui.artistLabel->setFont( defFont );
    ui.albumLabel->setFont( defFont );
    ui.aboutLabel->setFont( defFont );

    // Num listeners font
    defFont = scrollwidget->font();
    defFont.setBold( true );
    defFont.setPixelSize( k_standardFontSize );
    QPalette defPal = scrollwidget->palette();
    defPal.setColor( QPalette::Text, k_greyFontColour );
    ui.numListenersLabel->setFont( defFont );
    ui.numListenersLabel->setPalette( defPal );

    // Item types for drag labels
    ui.trackLabel->setItemType( UnicornEnums::ItemTrack );
    ui.artistLabel->setItemType( UnicornEnums::ItemArtist );
    ui.albumLabel->setItemType( UnicornEnums::ItemAlbum );
    ui.aboutLabel->setItemType( UnicornEnums::ItemArtist );
    ui.tagsLabel->setItemType( UnicornEnums::ItemTag );
    ui.similarLabel->setItemType( UnicornEnums::ItemArtist );
    ui.topFansLabel->setItemType( UnicornEnums::ItemUser );

    // No word wrap on one-liners
    ui.trackLabel->setWordWrap( false );
    ui.artistLabel->setWordWrap( false );
    ui.buyTrackLabel->setWordWrap( false );
    ui.albumLabel->setWordWrap( false );
    ui.releaseDateLabel->setWordWrap( false );
    ui.recordLabelLabel->setWordWrap( false );
    ui.numTracksLabel->setWordWrap( false );
    ui.buyAlbumLabel->setWordWrap( false );
    ui.aboutLabel->setWordWrap( false );
    ui.numListenersLabel->setWordWrap( false );
    ui.wikiLink->setWordWrap( false );

    // Set up comma-separated fields with headers
    defFont.setPixelSize( k_standardFontSize );
    ui.tagsLabel->setHeader( tr( "Tags:" ), defFont );
    ui.similarLabel->setHeader( tr( "Similar artists:" ), defFont );
    ui.topFansLabel->setHeader( tr( "Top listeners on Last.fm:" ), defFont );
    ui.tagsLabel->setCommaSeparated( true );
    ui.similarLabel->setCommaSeparated( true );
    ui.topFansLabel->setCommaSeparated( true );

    ui.tagNoTagsLabel->setText( tr( "No one has tagged this artist yet." ) );
    ui.tagLink->setText( tr( "Tag this artist..." ) );

    // Hand cursor on image hover
    ui.artistPic->setHoverCursor( QCursor( Qt::PointingHandCursor ) );
    ui.albumPic->setHoverCursor( QCursor( Qt::PointingHandCursor ) );

    // Load icons
    QPixmap buyTrack( dataPath( "icons/buy_track.png" ) );
    QPixmap buyAlbum( dataPath( "icons/buy_album.png" ) );

    // Set up web links
    ui.buyTrackIcon->setPixmap( buyTrack );
    ui.buyTrackIcon->setHoverCursor( Qt::PointingHandCursor );
    ui.buyTrackLabel->setHighlightedColor( QColor( 0, 0, 0 ) );
    ui.buyTrackLabel->setSelectedColor( QColor( 100, 100, 100 ) );

    ui.buyAlbumIcon->setPixmap( buyAlbum );
    ui.buyAlbumIcon->setHoverCursor( Qt::PointingHandCursor );
    ui.buyAlbumLabel->setHighlightedColor( QColor( 0, 0, 0 ) );
    ui.buyAlbumLabel->setSelectedColor( QColor( 100, 100, 100 ) );

    ui.recordLabelLabel->setHighlightedColor( QColor( 0, 0, 0 ) );
    ui.recordLabelLabel->setSelectedColor( QColor( 100, 100, 100 ) );

    ui.writeWikiButton->setPixmap( dataPath( "buttons/action_edit.png" ) );
    ui.writeWikiButton->setPixmapDown( dataPath( "buttons/action_edit_down.png" ) );
    ui.writeWikiButton->setPixmapHover( dataPath( "buttons/action_edit_hover.png" ) );
    ui.writeWikiButton->hide();

    ui.tagButton->setPixmap( dataPath( "buttons/action_tag.png" ) );
    ui.tagButton->setPixmapDown( dataPath( "buttons/action_tag_down.png" ) );
    ui.tagButton->setPixmapHover( dataPath( "buttons/action_tag_hover.png" ) );

    ui.wikiLink->setHighlightedColor( QColor( 0, 0, 0 ) );
    ui.wikiLink->setSelectedColor( QColor( 100, 100, 100 ) );

    ui.tagLink->setHighlightedColor( k_userActionFontColour );
    ui.tagLink->setSelectedColor( QColor( 100, 100, 100 ) );

    // Not listening screen
    WatermarkWidget* notPlaying = new WatermarkWidget( this );
    ui_notPlaying.setupUi( notPlaying );
    notPlaying->setWatermark( dataPath( "watermark.png" ) );

    QPixmap logo( dataPath( "logo.png" ) );
    ui_notPlaying.logoLabel->setPixmap( logo );

    QPalette p = ui_notPlaying.messageLabel->palette();
    p.setColor( QPalette::WindowText, k_notListeningFontColour );
    p.setColor( QPalette::Text, k_notListeningFontColour );
    ui_notPlaying.messageLabel->setPalette( p );

    // Scroll area
    m_scrollArea = new MetaDataScrollArea( this );

    p = m_scrollArea->palette();
    p.setColor( QPalette::Window, Qt::white );
    m_scrollArea->setPalette( p );
    m_scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    m_scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    m_scrollArea->setFrameStyle( QFrame::NoFrame );
    m_scrollArea->setWidgetResizable( true );
    m_scrollArea->setWidget( scrollwidget );

    QStackedWidget::addWidget( m_scrollArea );
    QStackedWidget::addWidget( notPlaying );

    displayNotListening(); // needed for jp build

    applyMarginAndSpacing();

//////
    connect( ui.writeWikiButton, SIGNAL( clicked() ), ui.wikiLink, SLOT( openURL() ) );

    connect( ui.tagButton, SIGNAL( clicked() ),        SIGNAL( tagButtonClicked() ) );
    connect( ui.tagLink,   SIGNAL( leftClickedURL() ), SIGNAL( tagButtonClicked() ) );

    connect( ui.trackLabel,         SIGNAL( urlHovered( QUrl ) ), SIGNAL( urlHovered( QUrl ) ) );
    connect( ui.artistLabel,        SIGNAL( urlHovered( QUrl ) ), SIGNAL( urlHovered( QUrl ) ) );
    connect( ui.albumLabel,         SIGNAL( urlHovered( QUrl ) ), SIGNAL( urlHovered( QUrl ) ) );
    connect( ui.aboutLabel,         SIGNAL( urlHovered( QUrl ) ), SIGNAL( urlHovered( QUrl ) ) );
    connect( ui.tagsLabel,          SIGNAL( urlHovered( QUrl ) ), SIGNAL( urlHovered( QUrl ) ) );
    connect( ui.similarLabel,       SIGNAL( urlHovered( QUrl ) ), SIGNAL( urlHovered( QUrl ) ) );
    connect( ui.topFansLabel,       SIGNAL( urlHovered( QUrl ) ), SIGNAL( urlHovered( QUrl ) ) );
    connect( ui.artistPic,          SIGNAL( urlHovered( QUrl ) ), SIGNAL( urlHovered( QUrl ) ) );
    connect( ui.albumPic,           SIGNAL( urlHovered( QUrl ) ), SIGNAL( urlHovered( QUrl ) ) );
    connect( ui.buyTrackIcon,       SIGNAL( urlHovered( QUrl ) ), SIGNAL( urlHovered( QUrl ) ) );
    connect( ui.buyAlbumIcon,       SIGNAL( urlHovered( QUrl ) ), SIGNAL( urlHovered( QUrl ) ) );

    connect( ui.recordLabelLabel,   SIGNAL( enteredURL( QUrl ) ), SIGNAL( urlHovered( QUrl ) ) );
    connect( ui.buyTrackLabel,      SIGNAL( enteredURL( QUrl ) ), SIGNAL( urlHovered( QUrl ) ) );
    connect( ui.buyAlbumLabel,      SIGNAL( enteredURL( QUrl ) ), SIGNAL( urlHovered( QUrl ) ) );
    connect( ui.wikiLink,           SIGNAL( enteredURL( QUrl ) ), SIGNAL( urlHovered( QUrl ) ) );

    connect( ui.recordLabelLabel,   SIGNAL( leftURL() ), SLOT( urlLeft() ) );
    connect( ui.buyTrackLabel,      SIGNAL( leftURL() ), SLOT( urlLeft() ) );
    connect( ui.buyAlbumLabel,      SIGNAL( leftURL() ), SLOT( urlLeft() ) );
    connect( ui.wikiLink,           SIGNAL( leftURL() ), SLOT( urlLeft() ) );

    clear();

    foreach ( QLabel *l, scrollwidget->findChildren<QLabel*>() )
        l->setTextInteractionFlags( Qt::TextSelectableByMouse );

    #ifdef STATIONARY_WATERMARK
    // for stationary watermark painted on viewport()
    foreach ( QWidget* w, m_scrollArea->findChildren<QWidget*>() )
        w->setAutoFillBackground( false );
    #endif
}


void
MetaDataWidget::applyMarginAndSpacing()
{
    int xImage = k_marginLeft;
    int xText = k_marginLeft + ui.albumPic->width() + k_column;

    int yImageUpper = k_marginTop;
    int yTextUpper = k_marginTop;

    int yImageLower = k_marginTop + ui.albumPic->height() + k_row;
    int yTextLower = k_marginTop + ui.albumPic->height() + k_row - 4;

    int xImgOffset = xImage - ui.albumPic->geometry().x();
    int yImgUpperOffset = yImageUpper - ui.albumPic->geometry().y();
    translateItem( ui.albumPic, xImgOffset, yImgUpperOffset );

    int yImgLowerOffset = yImageLower - ui.artistPic->geometry().y();
    translateItem( ui.artistPic, xImgOffset, yImgLowerOffset );

    int xTextOffset = xText - ui.trackLabel->geometry().x();
    int yUpperTextOffset = yTextUpper - ui.trackLabel->geometry().y();
    translateItem( ui.trackLabel, xTextOffset, yUpperTextOffset );
    translateItem( ui.byCaption, xTextOffset, yUpperTextOffset );
    translateItem( ui.artistLabel, xTextOffset, yUpperTextOffset );
    translateItem( ui.buyTrackIcon, xTextOffset, yUpperTextOffset );
    translateItem( ui.buyTrackLabel, xTextOffset, yUpperTextOffset );
    translateItem( ui.albumLabel, xTextOffset, yUpperTextOffset );
    translateItem( ui.releasedCaption, xTextOffset, yUpperTextOffset );
    translateItem( ui.releaseDateLabel, xTextOffset, yUpperTextOffset );
    translateItem( ui.recordLabelLabel, xTextOffset, yUpperTextOffset );
    translateItem( ui.totalCaption, xTextOffset, yUpperTextOffset );
    translateItem( ui.numTracksLabel, xTextOffset, yUpperTextOffset );
    translateItem( ui.buyAlbumIcon, xTextOffset, yUpperTextOffset );
    translateItem( ui.buyAlbumLabel, xTextOffset, yUpperTextOffset );

    int yLowerTextOffset = yTextLower - ui.aboutLabel->geometry().y();
    translateItem( ui.aboutLabel, xTextOffset, yLowerTextOffset );
    translateItem( ui.numListenersLabel, xTextOffset, yLowerTextOffset );
    translateItem( ui.wikiLabel, xTextOffset, yLowerTextOffset );
    translateItem( ui.writeWikiButton, xTextOffset, yLowerTextOffset );
    translateItem( ui.wikiLink, xTextOffset, yLowerTextOffset );
    translateItem( ui.tagsLabel, xTextOffset, yLowerTextOffset );
    translateItem( ui.tagNoTagsLabel, xTextOffset, yLowerTextOffset );
    translateItem( ui.tagButton, xTextOffset, yLowerTextOffset );
    translateItem( ui.tagLink, xTextOffset, yLowerTextOffset );
    translateItem( ui.similarLabel, xTextOffset, yLowerTextOffset );
    translateItem( ui.topFansLabel, xTextOffset, yLowerTextOffset );

    m_wikiLinkReadPos = ui.wikiLink->pos();
    m_wikiLinkWritePos = ui.wikiLink->pos() + QPoint( 25, 0 );

    // Set captions to their actual size and move succeeding labels to their
    // correct x pos (changes with language)
    adjustLabelWidth( ui.byCaption, -1, false );
    adjustLabelWidth( ui.releasedCaption, -1, false );
    adjustLabelWidth( ui.totalCaption, -1, false );

    if ( ui.byCaption->text().isEmpty() )
    {
        // Some languages can't have a "by", they just write the artist name
        ui.artistLabel->move(
            ui.trackLabel->geometry().x(),
            ui.artistLabel->geometry().y() );
    }
    else
    {
        ui.artistLabel->move(
            ui.byCaption->geometry().right() + ( k_spaceAfterCaption / 2 ),
            ui.artistLabel->geometry().y() );
    }
    ui.releaseDateLabel->move(
        ui.releasedCaption->geometry().right() + k_spaceAfterCaption,
        ui.releaseDateLabel->geometry().y() );

    ui.recordLabelLabel->move(
        ui.releaseDateLabel->geometry().right() + k_spaceAfterCaption,
        ui.releaseDateLabel->geometry().y() );

    ui.numTracksLabel->move(
        ui.totalCaption->geometry().right() + k_spaceAfterCaption,
        ui.numTracksLabel->geometry().y() );
}


void
MetaDataWidget::translateItem( QWidget* item, int xOffset, int yOffset )
{
    item->move( item->geometry().x() + xOffset, item->geometry().y() + yOffset );
}


void
MetaDataWidget::clear()
{
    clearArtist();
    clearAlbum();
    clearTrack();

    ui.byCaption->hide();
}


void
MetaDataWidget::clearArtist()
{
    ui.artistLabel->clearText();
    ui.aboutLabel->clearText();
    ui.numListenersLabel->clear();
    ui.wikiLabel->clear();
    ui.writeWikiButton->hide();
    ui.wikiLink->clear();
    ui.tagsLabel->clear();
    ui.tagNoTagsLabel->hide();
    ui.tagLink->hide();
    ui.tagButton->hide();

    ui.similarLabel->clear();
    ui.topFansLabel->clear();

    setDefaultArtistPic();
    ui.artistPic->hide();
    ui.artistPic->setUrl( QUrl() );

    adjustArtistLabels();
}


void
MetaDataWidget::clearAlbum()
{
    ui.albumLabel->clearText();
    ui.byCaption->hide();
    ui.albumLabel->hide();
    ui.releasedCaption->hide();
    ui.releaseDateLabel->clear();
    ui.recordLabelLabel->clear();
    ui.recordLabelLabel->hide();
    ui.totalCaption->hide();
    ui.numTracksLabel->clear();
    ui.buyAlbumIcon->hide();
    ui.buyAlbumIcon->setUrl( QUrl() );
    ui.buyAlbumLabel->clear();
    ui.buyAlbumLabel->setURL( QUrl() );
    QCursor cursor( Qt::ArrowCursor );
    ui.buyAlbumLabel->setUseCursor( true, &cursor );

    setDefaultAlbumCover();
    ui.albumPic->hide();
    ui.albumPic->setUrl ( QUrl() );

    adjustTrackLabels();
}


void
MetaDataWidget::clearTrack()
{
    ui.trackLabel->clearText();
    ui.recordLabelLabel->setText( "" );
    ui.recordLabelLabel->hide();
    ui.buyTrackIcon->hide();
    ui.buyTrackIcon->setUrl( QUrl() );
    ui.buyTrackLabel->clear();
    ui.buyTrackLabel->setURL( QUrl() );
    QCursor cursor( Qt::ArrowCursor );
    ui.buyTrackLabel->setUseCursor( true, &cursor );

    adjustTrackLabels();
}


void
MetaDataWidget::setDefaultAlbumCover()
{
    QPixmap image( dataPath( "no_cover.gif" ) );
    if ( image.isNull() )
        return;

    image = image.scaled( 137, 137, Qt::KeepAspectRatio, Qt::SmoothTransformation );
    ui.albumPic->setPixmap( image );
    ui.albumPic->show();
}


void
MetaDataWidget::setDefaultArtistPic()
{
    QPixmap image( dataPath( "no_artist.gif" ) );
    if ( image.isNull() )
        return;

    image = image.scaled( 137, 137, Qt::KeepAspectRatio, Qt::SmoothTransformation );
    ui.artistPic->setPixmap( image );
    ui.artistPic->show();
}


void
MetaDataWidget::displayTuningIn()
{
    ui_notPlaying.spinnerLabel->setVisible( true );
    ui_notPlaying.messageLabel->setText( tr( "Starting station..." ) );
    QStackedWidget::setCurrentIndex( 1 );

    m_tuning_in_timer->start();
}


void
MetaDataWidget::displayNotListening()
{
    ui_notPlaying.spinnerLabel->setVisible( false );
    ui_notPlaying.messageLabel->setText(
        tr( "Start listening in your media player\nor tune in to free radio" ) );
    setCurrentIndex( 1 );
}


void
MetaDataWidget::onDelayedTuningIn()
{
    ui_notPlaying.messageLabel->setText( tr( "Starting station (any second now)..." ) );
}


void
MetaDataWidget::setTrackInfo( const TrackInfo& track )
{
    // Only clear artist fields if the artist differs to avoid flicker
    // last.fm has case insensitive artist names so this works fine
    QString newText = track.artist().toLower();
    QString oldText = ui.artistLabel->text().toLower();
    if ( newText != oldText )
    {
        // TODO: this still flickers in the case of moderation
        clearArtist();
    }

    // Only clear album fields if the album differs to avoid flicker
    newText = track.album().toLower();
    oldText = ui.albumLabel->text().toLower();
    if ( newText != oldText )
    {
        // TODO: this still flickers in the case of moderation
        clearAlbum();
    }

    clearTrack();

    ui.artistLabel->setText( track.artist() );
    ui.trackLabel->setText( track.track() );

    // If we're expecting metadata later, don't show album label until then
    // to avoid a disjointed appearance.
    ui.albumLabel->setText( track.album() );
    ui.albumLabel->setVisible( The::currentUser().isMetaDataEnabled() );

    // EJ: commented out the spinners as they caused a weird hang when skipping a lot
/*
    if ( !ui.artistPic->movie() )
    {
        QMovie* movie = new QMovie( dataPath( "progress.mng" ) );
        movie->start();
        ui.artistPic->setMovie( movie );
    }
    ui.artistPic->setAlignment( Qt::AlignCenter );
*/
    ui.artistPic->show();

/*
    if ( !ui.albumPic->movie() )
    {
        QMovie* movie = new QMovie( dataPath( "progress.mng" ) );
        movie->start();
        ui.albumPic->setMovie( movie );
    }
    ui.albumPic->setAlignment( Qt::AlignCenter );
*/
    ui.albumPic->show();

    if ( !track.artist().isEmpty() && !track.track().isEmpty() )
    {
        adjustTopLabels();
        buildHomeBrewedUrls( track ); 
        buildTooltips();
        updateDragData( track ); // this is important since DragLabel::setText() clears the drag-data!

        QStackedWidget::setCurrentIndex( 0 );
        ui.byCaption->show();
        ui.albumPic->show();
    }
    else
    {
        //stop any previous, incompomplete image downloads from being completed and inserted into the metadata
        m_coverAlbumLoader->abort();
        m_coverArtistLoader->abort();

        ui.aboutLabel->setText( "" );
        ui.byCaption->hide();
        ui.albumPic->hide();
        ui.artistPic->hide();
    }
}


void
MetaDataWidget::adjustTopLabels()
{
    adjustLabelWidth( ui.trackLabel );
    int y = adjustLabelWidth( ui.artistLabel );
    y += 17;

    y = adjustLabelWidth( ui.albumLabel, y );

    adjustWidgetSize();
}


void
MetaDataWidget::adjustTrackLabels()
{
    adjustLabelWidth( ui.trackLabel );
    int y = adjustLabelWidth( ui.artistLabel );

    if ( !ui.buyTrackLabel->text().isEmpty() )
    {
        y = adjustLabelWidth( ui.buyTrackLabel );
        moveLabel( ui.buyTrackIcon, y - 2 );
    }
    y += 17;

    y = adjustLabelWidth( ui.albumLabel, y );
    y += 1;

    int dateY = y;
    if ( !ui.releaseDateLabel->text().isEmpty() )
    {
        y = adjustLabelWidth( ui.releaseDateLabel, y );
        moveLabel( ui.releasedCaption, y );
    }

    if ( !ui.recordLabelLabel->text().isEmpty() )
    {
        y = adjustLabelWidth( ui.recordLabelLabel, dateY );
        ui.recordLabelLabel->move( ui.releaseDateLabel->geometry().right() + k_spaceAfterCaption, ui.recordLabelLabel->y() );
    }

    if ( !ui.numTracksLabel->text().isEmpty() )
    {
        y += 0;
        y = adjustLabelWidth( ui.numTracksLabel, y );
        moveLabel( ui.totalCaption, y );
    }

    if ( !ui.buyAlbumLabel->text().isEmpty() )
    {
        y += 0;
        y = adjustLabelWidth( ui.buyAlbumLabel, y );
        moveLabel( ui.buyAlbumIcon, y - 2 );
    }

    adjustWidgetSize();
}


void
MetaDataWidget::adjustArtistLabels()
{
    int y = adjustLabelWidth( ui.aboutLabel ) + 1;

    y = adjustLabelWidth( ui.numListenersLabel, y );
    y += 12;

    if ( !ui.wikiLabel->text().isEmpty() )
    {
        y = adjustLabelSize( ui.wikiLabel, y );
        y += ui.writeWikiButton->isHidden() ? 2 : 6;
    }

    y = adjustLabelWidth( ui.wikiLink, y );
    moveLabel( ui.writeWikiButton, y + 2 );
    y += 10;

    if ( !ui.writeWikiButton->isHidden() )
    {
        y += 4;
    }

    if ( !ui.tagsLabel->isHidden() )
    {
        y = adjustLabelSize( ui.tagsLabel, y );
    }
    else
    {
        y = adjustLabelWidth( ui.tagNoTagsLabel, y );
        y += 4;
        adjustLabelWidth( ui.tagButton, y );
        y = adjustLabelWidth( ui.tagLink, y + 4 );
        y += 4;
    }
    y += 10;

    if ( !ui.similarLabel->text().isEmpty() )
    {
        y = adjustLabelSize( ui.similarLabel, y );
        y += 10;
    }

    if ( !ui.topFansLabel->text().isEmpty() )
    {
        y = adjustLabelSize( ui.topFansLabel, y );
    }

    adjustWidgetSize();
}


int
MetaDataWidget::adjustLabelWidth( QWidget* label, int y, bool adaptToViewport )
{
    QRect geom = label->geometry();

    int neededWidth = label->sizeHint().width();

    if ( adaptToViewport )
    {
        int availWidth = viewWidth() - k_marginLeft - ui.albumPic->width() -
            k_column - k_marginRight;
        availWidth = qMax( 0, availWidth );

        neededWidth = qMin( neededWidth, availWidth );
    }

    geom.setWidth( neededWidth );

    if ( y != -1 )
    {
        geom.moveTop( y );
    }

    label->setGeometry( geom );

    return geom.bottom();
}


int
MetaDataWidget::adjustLabelSize( QWidget* label, int y )
{
    QRect geom = label->geometry();

    int availWidth = viewWidth() - k_marginLeft - ui.albumPic->width()
                     - k_column - k_marginRight;
    availWidth = qMax( 0, availWidth );
    geom.setWidth( availWidth );

    int height = label->heightForWidth( availWidth );
    geom.setHeight( height );

    if ( y != -1 )
    {
        geom.moveTop( y );
    }

    label->setGeometry( geom );

    return geom.bottom();
}


void
MetaDataWidget::moveLabel( QWidget* label, int y )
{
    QRect geom = label->geometry();
    geom.moveBottom( y );
    label->setGeometry( geom );
}


void
MetaDataWidget::adjustWidgetSize()
{
    int widest = 0;
    int w = 0;
    if ( ( w = ui.trackLabel->width() ) > widest ) widest = w;
    if ( ( w = ui.artistLabel->width() ) > widest ) widest = w;
    if ( ( w = ui.buyTrackLabel->width() ) > widest ) widest = w;
    if ( ( w = ui.albumLabel->width() ) > widest ) widest = w;
    if ( ( w = ui.releaseDateLabel->width() ) > widest ) widest = w;
    if ( ( w = ui.numTracksLabel->width() ) > widest ) widest = w;
    if ( ( w = ui.buyAlbumLabel->width() ) > widest ) widest = w;
    if ( ( w = ui.aboutLabel->width() ) > widest ) widest = w;
    if ( ( w = ui.numListenersLabel->width() ) > widest ) widest = w;
    if ( ( w = ui.wikiLabel->width() ) > widest ) widest = w;
    if ( ( w = ui.wikiLink->width() ) > widest ) widest = w;
    if ( ( w = ui.tagsLabel->width() ) > widest ) widest = w;
    if ( ( w = ui.similarLabel->width() ) > widest ) widest = w;
    if ( ( w = ui.topFansLabel->width() ) > widest ) widest = w;

    int lowest = 0;
    if ( !ui.topFansLabel->text().isEmpty() )
        lowest = ui.topFansLabel->geometry().bottom();
    else if ( !ui.similarLabel->text().isEmpty() )
        lowest = ui.similarLabel->geometry().bottom();
    else if ( !ui.tagsLabel->text().isEmpty() )
        lowest = ui.tagsLabel->geometry().bottom();
    else if ( !ui.wikiLink->text().isEmpty() )
        lowest = ui.wikiLink->geometry().bottom();
    else if ( !ui.wikiLabel->text().isEmpty() )
        lowest = ui.wikiLabel->geometry().bottom();
    else if ( !ui.artistPic->isVisible() )
        lowest = ui.artistPic->geometry().bottom();
    else if ( !ui.numListenersLabel->text().isEmpty() )
        lowest = ui.numListenersLabel->geometry().bottom();
    else if ( !ui.aboutLabel->text().isEmpty() )
        lowest = ui.aboutLabel->geometry().bottom();
    else if ( !ui.buyAlbumLabel->text().isEmpty() )
        lowest = ui.buyAlbumLabel->geometry().bottom();
    else if ( !ui.numTracksLabel->text().isEmpty() )
        lowest = ui.numTracksLabel->geometry().bottom();
    else if ( !ui.releaseDateLabel->text().isEmpty() )
        lowest = ui.releaseDateLabel->geometry().bottom();
    else if ( !ui.albumLabel->text().isEmpty() )
        lowest = ui.albumLabel->geometry().bottom();
    else if ( !ui.buyTrackLabel->text().isEmpty() )
        lowest = ui.buyTrackLabel->geometry().bottom();
    else if ( !ui.artistLabel->text().isEmpty() )
        lowest = ui.artistLabel->geometry().bottom();
    else if ( !ui.trackLabel->text().isEmpty() )
        lowest = ui.trackLabel->geometry().bottom();

    int width = ( k_marginLeft + k_marginRight ) + ui.albumPic->width() + k_column + widest;
    int height = lowest + k_marginBottom;

    m_scrollArea->widget()->setMinimumSize( width, height );
}


int
MetaDataWidget::viewWidth()
{
    int viewWidth = m_scrollArea->viewport()->width();

    QScrollBar* vertBar = m_scrollArea->verticalScrollBar();
    if ( vertBar != NULL )
    {
        // Hmm, this returns the wrong value if we haven't had a scrollbar
        // on screen yet. Hardcode for now.
        //viewWidth -= vertBar->width();
        viewWidth -= 17;
    }

    return viewWidth;
}


void
MetaDataWidget::resizeEvent( QResizeEvent* /*event*/ )
{
    adjustTrackLabels();
    adjustArtistLabels();
}


void
MetaDataWidget::buildHomeBrewedUrls( const TrackInfo& track )
{
    QUrl url( m_urlBase );

    url.setPath( "/music/" + UnicornUtils::urlEncodeItem( track.artist() ) );
    ui.artistLabel->setURL( url );
    ui.aboutLabel->setURL( url );
    ui.artistPic->setUrl( url );

    url.setPath( "/music/" +
                 UnicornUtils::urlEncodeItem( track.artist() ) + "/_/" +
                 UnicornUtils::urlEncodeItem( track.track() ) );
    ui.trackLabel->setURL( url );

    url.setPath( "/music/" +
                 UnicornUtils::urlEncodeItem( track.artist() ) + "/" +
                 UnicornUtils::urlEncodeItem( track.album() ) );
    ui.albumLabel->setURL( url );
    ui.albumPic->setUrl( url );
}


void
MetaDataWidget::buildTooltips()
{
    QString tipBase = tr( "Drag to tag/share %1" );
    QString tipBaseTitle = tr( "Drag to tag/share \"%1\"" );
    QString tip;

    QString track = ui.trackLabel->text();
    if ( !track.isEmpty() )
    {
        tip = tipBaseTitle.arg( track );
        ui.trackLabel->setItemTooltip( 0, tip );
    }

    QString artist = ui.artistLabel->text();
    if ( !artist.isEmpty() )
    {
        tip = tipBase.arg( artist );
        ui.artistLabel->setItemTooltip( 0, tip );
        ui.aboutLabel->setItemTooltip( 0, tip );
    }

    QString album = ui.albumLabel->text();
    if ( !album.isEmpty() )
    {
        tip = tipBaseTitle.arg( album );
        ui.albumLabel->setItemTooltip( 0, tip );
    }

    QStringList similar = ui.similarLabel->items();
    for ( int i = 0; i < similar.count(); ++i )
    {
        QString entry = similar.at( i );
        tip = tipBase.arg( entry );
        ui.similarLabel->setItemTooltip( i, tip );
    }
}


void
MetaDataWidget::updateDragData( const TrackInfo& track )
{
    // TODO: need to include line-wrapping draglabels too
    QHash<QString, QString> data;
    data.insert( "artist", track.artist() );
    data.insert( "album", track.album() );
    data.insert( "track", track.track() );

    ui.artistPic->setItemData( data );
    ui.albumPic->setItemData( data );

    ui.artistLabel->setItemData( 0, data );
    ui.aboutLabel->setItemData( 0, data );
    ui.albumLabel->setItemData( 0, data );
    ui.trackLabel->setItemData( 0, data );
}


void
MetaDataWidget::setArtistMetaData( const MetaData& metadata )
{
    ui.artistLabel->setText( metadata.artist() );
    ui.aboutLabel->setText( metadata.artist() );

    if ( !metadata.artistPageUrl().isEmpty() )
    {
        // don't fill in empty URLs as we set preliminary ones in buildHomeBrewedUrls()
        ui.artistLabel->setURL( metadata.artistPageUrl() );
        ui.aboutLabel->setURL( metadata.artistPageUrl() );
        ui.artistPic->setUrl( metadata.artistPageUrl() );
    }

    if ( qApp->arguments().contains( "--sanity" ) )
    {
        ui.numListenersLabel->setText( tr( "%L1 listeners on Last.fm" ).arg( metadata.numListeners() ) );
    }
    else {
        ui.numListenersLabel->setText( tr( "%L1 plays scrobbled on Last.fm" ).arg( metadata.numPlays() ) );
    }

    populateWiki( metadata );

    // Have to set defFont on every item or it won't work properly on the Mac
    QFont defFont = m_scrollArea->widget()->font();
    defFont.setPixelSize( k_standardFontSize );
    if ( metadata.artistTags().isEmpty() )
    {
        ui.tagNoTagsLabel->show();
        ui.tagButton->show();
        ui.tagLink->show();

        ui.tagsLabel->hide();
    }
    else
    {
        ui.tagsLabel->setItems( metadata.artistTags().mid( 0, 5 ) );
        for ( int i = 0; i < ui.tagsLabel->items().count(); i++ )
        {
            ui.tagsLabel->setItemURL( i,
                    m_urlBase + "/tag/" +
                    UnicornUtils::urlEncodeItem( ui.tagsLabel->items().at( i ) ) );
            ui.tagsLabel->setItemFont( i, defFont );
        }
        ui.tagsLabel->show();
    }

    ui.similarLabel->setItems( metadata.similarArtists().mid( 0, 5 ) );
    for ( int i = 0; i < ui.similarLabel->items().count(); i++ )
    {
        ui.similarLabel->setItemURL( i, m_urlBase + "/music/" +
            UnicornUtils::urlEncodeItem( ui.similarLabel->items().at( i ) ) );
        ui.similarLabel->setItemFont( i, defFont );
    }

    ui.topFansLabel->setItems( metadata.topFans().mid( 0, 5 ) );
    for ( int i = 0; i < ui.topFansLabel->items().count(); i++ )
    {
        ui.topFansLabel->setItemURL( i, m_urlBase + "/user/" +
            UnicornUtils::urlEncodeItem( ui.topFansLabel->items().at( i ) ) );
        ui.topFansLabel->setItemFont( i, defFont );
    }

    // Download artist picture
    QUrl url = metadata.artistPicUrl();
    if ( !url.isEmpty() )
    {
        downloadPic( m_coverArtistLoader, url );
    }
    else
        setDefaultArtistPic();

    updateDragData( metadata );
    adjustArtistLabels();

    // Need to do this here too as this is included in adjustTrackLabels only
    // since it belongs to the track section.
    adjustLabelWidth( ui.artistLabel );

    buildTooltips();
}


void
MetaDataWidget::setTrackMetaData( const MetaData& metadata )
{
    // Populate track fields
    ui.trackLabel->setText( metadata.track() );
    if ( !metadata.trackPageUrl().isEmpty() )
    {
        ui.trackLabel->setURL( metadata.trackPageUrl() );
    }

    if ( metadata.isTrackBuyable() )
    {
        ui.buyTrackLabel->setText( metadata.buyTrackString() );
        ui.buyTrackLabel->setURL( metadata.buyTrackUrl() );
        QCursor cursor( Qt::PointingHandCursor );
        ui.buyTrackLabel->setUseCursor( true, &cursor );
        ui.buyTrackLabel->show();

        ui.buyTrackIcon->setUrl( metadata.buyTrackUrl() );
        ui.buyTrackIcon->show();
    }

    // Populate album fields
    if ( !metadata.album().isEmpty() )
    {
        ui.albumLabel->setText( metadata.album() );
        ui.albumLabel->show();
    }
    if ( !metadata.albumPageUrl().isEmpty() )
    {
        ui.albumLabel->setURL( metadata.albumPageUrl() );
        ui.albumPic->setUrl( metadata.albumPageUrl() );
    }
    if ( metadata.isAlbumBuyable() )
    {
        ui.buyAlbumLabel->setText( metadata.buyAlbumString() );
        ui.buyAlbumLabel->setURL( metadata.buyAlbumUrl() );
        QCursor cursor( Qt::PointingHandCursor );
        ui.buyAlbumLabel->setUseCursor( true, &cursor );
        ui.buyAlbumLabel->show();

        ui.buyAlbumIcon->setUrl( metadata.buyAlbumUrl() );
        ui.buyAlbumIcon->show();
    }

    QDate date = metadata.releaseDate();
    if ( date.isValid() )
    {
        QString dateStr = date.toString( "d MMM yyyy" );

        if ( !metadata.label().isEmpty() )
        {
            dateStr +=
            #ifdef Q_WS_MAC
                QChar( 8471 ) + QString( " " );
            #else
                tr( " on" );
            #endif

            ui.recordLabelLabel->setText( metadata.label() );
            ui.recordLabelLabel->setURL( metadata.labelUrl() );
            ui.recordLabelLabel->show();
        }

        ui.releaseDateLabel->setText( dateStr );
        ui.releaseDateLabel->show();

        ui.releasedCaption->show();
    }

    if ( metadata.numTracks() > 0 )
    {
        ui.numTracksLabel->setText( tr( "%1 tracks" ).arg( metadata.numTracks() ) );
        ui.numTracksLabel->show();
        ui.totalCaption->show();
    }

    // Download album cover
    QUrl url = metadata.albumPicUrl();
    if ( !url.isEmpty() )
    {
        downloadPic( m_coverAlbumLoader, url );
    }
    else
        setDefaultAlbumCover();

    updateDragData( metadata );
    adjustTrackLabels();
    buildTooltips();
}


void
MetaDataWidget::populateWiki( const MetaData& metadata )
{
    QString wikiText = metadata.wiki();

    QString linkText;
    if ( wikiText.isEmpty() )
    {
        wikiText = tr( "We don't have a description for this artist yet, care to help?" );
        linkText = tr( "Write an artist description..." );
        ui.writeWikiButton->show();
        ui.wikiLink->move( m_wikiLinkWritePos );
        ui.wikiLink->setHighlightedColor( k_userActionFontColour );
    }
    else {
        linkText = tr( "Read more..." );
        ui.writeWikiButton->hide();
        ui.wikiLink->move( m_wikiLinkReadPos );
        ui.wikiLink->setHighlightedColor( QColor( 0, 0, 0 ) );
    }

    ui.wikiLabel->setText( wikiText );
    ui.wikiLink->setText( linkText );
    ui.wikiLink->setURL( metadata.wikiPageUrl() );
}


void
MetaDataWidget::downloadPic( CachedHttp* loader, const QUrl& url )
{
    loader->abort(); // no dataAvailable for lingering requests will be sent

    loader->setHost( url.host() );
    if ( !url.encodedQuery().isEmpty() )
    {
        loader->get( url.path() + "?" + url.encodedQuery(), true );
    }
    else
    {
        loader->get( url.path(), true );
    }
}


void
MetaDataWidget::coverArtistLoaded( const QByteArray& to )
{
    //delete ui.artistPic->movie();
    ui.artistPic->setAlignment( Qt::AlignHCenter | Qt::AlignTop );

    if ( !render( to, ui.artistPic ) )
        setDefaultArtistPic();

    ui.artistPic->show();
}


void
MetaDataWidget::coverAlbumLoaded( const QByteArray& to )
{
    //delete ui.albumPic->movie();
    ui.albumPic->setAlignment( Qt::AlignHCenter | Qt::AlignTop );

    if ( !render( to, ui.albumPic ) )
        setDefaultAlbumCover();

    ui.albumPic->show();
}


bool
MetaDataWidget::render( const QByteArray& from, ImageButton* to )
{
    if ( from.size() > 0 && !from.startsWith( "<?xml" ) )
    {
        QPixmap cover;
        bool success = cover.loadFromData( from );
        if ( !success )
        {
            LOGL( 1, "Loading of image from byte array failed." );
        }
        else if ( !cover.isNull() && cover.width() > 0 && cover.height() > 0 )
        {
            cover = cover.scaled( 137, 137, Qt::KeepAspectRatio, Qt::SmoothTransformation );
            to->setPixmap( cover );

            return true;
        }
    }

    return false;
}


void
MetaDataWidget::urlLeft()
{
    emit urlHovered( QUrl() );
}


// Need to override this in order to work out when the widget becomes
// visible, and when we need to request metadata.
void
MetaDataScrollArea::paintEvent( QPaintEvent* event )
{
  #ifndef Q_WS_X11
    static QPixmap watermark( dataPath( "watermark.png" ) );
    QRect const r = parentWidget()->rect();
    QPainter( viewport() ).drawPixmap( r.bottomRight() - watermark.rect().bottomRight(), watermark );
  #else
    QScrollArea::paintEvent( event );
  #endif
}
