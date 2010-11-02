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

class TestSettings : public QObject
{
    Q_OBJECT

    private slots:
        void initTestCase()
        {
            qApp->setOrganizationName( "Last.fm.test" );
            qApp->setOrganizationDomain( "Last.fm.test" );
            qApp->setApplicationName( "Last.fm.test" );
        }

        void testVersion();
        void testContainerGeometry();
        void testContainerWindowState();
        void testCurrentUsername();

        void testVolume();
        void testSoundCard();
        void testSoundSystem();

        void testBufferManagedAutomatically();
        void testHttpBufferSize();
        void testUseProxy();
        void testProxyHost();
        void testProxyPort();
        void testProxyUser();
        void testProxyPassword();
        void testMusicProxyPort();

        void testFirstRunDone();
        void testBootstrapDone();
        void testDontAsk();
        void testFingerprintUploadUrl();
        void testAppLanguage();
};


void TestSettings::testVersion()
{
    QString ver = "testVersion";
    The::settings().setVersion( ver );

    QString cmp = The::settings().version();
    QCOMPARE( ver, cmp );
}


void TestSettings::testContainerGeometry()
{
    QByteArray geo( "testdata" );
    The::settings().setContainerGeometry( geo );

    QByteArray cmp = The::settings().containerGeometry();
    QCOMPARE( geo, cmp );
}


void TestSettings::testContainerWindowState()
{
    int state = 2;
    The::settings().setContainerWindowState( state );

    int cmp = The::settings().containerWindowState();
    QCOMPARE( state, cmp );
}


void TestSettings::testCurrentUsername()
{
    QString user( "testuser" );
    The::settings().setCurrentUsername( user );

    QString cmp = The::settings().currentUsername();
    QCOMPARE( user, cmp );
}


void TestSettings::testVolume()
{
    int vol = 75;
    The::settings().setVolume( vol );

    int cmp = The::settings().volume();
    QCOMPARE( vol, cmp );
}


void TestSettings::testSoundCard()
{
    int card = 2;
    The::settings().setSoundCard( card );

    int cmp = The::settings().soundCard();
    QCOMPARE( card, cmp );
}


void TestSettings::testSoundSystem()
{
    int sys = 2;
    The::settings().setSoundSystem( sys );

    int cmp = The::settings().soundSystem();
    QCOMPARE( sys, cmp );
}


void TestSettings::testBufferManagedAutomatically()
{
    bool tst = true;
    The::settings().setBufferManagedAutomatically( tst );

    bool cmp = The::settings().isBufferManagedAutomatically();
    QCOMPARE( tst, cmp );
}


void TestSettings::testHttpBufferSize()
{
    int buf = 8192;
    The::settings().setHttpBufferSize( buf );

    int cmp = The::settings().httpBufferSize();
    QCOMPARE( buf, cmp );
}


void TestSettings::testUseProxy()
{
    bool tst = true;
    The::settings().setUseProxy( tst );

    bool cmp = The::settings().isUseProxy();
    QCOMPARE( tst, cmp );
}


void TestSettings::testProxyHost()
{
    QString host( "http://proxyhost" );
    The::settings().setProxyHost( host );

    QString cmp = The::settings().getProxyHost();
    QCOMPARE( host, cmp );
}


void TestSettings::testProxyPort()
{
    int port = 32768;
    The::settings().setProxyPort( port );

    int cmp = The::settings().getProxyPort();
    QCOMPARE( port, cmp );
}


void TestSettings::testProxyUser()
{
    QString user( "proxyuser" );
    The::settings().setProxyUser( user );

    QString cmp = The::settings().getProxyUser();
    QCOMPARE( user, cmp );
}


void TestSettings::testProxyPassword()
{
    QString pw( "proxypassword" );
    The::settings().setProxyPassword( pw );

    QString cmp = The::settings().getProxyPassword();
    QCOMPARE( pw, cmp );
}


void TestSettings::testMusicProxyPort()
{
    int port = 32768;
    The::settings().setMusicProxyPort( port );

    int cmp = The::settings().musicProxyPort();
    QCOMPARE( port, cmp );
}


void TestSettings::testFirstRunDone()
{
    The::settings().setFirstRunDone();

    bool cmp = The::settings().isFirstRun();
    QCOMPARE( false, cmp );
}


void TestSettings::testBootstrapDone()
{
    The::settings().setBootstrapDone();

    bool cmp = The::settings().isBootstrapDone();
    QCOMPARE( true, cmp );
}


void TestSettings::testDontAsk()
{
    QString op = "testOperation";
    bool tst = true;

    The::settings().setDontAsk( op, tst );
    bool cmp = The::settings().isDontAsk( op );
    QCOMPARE( tst, cmp );
}


void TestSettings::testFingerprintUploadUrl()
{
    QString url( "http://fingerprinterurl" );
    The::settings().setFingerprintUploadUrl( url );

    QString cmp = The::settings().fingerprintUploadUrl();
    QCOMPARE( url, cmp );
}


void TestSettings::testAppLanguage()
{
    QString lang = "appLang";
    The::settings().setAppLanguage( lang );

    QString cmp = The::settings().appLanguage();
    QCOMPARE( lang, cmp );
}


QTEST_MAIN(TestSettings)
#include "TestSettings.moc"
