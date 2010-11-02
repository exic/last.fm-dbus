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

#include "container.h"

#include "updatewizard.h"
#include "logger.h"
#include "LastMessageBox.h"


using namespace std;

/******************************************************************************
    UpdateWizard
******************************************************************************/
UpdateWizard::UpdateWizard(
    CAutoUpdater& updater,
    QWidget*      parent) :
        BaseWizard(parent),
        m_showWizard( true )
{
    LOGL(3, "Launching UpdateWizard");

    // Init strings here as they don't get translated if they're global
    #ifndef Q_WS_MAC
        m_winTitle =
            tr("Automatic Update Wizard");
    #else
        m_winTitle =
            tr("Automatic Updater");
    #endif

    m_introHeader =
        tr("Updates Available");

    m_downloadingHeader =
        tr("Downloading Updates");

    m_doneHeader =
        tr("Done");

    m_doneInfo =
        tr("The updates were successfully installed.");

    m_needCloseHeader =
        tr("Application Update");

    m_needCloseInfo =
        tr("In order to update itself, Last.fm needs to shut down. "
        "Pressing Finish will make Last.fm close down, automatically install "
        "the new version, and then relaunch when done.");

    m_needCloseInfoManualRestart =
        tr("In order to update itself, Last.fm needs to shut down. "
        "Please restart the application to finish the update.");

    setTitle(m_winTitle);
    setNumPages(3);
    BaseWizard::nextButtonClicked();

    m_updater = &updater;

    vector<CComponentInfo*>& vecUpdatables = m_updater->GetUpdatables();
    m_showWizard = m_page1->Populate(vecUpdatables);

    connect(m_updater, SIGNAL(updateDownloadDone(bool, QString)),
            this,      SLOT  (updateDownloadDone(bool, QString)), Qt::QueuedConnection);
}


/******************************************************************************
    createPage
******************************************************************************/
QWidget*
UpdateWizard::createPage(
    int index)
{
    switch (index)
    {
        case 0:
        {
            m_page1 = new WizardSelectUpdatesPage(this);
            return m_page1;
        }
        break;

        case 1:
        {
            m_page2 = new WizardProgressPage(this, "", "");

            // Hook up downloader signals to progress page
            connect(m_updater, SIGNAL(progressMade(int, int)),
                    m_page2,    SLOT  (setProgress(int, int)));
            connect(m_updater, SIGNAL(statusChange(QString)),
                    m_page2,    SIGNAL(detailedInfoChanged(QString)));
            connect(m_updater, SIGNAL(newFile(QString)),
                    m_page2,    SLOT  (setInfo(QString)));

            return m_page2;
        }
        break;

        case 2:
        {
            QString info;
#if defined WIN32 || defined Q_WS_MAC
            info = m_restartNeeded ? m_needCloseInfo : m_doneInfo;
#else
            info = m_restartNeeded ? m_needCloseInfoManualRestart : m_doneInfo;
#endif
            m_page3 = new WizardInfoPage(this, info);
            enableBack(false);
            return m_page3;
        }
        break;

    }

    return NULL;
}

/******************************************************************************
    headerForPage
******************************************************************************/
QString
UpdateWizard::headerForPage(
    int index)
{
    switch (index)
    {
        case 0: return m_introHeader; break;
        case 1: return m_downloadingHeader; break;
        case 2: return m_restartNeeded ? m_needCloseHeader : m_doneHeader; break;
    }

    return "";
}

/******************************************************************************
    backButtonClicked
******************************************************************************/
void
UpdateWizard::backButtonClicked()
{
    BaseWizard::backButtonClicked();
}

/******************************************************************************
    nextButtonClicked
******************************************************************************/
void
UpdateWizard::nextButtonClicked()
{
    switch (currentPage())
    {
        case 0:
        {
            // Leaving select page, start downloads
            BaseWizard::nextButtonClicked();
            if (!downloadUpdates())
            {
                backButtonClicked();
            }
            return;
        }
        break;

        case 1:
        {
            // Leaving progress page, display done page
        }
        break;

    }

    BaseWizard::nextButtonClicked();
}

/******************************************************************************
    downloadUpdates

    A lot of this is duplicate code from UpdateWizard, should really
    consolidate this.
******************************************************************************/
bool
UpdateWizard::downloadUpdates()
{
    mDownloadTasks.clear();

    // Get selections
    vector<CComponentInfo*> vecToUpdate;
    if( m_showWizard )
        m_page1->GetChecked(vecToUpdate);
    else {
        m_page1->GetMajorUpdateComponent( vecToUpdate );
    }


    m_restartNeeded = false;
    for (size_t i = 0; i < vecToUpdate.size(); ++i)
    {
        CComponentInfo* current = vecToUpdate.at(i);
        mDownloadTasks.push_back(current);
        if (current->IsApp())
        {
            m_restartNeeded = true;
        }
    }

    // Check if any are running and ask to shut them down
    vector<QString> vecRunning;
    if (m_updater->CheckIfRunning(mDownloadTasks, vecRunning))
    {
        // Ask user if we should shut these running apps down
        QString sPrompt(tr("The following player applications seem to be running at the moment:\n\n"));
        for (size_t j = 0; j < vecRunning.size(); ++j)
        {
            sPrompt += vecRunning.at(j) + "\n";
        }
        sPrompt += tr("\nThey need to be shut down before plugins can be installed.\n"
            "Do you want Last.fm to close them?");

        QString sCaption( tr("Detected Running Players") );

        int answer = LastMessageBox::question(sCaption, sPrompt,
            QMessageBox::Yes, QMessageBox::No);
        if (answer == QMessageBox::No)
        {
            return false;
        }

        update();

        // Go ahead and kill the poor bastards
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        bool bSuccess = m_updater->KillRunning();

        if (!bSuccess)
        {
            QApplication::restoreOverrideCursor();

            // Let user shut them down if it didn't work
            LastMessageBox::warning(tr("Shutdown Failed"),
                tr("Some of the running applications couldn't be shut down. Please close them manually."),
                QMessageBox::Ok, QMessageBox::NoButton);

            return false;
        }

        QApplication::restoreOverrideCursor();
    }

    // Let the download commence
    m_updater->downloadUpdates(mDownloadTasks);

    return true;
}

/******************************************************************************
    updateDownloadDone
******************************************************************************/
void
UpdateWizard::updateDownloadDone(
    bool    error,
    QString errorMsg)
{
    if (error)
    {
        LOG(2, "Download Error: " << errorMsg << "\n");

        LastMessageBox::critical(tr("Download Error"),
            tr("Last.fm failed to download and install the selected "
               "updates.\n\nError: %1").arg(errorMsg));

        backButtonClicked();
    }
    else
    {
        BaseWizard::nextButtonClicked();
    }
}

/******************************************************************************
    accept
******************************************************************************/
void
UpdateWizard::accept()
{
    if (m_restartNeeded)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#ifndef Q_WS_X11
        bool ok = m_updater->LaunchAppInstaller();
#else
        bool ok = true;
#endif
        if (ok)
        {
            QDialog::accept();
            qApp->quit();
        }
        else
        {
            QApplication::restoreOverrideCursor();
            LastMessageBox::critical(tr("Install Error"),
                tr("The automatic installation failed.\nPlease download the "
                "new version manually from www.last.fm."));
            QDialog::reject();
        }
    }
    else
    {
        QDialog::accept();
    }
}

/******************************************************************************
    reject
******************************************************************************/
void
UpdateWizard::reject()
{
    m_updater->Cancel();

    QDialog::reject();
}
