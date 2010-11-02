/***************************************************************************
 *   Copyright (C) 2005 - 2008 by                                          *
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

#include "aboutdialog.h"
#include "MooseCommon.h"
#include "container.h"
#include "logger.h"
#include "LastFmSettings.h"

#include <QMessageBox>
#include <QPainter>
#include <QShortcut>


AboutDialog::AboutDialog( QWidget* parent )
        : QDialog( parent )
{
    ui.setupUi( this );

#ifdef WIN32
    ui.line->setFrameShadow( QFrame::Sunken ); // Want etched, not flat
    m_watermark.load( MooseUtils::dataPath( "about.png" ) );
#endif
#ifdef Q_WS_MAC
    delete ui.line;
    delete ui.okButton;
    delete ui.hboxLayout1;
    m_watermark.load( MooseUtils::dataPath( "about_mac.png" ) );
    QFont f = ui.labelInfo->font();
    f.setPointSize( 12 );
    ui.labelInfo->setFont( f );
    new QShortcut( QKeySequence::Close, this, SLOT(close()) ); //Qt should do this
#endif
#ifdef Q_WS_X11
    m_watermark.load( MooseUtils::dataPath( "about_generic.png" ) );
    ui.okButton->setText( tr("Close") );
#endif

    QString labelText = tr( "Version %1" ).arg( The::settings().version() );
    foreach(QString v, The::settings().allPlugins())
        labelText += '\n' + v;
    labelText += '\n' + tr( "Copyright %1 Last.fm Ltd. (C)", "%1 = Year" ).arg("2010");
    ui.labelInfo->setText( labelText );

    adjustSize();
    setFixedSize( sizeHint() );
    
    new QShortcut( QKeySequence( Qt::Key_M, Qt::Key_L, Qt::Key_I ), this, SLOT(onEasterEgg()) );
}


void
AboutDialog::paintEvent( QPaintEvent* )
{
    QPainter( this ).drawPixmap( 0, 0, m_watermark );
}


void
AboutDialog::onEasterEgg()
{
    QMessageBox::information( this, "Easter Egg",
            "<erik> jono: yes! eriktesttest26 now can haz profile!\n"
            "<jono> yay :-d\n"
            "<jono> jono == happy\n"
            "<max> that evaluates to true I guess\n"
            "<jono> yeah\n"
            "<jono> ++jono.happiness\n"
            "<erik> pretty nasty encapsulation breakage there jono\n"
            "<erik> hide your implementation, man\n"
            "<jono> I'm so happy I can't contain myself ;-)" );
}
