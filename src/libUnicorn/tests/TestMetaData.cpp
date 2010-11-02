/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
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

#include <QtTest/QtTest>

#include "MooseCommon.h"
#include "LastFmSettings.h"
#include "UnicornCommon.h"
#include "metadata.h"

class TestMetaData : public QObject
{
    Q_OBJECT

    private slots:
        void testTrackTags();
        void testTrackPageUrl();
        void testBuyTrackString();
        void testBuyTrackUrl();
        void testAlbumPicUrl();
        void testAlbumPageUrl();

    private:
        MetaData m_metadata;
};


void TestMetaData::testTrackTags()
{
    QStringList tags;
    tags << "one" << "two" << "three";
    m_metadata.setTrackTags( tags );

    QStringList cmp = m_metadata.trackTags();
    QCOMPARE( tags, cmp );
}


void TestMetaData::testTrackPageUrl()
{
    QString url( "http://testurl" );
    m_metadata.setTrackPageUrl( url );

    QString cmp = m_metadata.trackPageUrl();
    QCOMPARE( url, cmp );
}


void TestMetaData::testBuyTrackString()
{
    QString text( "mybuystring" );
    m_metadata.setBuyTrackString( text );

    QString cmp = m_metadata.buyTrackString();
    QCOMPARE( text, cmp );
}


void TestMetaData::testBuyTrackUrl()
{
    QString url( "http://testurl" );
    m_metadata.setBuyTrackUrl( url );

    QString cmp = m_metadata.buyTrackUrl();
    QCOMPARE( url, cmp );
}


void TestMetaData::testAlbumPicUrl()
{
    QUrl url( "http://testurl" );
    m_metadata.setAlbumPicUrl( url );

    QUrl cmp = m_metadata.albumPicUrl();
    QCOMPARE( url, cmp );
}


void TestMetaData::testAlbumPageUrl()
{
    QString url( "http://testurl" );
    m_metadata.setAlbumPageUrl( url );

    QString cmp = m_metadata.albumPageUrl();
    QCOMPARE( url, cmp );
}


QTEST_MAIN(TestMetaData)
#include "TestMetaData.moc"
