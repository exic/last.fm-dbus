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

#include "updateinfogetter.h"
#include "MooseCommon.h"
#include "UnicornCommon.h"
#include "lastfmapplication.h"
#include "CachedHttp.h"
#include "LastFmSettings.h"
#include "logger.h"
#include "WebService/Request.h"

#include <QDomDocument>

#include <sstream>

using namespace std;


/******************************************************************************
    UpdateInfoGetter
******************************************************************************/
CUpdateInfoGetter::CUpdateInfoGetter()
        : mExtApp( 0 )
        , mExtPlugins( 0 )
        , mHttp( 0 )
        , mGetId( 0 )
        , mbCancelled( false )
{
    //mDowner.Attach(this);
/*
    connect(&mHttp, SIGNAL(requestFinished(int, bool)),
            this,   SLOT(downloadFinished(int, bool)));

    connect(&mHttp, SIGNAL(dataReadProgress(int, int)),
            this,   SIGNAL(progressMade(int, int)));

    connect(&mHttp, SIGNAL(stateChanged(int)),
            this,   SLOT(statusChanged(int)));
*/
}

/******************************************************************************
    ~UpdateInfoGetter
******************************************************************************/
CUpdateInfoGetter::~CUpdateInfoGetter()
{
    //mDowner.Detach(this);
}

/******************************************************************************
    GetAppInfo
******************************************************************************/
void
CUpdateInfoGetter::GetUpdateInfo(
    CComponentInfo*           appInfo,
    std::vector<CPluginInfo>* vecPluginInfo,
    bool                      bForceRefresh)
{
    mbCancelled = false;

    if (bForceRefresh || mApp.GetName() == "")
    {
        mApp.Clear();
        mPlugins.clear();

        DownloadInfo();
        mExtApp = appInfo;
        mExtPlugins = vecPluginInfo;
    }
    else
    {
        // Copy our cached stuff
        *appInfo = mApp;
        *vecPluginInfo = mPlugins;
        emit updateInfoDone(false, "");
    }
}

/******************************************************************************
    GetPluginInfo
******************************************************************************/
/*
void
CUpdateInfoGetter::GetPluginInfo(
    std::vector<CPluginInfo>& vecInfo,
    bool                      bForceRefresh)
{
    mbCancelled = false;

    if (bForceRefresh || mPlugins.size() == 0)
    {
        DownloadInfo();
        mExtPlugins = &vecInfo;
    }
    else
    {
        // Copy our cached plugins
        vecInfo = mPlugins;
        emit pluginInfoDone(false, "");
    }
}
*/
/******************************************************************************
    Cancel
******************************************************************************/
void
CUpdateInfoGetter::Cancel()
{
    mbCancelled = true;

    if ( mHttp != NULL )
    {
        mHttp->abort();
    }
}

/******************************************************************************
    DownloadInfo
******************************************************************************/
void
CUpdateInfoGetter::DownloadInfo()
{
    #ifdef WIN32
        QString platform = "win";
    #elif defined Q_WS_X11
        QString platform = "linux";
    #else
        QString platform = "mac";
    #endif

    QString version = The::settings().version();
    QString user = The::settings().currentUsername();
    QString host = Request::baseHost();
    LastFmApplication* app = qobject_cast<LastFmApplication*>( QApplication::instance() );

    Q_ASSERT( !version.isEmpty() && !user.isEmpty()  && !host.isEmpty() );

    QString path = QString( "/ass/upgrade.xml.php?platform=%1&version=%2&lang=%3&user=%4" )
        .arg( platform )
        .arg( version )
        .arg( app->languageCode() )
        .arg( QString( QUrl::toPercentEncoding( user ) ) );

    // Must use a pointer here or it won't work (bug that happened on Spencer's laptop)
    mHttp = new CachedHttp( host, 80, this );
    mGetId = mHttp->get( path );

    connect(mHttp, SIGNAL(requestFinished(int, bool)),
            this,  SLOT(downloadFinished(int, bool)));

    connect(mHttp, SIGNAL(dataReadProgress(int, int)),
            this,  SIGNAL(progressMade(int, int)));

    connect(mHttp, SIGNAL(stateChanged(int)),
            this,  SLOT(statusChanged(int)));

    connect(mHttp, SIGNAL(dataSendProgress(int, int)),
            this,  SLOT(sendProgress(int, int) ));

    connect(mHttp, SIGNAL(readyRead(const QHttpResponseHeader&)),
            this,  SLOT(readyRead(const QHttpResponseHeader&) ));

    connect(mHttp, SIGNAL(requestStarted(int)),
            this,  SLOT(requestStarted(int)));

    connect(mHttp, SIGNAL(responseHeaderReceived ( const QHttpResponseHeader & )),
            this,  SLOT(responseHeaderReceived ( const QHttpResponseHeader & )));

    LOGL(3, "Requesting update info from: " << host << path );
}

/******************************************************************************
    downloadFinished
******************************************************************************/
void
CUpdateInfoGetter::downloadFinished(
    int  requestId,
    bool error)
{
    if (requestId != mGetId)
    {
        // Ignore the setHost (or any other) requestFinished
        return;
    }

    if (mbCancelled)
    {
        LOG(3, "Download of update/plugin info cancelled. Not emitting any signals.\n");
        mHttp->deleteLater();
        mHttp = NULL;
        return;
    }

    if (error)
    {
        LOG(1, "Download of update/plugin info failed. Error: " <<
              mHttp->error() << " - " << mHttp->errorString() << "\n");

        emit updateInfoDone(true, QString(tr("Info download failed: %1")).
            arg(mHttp->errorString()));
    }
    else
    {
        try
        {
            QDomDocument updateXML;
            updateXML.setContent( mHttp->readAll(), false );

            QDomElement components = updateXML.firstChildElement( "Components" );

            QDomNodeList appList = components.elementsByTagName( "App" );
            QDomNodeList pluginList = components.elementsByTagName( "Plugin" );

            for( int index = 0; index < appList.count(); ++index )
            {
                QDomNode node = appList.item( index );
                PopulateComponent( node, mApp );
            }

            for( int index = 0; index < pluginList.count(); ++index )
            {
                CPluginInfo info;
                QDomNode node = pluginList.item( index );
                PopulatePlugin( node, info );
                mPlugins.push_back( info );
            }

            // Copy app data into object given to us
            if (mExtApp != NULL)
            {
                *mExtApp = mApp;
                mExtApp = NULL;
            }

            // Copy plugin data into vector given to us
            if (mExtPlugins != NULL)
            {
                *mExtPlugins = mPlugins;
                mExtPlugins = NULL;
            }

            emit updateInfoDone(false, "");
        }
        catch (ConnectionException& e)
        {
            emit updateInfoDone(true, QString(tr("Info download failed: %1")).
                arg(e.what()));
        }
    }

    mHttp->deleteLater();
    mHttp = NULL;
}

/******************************************************************************
    PopulateComponent
******************************************************************************/
void
CUpdateInfoGetter::PopulateComponent(
    const QDomNode &appNode,
    CComponentInfo& info)
{

    //TODO: sanity check xml

    //if (vecStrings.size() != 5)
    //{
    //    Q_ASSERT(false);
    //    throw ConnectionException(QT_TR_NOOP("Downloaded update info for "
    //        "component didn't contain the correct number of entries."));
    //}

    info.Clear();

    QDomNamedNodeMap appAttributes = appNode.attributes();

    info.SetName(appAttributes.namedItem( "name" ).nodeValue());

    #ifdef WIN32
        string sPF = UnicornUtils::programFilesPath();
        if (sPF == "")
        {
            // Do our best at faking it. Will at least work some of the time.
            LOG(1, "Couldn't get PF path so trying to fake it.");
            sPF = "C:\\Program Files\\";
        }
        info.SetPath(QString::fromStdString(sPF) +
                     appNode.firstChildElement("Path").text());
    #else // not WIN32
        info.SetPath(appNode.firstChildElement("Path").text());
    #endif // WIN32

    if ( !appNode.firstChildElement("Size").isNull() )
        info.SetSize( appNode.firstChildElement("Size").text().toInt() );
    else
        info.SetSize( 0 );

    info.SetDownloadURL ( appNode.firstChildElement("Url").text() );
    info.SetVersion     ( appAttributes.namedItem( "version" ).nodeValue() );
    info.SetInstallArgs ( appNode.firstChildElement("Args").text()  );

    if( appAttributes.contains( "majorUpgrade" )) {
        info.SetMajorUpgrade( appAttributes.namedItem( "majorUpgrade" ).nodeValue()
                              == "true" );
    }

    if( !appNode.firstChildElement("Description").isNull())
        info.SetDescription( appNode.firstChildElement("Description").text());

    if( !appNode.firstChildElement("Image").isNull())
        info.SetImage( QUrl( appNode.firstChildElement("Image").text()));
}

/******************************************************************************
    PopulatePlugin
******************************************************************************/
void
CUpdateInfoGetter::PopulatePlugin(
    const QDomNode& pluginNode,
    CPluginInfo&    info)
{

    //TODO: sanity check xml
    //if (vecStrings.size() != 9)
    //{
    //    Q_ASSERT(false);
    //    throw ConnectionException(QT_TR_NOOP("Downloaded update info for plugin "
    //        " didn't contain the correct number of entries."));
    //}

    info.Clear();

    ////   0. Player name
    ////   1. Plugin ID
    ////   2. Player exe path relative to %Program Files
    ////   3. Plugin download URL
    ////   4. Plugin version
    ////   5. Plugin install dir relative to player's current dir
    ////   6. Plugin install args if it's a setup exe
    ////   7. Min required player version for this plugin (inclusive range, leave empty for no restriction)
    ////   8. Max required player version for this plugin (inclusive range, leave empty for no restriction)

    QDomNamedNodeMap pluginAttributes = pluginNode.attributes();

    info.SetName        ( pluginAttributes.namedItem( "name" ).nodeValue() );
    info.SetPlayerName  ( pluginAttributes.namedItem( "name" ).nodeValue() );
    info.SetId          ( pluginAttributes.namedItem( "id" ).nodeValue() );

    // Need to check if we already have a custom path in registry first
    QString customPath = The::settings().pluginPlayerPath(info.GetId());
    if (customPath.size() != 0)
    {
        info.SetPlayerPath(customPath);
    }
    else
    {
        #ifdef WIN32
            // Tack on Program Files dir to beginning of default path if on Win
            string sPF = UnicornUtils::programFilesPath();
            if (sPF == "")
            {
                // Do our best at faking it. Will at least work some of the time.
                LOG(1, "Couldn't get PF path so trying to fake it.");
                sPF = "C:\\Program Files\\";
            }
            info.SetPlayerPath(QString::fromStdString(sPF) +
                               pluginNode.firstChildElement("Path").text() );
        #else // not WIN32
            // Just write what we got for now, TODO properly
            info.SetPlayerPath( pluginNode.firstChildElement("Path").text() );
        #endif
    }

    info.SetDownloadURL  ( pluginNode.firstChildElement("Url").text() );
    info.SetVersion      ( pluginAttributes.namedItem( "version" ).nodeValue() );
    info.SetInstallDir   ( pluginNode.firstChildElement( "InstallDir" ).text() );
    info.SetPath         ( pluginNode.firstChildElement( "Path" ).text() );
    info.SetInstallArgs  ( pluginNode.firstChildElement( "Args" ).text() );
    info.SetPlayerVerMin ( pluginNode.firstChildElement( "MinVersion" ).text() );
    info.SetPlayerVerMax ( pluginNode.firstChildElement( "MaxVersion" ).text() );
    info.SetBootstrapType( pluginNode.firstChildElement( "Bootstrap" ).text() );
}

/******************************************************************************
    statusChanged
******************************************************************************/
void
CUpdateInfoGetter::statusChanged(
    int  state)
{
    //LOGL(3, "New QHttp state: " << state);

    QString msg = UnicornUtils::QHttpStateToString(state);
    emit statusChange(msg);
}
