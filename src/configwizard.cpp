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

#include "configwizard.h"
#include "logger.h"
#include "LastFmSettings.h"
#include "WebService.h"
#include "LastMessageBox.h"
#include "container.h"

#ifndef Q_WS_X11
    #include "Bootstrapper/iTunesBootstrapper.h"
    #include "Bootstrapper/PluginBootstrapper.h"
#endif


using namespace std;

bool ConfigWizard::s_wizardRunning = false;


/******************************************************************************
    ConfigWizard
******************************************************************************/
ConfigWizard::ConfigWizard( QWidget* parent, Mode mode,  QString uid )
        : BaseWizard( parent ),
          m_mode( mode ),
          m_bootstrapAllowed( false ),
          m_didBootstrap( false ),
          m_bootstrapStatus( -1 )
{

    LOGL(3, "Launching ConfigWizard");

    m_uid = uid;

    // Init strings here as they don't get translated if they're global
    m_introHeader = tr("Last.fm Setup");

    m_introInfo =
        tr("Now that you've installed Last.fm, it needs to be "
        "set up for your computer. Don't worry, it won't take long and only "
        "needs to be done once.\n\nThe social music revolution awaits! ");
    #ifdef Q_WS_MAC
        m_introInfo += tr("Click Continue to begin.");
    #else
        m_introInfo += tr("Click Next to begin.");
    #endif

    m_notAllowedInfo =
        tr("As this wizard installs file on your computer, you must be "
        "logged in as an Administrator to complete it. Please get an Administrator "
        "to run this for you.");

    m_loginHeader =
        tr("Log in");

    m_detectExplainHeader =
        tr("Music Player Detection");

    m_detectExplainInfo =
        tr("Last.fm will now look for music players on your computer "
        "and then download the plugins you need to get scrobbling.\n\n"
        "Before continuing, make sure all your music player software is closed.\n\n"
        "Click Next to continue.");

    m_detectHeader =
        tr("Detecting Music Players");

    m_detectInfo =
        tr("Downloading plugin information from Last.fm.");

    m_selectHeader =
        tr("Select Plugins");

    m_downloadHeader =
        tr("Downloading Plugins");

    m_doneHeader =
        tr("Finally...");

    m_bootstrapHeader =
        tr("Import your media player listening history");

    m_bootstrapQuestion =
        tr("Do you want to import your media player listening history?\n\n"
        "This will add charts to your profile based on what you've listened to in the past.");

    m_bootstrapInfo =
        tr("Last.fm is now importing your listening history.");

    m_mediaDeviceHeader =
        tr("Connect your iPod with Last.fm");

    m_mediaDeviceQuestion =
        tr("You've connected your iPod with Last.fm running for the first time. "
           "Would you like to scrobble the tracks played on your iPod to a profile from now on?");

    #ifdef Q_WS_MAC
        m_doneInfoFirstRun =
            tr("Last.fm is set up and you're ready to start scrobbling.\n\n"
            "If you close your Last.fm window, you can easily access it from the "  
            "icon in the menu bar.");
    #else
        m_doneInfoFirstRun =
            tr("Last.fm is set up and you're ready to start scrobbling.\n\n"
            "You can access Last.fm at any time by double-clicking the Last.fm user "
            "icon in the system tray.");
    #endif

    m_doneInfoClientBootstrapExtra = 
        tr("\n\nYour imported media player library will show up on your profile page "
        "within a few minutes.");

    m_doneInfoPluginBootstrapExtra =
        tr("\n\nYour imported media player library will finish importing when the media player restarts." );

    m_doneInfoPlugin =
        tr("The plugin(s) you selected have now been installed.");

    #ifdef Q_WS_MAC
        setTitle(tr("Set up Last.fm"));
    #else 
        setTitle(tr("Setup Wizard"));
    #endif

    switch (mode)
    {
        case Login:
            m_pageOffset = 0;
          #ifdef WIN32
            setNumPages(11);
          #elif defined Q_WS_MAC
            setNumPages(7);
          #elif defined Q_WS_X11
            setNumPages(3);
          #endif
            break;

        case MediaDevice:
            #ifdef Q_WS_X11
              Q_ASSERT( !"MediaDevice mode not available on Linux" );
            #endif
            
            setTitle(tr("Set up Last.fm for your iPod"));
            m_pageOffset = 9;
            setNumPages( 2 );
            break;            

        case Plugin:
            #ifndef WIN32
                Q_ASSERT( !"Plugin mode only available on Win" );
            #endif

            m_pageOffset = 2;
            setNumPages( 8 );
            break;

        case BootStrap:
            #ifdef Q_WS_X11
                Q_ASSERT( !"Bootstrap not yet available for Linux!" );
            #endif
            m_pageOffset = 6;
            setNumPages( 4 );

            break;
    }

    BaseWizard::nextButtonClicked();


    if ( mode != MediaDevice )
    {
        connect(&m_infoGetter, SIGNAL(updateInfoDone(bool, QString)),
                this,          SLOT  (pluginInfoDone(bool, QString)), Qt::QueuedConnection);

        connect(&m_updater, SIGNAL(updateDownloadDone(bool, QString)),
                this,       SLOT  (pluginDownloadDone(bool, QString)), Qt::QueuedConnection);

        connect( The::webService(), SIGNAL(handshakeResult( Handshake* )), SLOT(handshakeFinished()) );
    }
}

/******************************************************************************
    createPage
******************************************************************************/
QWidget* ConfigWizard::createPage( int index )
{
    index += m_pageOffset;

    switch ( index )
    {
        case 0:
        {
            m_page1 = new WizardInfoPage( this, m_introInfo );
            return m_page1;
        }
        break;

        case 1:
        {
            m_page2 = new WizardLoginPage( this );
            connect( m_page2, SIGNAL( verifyResult( bool, bool ) ),
                     this,    SLOT( loginVerified( bool, bool ) ) );
            return m_page2;
        }
        break;

        case 2:
        {
            m_page3 = new WizardInfoPage( this, m_detectExplainInfo );
            return m_page3;
        }
        break;

        case 3:
        {
            m_page4 = new WizardProgressPage( this, "", "" );

            // Hook up infoGetter signals to progress page
            connect( &m_infoGetter, SIGNAL( progressMade( int, int ) ),
                     m_page4,       SLOT( setProgress( int, int ) ) );
            connect( &m_infoGetter, SIGNAL( statusChange( QString ) ),
            
                     m_page4,       SIGNAL( detailedInfoChanged( QString ) ) );
            m_page4->setInfo( m_detectInfo );

            return m_page4;
        }
        break;

        case 4:
        {
            m_page5 = new WizardSelectPluginPage( this );
            return m_page5;
        }
        break;

        case 5:
        {
            m_page6 = new WizardProgressPage( this, "", "" );

            // Hook up downloader signals to progress page
            connect(&m_updater, SIGNAL(progressMade(int, int)),
                    m_page6,    SLOT  (setProgress(int, int)));
            connect(&m_updater, SIGNAL(statusChange(QString)),
                    m_page6,    SIGNAL(detailedInfoChanged(QString)));
            connect(&m_updater, SIGNAL(newFile(QString)),
                    m_page6,    SLOT  (setInfo(QString)));

            return m_page6;
        }
        break;

        case 6:
        {
            const QString& info = m_bootstrapQuestion;
            
            m_page7 = new WizardBootstrapSelectorPage(this, info);

			// Due to the mac build not checking for available plugins, this creates a pseuso plugin
			// with details for the bootstrapper.
            #ifdef Q_WS_MAC

				vector<CPluginInfo> list;	
				list.push_back( CPluginInfo() );
				list[0].SetId( "itm" );
				list[0].SetPlayerName( "iTunes" );
				list[0].SetBootstrapType( "Client" );
				
			#else //not Q_WS_MAC
			
	            vector<CPluginInfo>& list = The::container().getPluginList();
            
	            if( list.empty() )
	                list = mAvailPlugins;
	
			#endif

            m_page7->populate( list );

            return m_page7;
        }
        break;

        case 8:
        {
            #ifndef Q_WS_X11
            // Create a bootstrapper progress page
            const QString& info = m_bootstrapInfo;

            m_page9 = new WizardBootstrapPage(this, info);

            CPluginInfo* bootstrapPlugin = m_page7->selectedPlugin();
            if( bootstrapPlugin->GetBootstrapType() != CPluginInfo::BOOTSTRAP_CLIENT )
            {
                m_page9->onUploadProgress( 100 );
            }
            else
            {
                AbstractFileBootstrapper* fileBootstrapper = static_cast<AbstractFileBootstrapper*>( m_bootstrapper );
                connect( fileBootstrapper , SIGNAL( trackProcessed( int, const TrackInfo& ) ),
                         m_page9,           SLOT  ( onTrackFound( int, const TrackInfo& ) ) );

                connect( fileBootstrapper, SIGNAL(percentageUploaded( int ) ), 
                         m_page9,          SLOT  (onUploadProgress( int ) ) );

                connect( fileBootstrapper,  SIGNAL(done( int ) ),
                         this,              SLOT  (onBootstrapDone( int ) ) );
            }

            return m_page9;
            #endif
        }
        break;

        case 9:
        {
            switch (m_mode)
            {
              #ifndef Q_WS_X11
                case MediaDevice:
                case Login:
                case Plugin:
                    if ( twiddlyBootstrapRequired() )
                        break;
              #endif

                // FALL THROUGH

                default:
                    // don't do the twiddly bootstrap
                    // returning 0 forces the basewizard to do nextButtonClicked()
                    m_pageOffset++;
                    setNumPages( numPages() - 1 );
                    return 0;
            }

            m_pageTwiddly = new WizardTwiddlyBootstrapPage( this );
            connect( m_pageTwiddly, SIGNAL(done()), SLOT(onTwiddlyBootstrapDone()) );
            enableNext( false );
            return m_pageTwiddly;
        }
        break;

        case 10:
        {
            QString info;

            if ( m_mode == Login )
            {
                LOGL( 3, "Setting first run done" );
                The::settings().setFirstRunDone();
                info = m_doneInfoFirstRun;
            }
            else if ( m_mode == Plugin )
            {
                info = m_doneInfoPlugin;
            }
            else if ( m_mode == MediaDevice )
            {
                info = tr( "Last.fm is now ready for iPod scrobbling. Scrobbles should show up on your profile within 20 minutes of syncing your iPod with iTunes." );
            }

            #ifndef Q_WS_X11
            if ( m_didBootstrap )
            {
                info += "<p>"; // we may have already got a text from above
                switch (m_bootstrapStatus)
                {
                    case AbstractBootstrapper::Bootstrap_Spam:
                        info += tr( "Sorry, your iTunes listening history is possibly corrupted "
                                    "or too large to be imported into Last.fm. "
                                    "<p>"
                                    "Listen to music and your profile will build up quickly!" );
                        break;

                    case AbstractBootstrapper::Bootstrap_UploadError:
                        info += tr( "Sorry, an error occurred while uploading your listening history. "
                                    "Please try again later." );
                        break;

                    case AbstractBootstrapper::Bootstrap_Ok:
                        if( m_page7->selectedPlugin()->GetBootstrapType() == CPluginInfo::BOOTSTRAP_CLIENT )
                        {
                            info += m_doneInfoClientBootstrapExtra;
                        }
                        else if( m_page7->selectedPlugin()->GetBootstrapType() == CPluginInfo::BOOTSTRAP_PLUGIN )
                        {
                            info += m_doneInfoPluginBootstrapExtra;
                        }
                        break;

                    case AbstractBootstrapper::Bootstrap_Denied:
                        info += tr( "It was not possible to import your listening history. "
                                    "Could it be that you've already imported it?" );
                        break;

                    default:
                        Q_ASSERT( !"unhandled" );
                        break;

                }
            }
            #endif

            m_page10 = new WizardInfoPage( this, info );
            enableBack(false);
            return m_page10;
        }
        break;

    }

    return NULL;
}

/******************************************************************************
    headerForPage
******************************************************************************/
QString
ConfigWizard::headerForPage(
    int index )
{
    index += m_pageOffset;

    switch (index)
    {
        case 0: return m_introHeader;         break;
        case 1: return m_loginHeader;         break;
        case 2: return m_detectExplainHeader; break;
        case 3: return m_detectHeader;        break;
        case 4: return m_selectHeader;        break;
        case 5: return m_downloadHeader;      break;
        case 6: return m_bootstrapHeader;     break;
        case 7: return m_mediaDeviceHeader;   break;
        case 8: return m_bootstrapHeader;     break;
        case 9: return tr("Preparing for iPod Scrobbling");
        case 10: return m_doneHeader;          break;
    }

    return "";
}

/******************************************************************************
    backButtonClicked
******************************************************************************/
void
ConfigWizard::backButtonClicked()
{
    BaseWizard::backButtonClicked();
}

/******************************************************************************
    nextButtonClicked
******************************************************************************/
void
ConfigWizard::nextButtonClicked()
{
    // When we get in here, currentPage holds the page number we're leaving

    switch (currentPage() + m_pageOffset)
    {
        case 0:
        {
            // Do nothing
        }
        break;

        case 1:
        {
            // Leaving login page, verify login details
            QApplication::setOverrideCursor( Qt::WaitCursor );
            m_page2->verify();

            //disable next / forward buttons during verification
            enableNext(false);
            enableBack(false);
            return;
        }
        break;

        case 2:
        {
            // Leaving detect explain page, download plugin info
            mAvailPlugins.clear();
            m_infoGetter.GetUpdateInfo(NULL, &mAvailPlugins, true);
        }
        break;

        case 3:
        {
            // Leaving progress page, display available plugins
            BaseWizard::nextButtonClicked();
            m_page5->Populate(mAvailPlugins);
            return;
        }
        break;

        case 4:
        {
            // Made plugin selection, move to progress screen
            BaseWizard::nextButtonClicked();
            if (!downloadPlugins())
            {
                BaseWizard::backButtonClicked();
            }
            return;
        }
        break;

        case 5:
        {
            // Progress screen for plugin download
        }
        break;

        case 6:
        {
            // We have just answered the bootstrap question

            if ( m_page7 && m_page7->selectedPlugin() != NULL )
            {
                CPluginInfo* bootstrapPlugin = m_page7->selectedPlugin();

                if( !bootstrapPlugin )
                {
                    nextButtonClicked();
                    return;
                }

                #ifndef Q_WS_X11
                switch( bootstrapPlugin->GetBootstrapType() )
                {
                case CPluginInfo::BOOTSTRAP_CLIENT:
                    if( bootstrapPlugin->GetPlayerName() == "iTunes" )
                    {
                        m_bootstrapper = new iTunesBootstrapper();
                    }
                    else
                    {
                        Q_ASSERT( !"No clientside bootstrapper implemented for this mediaplayer yet." );
                    }
                    break;

                case CPluginInfo::BOOTSTRAP_PLUGIN:
                    m_bootstrapper = new PluginBootstrapper( bootstrapPlugin->GetId() );
                    m_didBootstrap = true;
                    m_bootstrapStatus = AbstractBootstrapper::Bootstrap_Ok;
                    break;

                default:
                    Q_ASSERT( !"not possible" );
                }

                setNumPages( numPages() - 1 );
                m_pageOffset++;

                if ( m_page7->selectedPlugin()->GetBootstrapType() == CPluginInfo::BOOTSTRAP_PLUGIN )
                {
                    setNumPages( numPages() - 1 );
                    m_pageOffset++;
                }

                nextButtonClicked();

                m_bootstrapper->bootStrap();

                if( !dynamic_cast<AbstractFileBootstrapper*>( m_bootstrapper ) )
                {
                    onBootstrapDone( AbstractBootstrapper::Bootstrap_Ok );
                }
                #endif

                return;
            }
            else
            {
                setNumPages( numPages() - 2 );
                m_pageOffset += 2;
            }
        }
        break;

        case 7:
        break;

        case 8:
        {
            // next clicked on bootstrap progress page
        }
        break;
        
        case 9:
        {
            // next clicked on Twiddly Bootstrap page
        }
        break;

        case 10:
        {
            // Done page
        }
        break;

        default:
            Q_ASSERT( !"duh" );

    }

    BaseWizard::nextButtonClicked();

}

/******************************************************************************
    downloadPlugins

    A lot of this is duplicate code from UpdateWizard, should really
    consolidate this.
******************************************************************************/
bool
ConfigWizard::downloadPlugins()
{
    mDownloadTasks.clear();

    // Get plugin selections
    vector<int> indicesToInstall;
    m_page5->GetChecked(indicesToInstall);
    for (size_t i = 0; i < indicesToInstall.size(); ++i)
    {
        CPluginInfo& current = mAvailPlugins.at(indicesToInstall.at(i));
        mDownloadTasks.push_back(&current);
    }

    // Check if any are running and ask to shut them down
    vector<QString> vecRunning;
    if (m_updater.CheckIfRunning(mDownloadTasks, vecRunning))
    {
        // Ask user if we should shut these running apps down
        QString sPrompt(tr("The following music players seem to be running at the moment:\n\n"));
        for (size_t j = 0; j < vecRunning.size(); ++j)
        {
            sPrompt += vecRunning.at(j) + "\n";
        }
        sPrompt += tr("\nThey need to be shut down before plugins can be installed.\n"
            "Do you want Last.fm to close them?");

        QString sCaption(tr("Detected Running Player(s)"));

        int answer = LastMessageBox::question(sCaption, sPrompt,
            QMessageBox::Yes, QMessageBox::No);
        if (answer == QMessageBox::No)
        {
            return false;
        }

        update();

        // Go ahead and kill the poor bastards
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        bool bSuccess = m_updater.KillRunning();

        if (!bSuccess)
        {
            QApplication::restoreOverrideCursor();

            // Let user shut them down if it didn't work
            LastMessageBox::warning(tr("Shutdown Failed"),
                tr("Some of the running programs couldn't be shut down. Please close them manually."),
                QMessageBox::Ok, QMessageBox::NoButton);

            return false;
        }

        QApplication::restoreOverrideCursor();
    }

    // Let the download commence
    m_updater.downloadUpdates(mDownloadTasks);

    return true;
}


/******************************************************************************
    loginVerified
******************************************************************************/
void
ConfigWizard::loginVerified(
    bool valid,
    bool bootstrap)
{
    m_bootstrapAllowed = bootstrap;
    qDebug() << "Bootstrap allowed" << m_bootstrapAllowed;

    #ifdef Q_WS_X11
    m_bootstrapAllowed = false;
    #endif

    if ( valid )
    {
        // This will cause the web service to do a handshake. We do not want
        // to advance to the next page until the handshake has finished so
        // we'll wait until that signal comes back before advancing.
        m_page2->save();
    }
    else
    {
        // Do nothing, remain on login page
        QApplication::restoreOverrideCursor();

        //re-enable next / previous buttons
        enableNext(true);
        enableBack(true);
    }
}

/******************************************************************************
    handshakeFinished
******************************************************************************/
void
ConfigWizard::handshakeFinished()
{
    QApplication::restoreOverrideCursor();

    #ifndef WIN32
        #ifdef Q_WS_MAC
            if ( m_bootstrapAllowed )
                m_pageOffset += 4;
            else
            {
                setNumPages( numPages() - 3 );
                m_pageOffset += 7;
            }

            nextButtonClicked();
        #else
            m_pageOffset += 7;
            BaseWizard::nextButtonClicked();
        #endif

    #else
        BaseWizard::nextButtonClicked();
    #endif
}

/******************************************************************************
    pluginInfoDone
******************************************************************************/
void
ConfigWizard::pluginInfoDone(
    bool    error,
    QString errorMsg)
{
    if (error)
    {
        LOG(2, "Connection problem: " << errorMsg << "\n");

        LastMessageBox::critical(tr("Connection Problem"),
            tr("Last.fm couldn't connect to the Internet to download "
               "plugin information.\n\nError: %1").arg(errorMsg));

        BaseWizard::backButtonClicked();
    }
    else
    {
        // mAvailPlugins should now have been filled
        nextButtonClicked();
    }
}

/******************************************************************************
    pluginDownloadDone
******************************************************************************/
void
ConfigWizard::pluginDownloadDone(
    bool    error,
    QString errorMsg)
{
    if (error)
    {
        LOG(2, "Download Error: " << errorMsg << "\n");

        LastMessageBox::critical(tr("Download Error"),
            tr("Last.fm failed to download and install the selected "
               "plugins.\n\nError: %1").arg(errorMsg));

        BaseWizard::backButtonClicked();
    }
    else
    {
        // Leaving select plugin page, start download
        QStringList l = The::settings().allPlugins();

        qDebug() << l;

        // Skip all iPod/bootstrap screens
        if ( !m_bootstrapAllowed )
        {
            setNumPages( numPages() - 3 );
            m_pageOffset += 3;
        }

        /*
        else if ( m_bootstrapAllowed )
        {
            setNumPages( numPages() - 1 );
            m_pageOffset += 1;
            nextButtonClicked();
            return;
        }
        */

        BaseWizard::nextButtonClicked();
    }
}

/******************************************************************************
    reject
******************************************************************************/
void
ConfigWizard::reject()
{
    m_infoGetter.Cancel();
    m_updater.Cancel();

    QDialog::reject();
}


void
ConfigWizard::onBootstrapDone( int status )
{
    #ifndef Q_WS_X11
    bool pluginBootstrap = dynamic_cast<PluginBootstrapper*>( m_bootstrapper );
    if( pluginBootstrap )
    {
        bool restart = false;
        bool mediaPlayerRunning = m_page7->selectedPlugin()->IsRunning();
        if( mediaPlayerRunning )
        {
            restart = ( LastMessageBox::question( tr( "Media Player Restart Required" ),
                                                  tr( "Your listening history will be imported the next time you restart your media player.\n\n"
                                                      "Do you want Last.fm to restart it now?" ),
                                                  QMessageBox::Yes | QMessageBox::No, 
                                                  QMessageBox::Yes ) == QMessageBox::Yes );
            if( restart )
                m_page7->selectedPlugin()->KillProcess();
        }

        //Start the media player again if either restart was selected or the user chooses to start it up.
        if( restart || (!mediaPlayerRunning && 
                        LastMessageBox::question( tr( "Media Player Needs Starting" ),
                                                  tr( "Your listening history will be imported the next time you start your media player.\n\n"
                                                      "Do you want Last.fm to start it now?" ),
                                                  QMessageBox::Yes | QMessageBox::No, 
                                                  QMessageBox::Yes ) == QMessageBox::Yes ) )
        {
            m_page7->selectedPlugin()->ExecuteProcess();
        }

    }
    else
    {
        m_didBootstrap = true;
        m_bootstrapStatus = status;

        BaseWizard::nextButtonClicked();
    }   // !pluginBootstrap
    #endif
}


int
ConfigWizard::exec()
{
    if (m_mode == MediaDevice && !twiddlyBootstrapRequired())
        return QDialog::Accepted;
        
    s_wizardRunning = true;
    activateWindow();

    #ifdef Q_WS_MAC
    if ( m_mode == MediaDevice )
        setMinimumSize( 520, 392 );
    #endif

    int r = QDialog::exec();
    s_wizardRunning = false;

    return r;
}


void
ConfigWizard::onTwiddlyBootstrapDone()
{
    BaseWizard::nextButtonClicked();
    enableNext( true );
}


bool
ConfigWizard::twiddlyBootstrapRequired()
{
  #ifdef Q_OS_WIN
    // Check plugin version is new enough to bootstrap
    if ( The::settings().pluginVersion( "itw" ).split( '.' ).value( 0 ).toInt() < 3 )
        return false;
  #endif
    
    QProcess p;
    p.start( TWIDDLY_PATH, QStringList() << "--bootstrap-needed?" );
    
    if ( !p.waitForFinished() )
        // better to let the plugin manage the bootstrap if we can't be sure
        // it's needed, returning true could lead to misscrobbles
        return false;

    return p.exitCode();
}
