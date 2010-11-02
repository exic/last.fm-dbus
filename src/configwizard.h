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

#ifndef CONFIGWIZARD_H
#define CONFIGWIZARD_H

#ifdef WIN32
    #include "simplewizard_win.h"
#else
    #include "simplewizard_mac.h"
#endif

#include "wizardinfopage.h"
#include "wizardloginpage.h"
#include "wizardprogresspage.h"
#include "wizardselectpluginpage.h"
#include "wizardbootstrappage.h"
#include "WizardBootstrapSelectorPage.h"
#include "wizardmediadeviceconfirmpage.h"
#include "WizardTwiddlyBootstrapPage.h"
#include "updateinfogetter.h"
#include "autoupdater.h"

#include <vector>



#ifdef WIN32
    typedef SimpleWizardWin BaseWizard;
#else
    typedef SimpleWizardMac BaseWizard;
#endif


class ConfigWizard : public BaseWizard
{
    Q_OBJECT

public:

    enum Mode
    {
        Login,
        Plugin,
        MediaDevice,
        BootStrap
    };

    ConfigWizard(
        QWidget* parent = 0,
        Mode     mode = Login,
        QString  uid = "");

    static bool isActive() { return s_wizardRunning; }

protected:

    virtual QWidget*
    createPage(
        int index);

    virtual QString
    headerForPage(
        int index);

    virtual void
    reject();


public slots:
    virtual int
    exec();


protected slots:

    virtual void
    backButtonClicked();

    virtual void
    nextButtonClicked();


private:

    /*********************************************************************/ /**
        Start downloading plugins. Returns true if plugin downloaded started,
        false otherwise.
    **************************************************************************/
    bool downloadPlugins();
    
    bool twiddlyBootstrapRequired();

    WizardInfoPage*              m_page1;
    WizardLoginPage*             m_page2;
    WizardInfoPage*              m_page3;
    WizardProgressPage*          m_page4;
    WizardSelectPluginPage*      m_page5;
    WizardProgressPage*          m_page6;
    WizardBootstrapSelectorPage* m_page7;
    WizardBootstrapPage*         m_page9;
    WizardInfoPage*              m_page10;
    
    WizardTwiddlyBootstrapPage*  m_pageTwiddly; //I'm abandoning the stupid numbering scheme

    Mode m_mode;

    // Set if the wizard is starting from a later page than 1
    int m_pageOffset;

    CUpdateInfoGetter           m_infoGetter;
    CAutoUpdater                m_updater;
    class AbstractBootstrapper* m_bootstrapper;
    QString                     m_uid;
    bool                        m_bootstrapAllowed;
    bool                        m_didBootstrap;
    static bool                 s_wizardRunning;

    std::vector<CPluginInfo> mAvailPlugins;

    std::vector<CComponentInfo*> mDownloadTasks;

    QString m_introHeader;
    QString m_introInfo;
    QString m_notAllowedInfo;
    QString m_loginHeader;
    QString m_detectExplainHeader;
    QString m_detectExplainInfo;
    QString m_detectHeader;
    QString m_detectInfo;
    QString m_selectHeader;
    QString m_downloadHeader;
    QString m_doneHeader;
    QString m_bootstrapHeader;
    QString m_bootstrapInfo;
    QString m_bootstrapQuestion;
    QString m_mediaDeviceHeader;
    QString m_mediaDeviceQuestion;
    QString m_doneInfoFirstRun;
    QString m_doneInfoPlugin;
    QString m_doneInfoClientBootstrapExtra;
    QString m_doneInfoPluginBootstrapExtra;

    int m_bootstrapStatus;
    
private slots:

    /*********************************************************************/ /**
        Called by LoginPage when it has finished verifying the user details
        against the server.
    **************************************************************************/
    void
    loginVerified(
        bool valid,
        bool bootstrap);

    /*********************************************************************/ /**
        Called by web service when it's finished with the handshake.
    **************************************************************************/
    void
    handshakeFinished();

    /*********************************************************************/ /**
        Called by UpdateInfoGetter when the plugin info has finished
        downloading.
    **************************************************************************/
    void
    pluginInfoDone(
        bool error,
        QString errorMsg);

    /*********************************************************************/ /**
        Called by AutoUpdater when the plugins are downloaded and installed.
    **************************************************************************/
    void
    pluginDownloadDone(
        bool error,
        QString errorMsg);

    void
    onBootstrapDone(
        int /* MediaDevices::BootstrapStatus */ status );
        
    void
    onTwiddlyBootstrapDone();
};


#ifdef Q_OS_MAC
    #define TWIDDLY_PATH The::settings().path() + "/../../Resources/iPodScrobbler"
#else
    #define TWIDDLY_PATH QFileInfo( The::settings().path() ).dir().filePath( "iPodScrobbler.exe" )
#endif


#endif // CONFIGWIZARD_H
