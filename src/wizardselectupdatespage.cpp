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

#include "updatewizard.h"
#include "wizardselectupdatespage.h"
#include "simplewizard.h"
#include "CachedHttp.h"

#include <QMessageBox>
#include <string>


Q_DECLARE_METATYPE(CComponentInfo *)

using namespace std;

/******************************************************************************
    WizardSelectUpdatesPage
******************************************************************************/
WizardSelectUpdatesPage::WizardSelectUpdatesPage(
    SimpleWizard*   wizard) :
        QWidget(wizard)
{
    ui.setupUi(this);

  #ifdef Q_WS_MAC
    ui.label->setText( tr("There is a new version of Last.fm available.") );
	ui.label_2->setText( tr("Please click Continue to have it downloaded and installed automatically.") );
  #endif

    connect(ui.updatesList, SIGNAL(itemChanged(QListWidgetItem*)),
            this,           SLOT  (pageTouched()));

    wizard->enableNext(true);
    wizard->enableBack(false);

    m_Wizard = wizard;
}

/******************************************************************************
    Populate
******************************************************************************/
bool
WizardSelectUpdatesPage::Populate(
    vector<CComponentInfo*>& vecUpdatables)
{
//     qDebug() << "WSU pop";
    
    ui.updatesList->clear();

    bool majorUpgrade = false;
    QString description;
    QUrl image;

    for(int i = 0; i < static_cast<int>(vecUpdatables.size()); ++i) {
        CComponentInfo& current = *vecUpdatables.at( i );
        if( current.IsApp() && current.IsMajorUpgrade() ) {
            m_majorComponent = &current;
            majorUpgrade = true;
            description = current.GetDescription();
            image = current.GetImage();
            break;
        }
    }

    if( majorUpgrade ) {
        MajorUpgrade( description, image );
        return false;
    }

    for (int i = 0; i < static_cast<int>(vecUpdatables.size()); ++i)
    {
        CComponentInfo& current = *vecUpdatables.at(i);
        QString sName = current.GetName();

        if (!current.IsApp())
        {
            sName += tr(" plugin"); 
        }

        int nCheck;
        if (!current.IsInstalled())
        {
            sName += tr(" (not installed)");
            nCheck = 0;
        }
        else if (current.IsInstalled() && current.IsVersionNewer())
        {
            sName += tr(" (version ") + current.GetVersion() + ")";
            nCheck = 1;
        }
        else
        {
            // Component is installed and up to date
            return true;
        }

        QListWidgetItem* item = new QListWidgetItem(sName, ui.updatesList);
        
        #ifndef Q_WS_MAC
            item->setCheckState(nCheck == 1 ? Qt::Checked : Qt::Unchecked);
        #endif
        
        // Store its pointer along with string
        item->setData(1, qVariantFromValue(&current));
    }
    return true;
}

/******************************************************************************
    pageTouched
******************************************************************************/
void
WizardSelectUpdatesPage::pageTouched()
{
//     qDebug() << "WSU touched";

    #ifndef Q_WS_MAC
    
        // Only enable Next if at least one entry is checked
        for (int i = 0; i < ui.updatesList->count(); ++i)
        {
            if (ui.updatesList->item(i)->checkState() == Qt::Checked)
            {
                m_Wizard->enableNext(true);
                return;
            }
        }
        m_Wizard->enableNext(false);

    #endif
}

/******************************************************************************
    GetChecked
******************************************************************************/
void
WizardSelectUpdatesPage::GetChecked(
    vector<CComponentInfo*>& vecChecked)
{
//     qDebug() << "WSU get checked";

    // Step through list box and pick out the checked ones
    for (int i = 0; i < ui.updatesList->count(); ++i)
    {
        #ifndef Q_WS_MAC
        if (ui.updatesList->item(i)->checkState() == Qt::Checked)
        #endif
        {
            CComponentInfo* pComp =
                qVariantValue<CComponentInfo*>(
                    ui.updatesList->item(i)->data(1));
            vecChecked.push_back(pComp);
        }
    }

}


void 
WizardSelectUpdatesPage::GetMajorUpdateComponent(
     vector<CComponentInfo*>& vecToUpdate )
{
    vecToUpdate.push_back( m_majorComponent );
}


void 
WizardSelectUpdatesPage::MajorUpgrade( QString desc, QUrl imageUrl )
{
    majorUpdateUi.setupUi( &majorUpdateDialog );
    QPushButton* okButton = majorUpdateUi.buttonBox->button( QDialogButtonBox::Ok );
    okButton->setText( tr( "Update" ));
    okButton->setFocus( Qt::OtherFocusReason );
    okButton->setDefault( true );
    okButton->setAutoDefault( true );
    majorUpdateUi.description->setText( desc );
    CachedHttp* http = new CachedHttp( imageUrl.host(), imageUrl.port() >= 0 ?
                                                        imageUrl.port() : 80, this );
    m_imageRequestId = http->get( imageUrl.path() );

    connect( http, SIGNAL(requestFinished( int, bool )), SLOT( onImageRequestFinished( int, bool ))); 
}


void
WizardSelectUpdatesPage::onImageRequestFinished( int id, bool error )
{
    if( error || id != m_imageRequestId ) {
        //sender()->deleteLater();
        return;
    }

    CachedHttp* http = qobject_cast<CachedHttp*>( sender());

    QPixmap pm;
    pm.loadFromData(http->readAll());
    
    majorUpdateUi.screenshot->setPixmap( pm );

    if( majorUpdateDialog.exec() == QDialog::Accepted ) {
        UpdateWizard* wizard = qobject_cast<UpdateWizard*>(m_Wizard);
        wizard->nextButtonClicked();
        wizard->exec();
    }

    sender()->deleteLater();
}
