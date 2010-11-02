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

#ifndef UPDATEWIZARD_H
#define UPDATEWIZARD_H

#ifdef WIN32
#include "simplewizard_win.h"
#else
#include "simplewizard_mac.h"
#endif
#include "wizardinfopage.h"
#include "wizardprogresspage.h"
#include "wizardselectupdatespage.h"
#include "autoupdater.h"

#include <vector>

#ifdef WIN32
    typedef SimpleWizardWin BaseWizard;
#else
    typedef SimpleWizardMac BaseWizard;
#endif

class UpdateWizard : public BaseWizard
{
    Q_OBJECT

public:

    UpdateWizard(
        CAutoUpdater& updater,
        QWidget*      parent = NULL);

    UpdateWizard::UpdateWizard( CComponentInfo* );

    bool shouldShow() const{ return m_showWizard; }

protected:

    virtual QWidget*
    createPage(
        int index);

    virtual QString
    headerForPage(
        int index);

    virtual void
    accept();

    virtual void
    reject();


public slots:

    virtual void
    backButtonClicked();
    
    virtual void
    nextButtonClicked();


private:

    /*********************************************************************/ /**
        Start downloading updates. Returns true if download started,
        false otherwise.
    **************************************************************************/
    bool
    downloadUpdates();

    WizardSelectUpdatesPage* m_page1;
    WizardProgressPage*      m_page2;
    WizardInfoPage*          m_page3;

    CAutoUpdater*            m_updater;

    std::vector<CComponentInfo*> mDownloadTasks;
    
    bool m_restartNeeded;
    bool m_showWizard;
    
    QString m_winTitle;
    QString m_introHeader;
    QString m_downloadingHeader;
    QString m_doneHeader;
    QString m_doneInfo;
    QString m_needCloseHeader;
    QString m_needCloseInfo;
    QString m_needCloseInfoManualRestart;

private slots:

    /*********************************************************************/ /**
        Called by AutoUpdater when the plugins are downloaded and installed.
    **************************************************************************/
    void
    updateDownloadDone(
        bool error,
        QString errorMsg);


};

#endif // UPDATEWIZARD_H
