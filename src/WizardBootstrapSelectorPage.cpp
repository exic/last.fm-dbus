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

#include "plugininfo.h"
#include "WizardBootstrapSelectorPage.h"
#include "simplewizard.h"
#include "logger.h"

#include <QWidget>
#include <QRadioButton>


WizardBootstrapSelectorPage::WizardBootstrapSelectorPage(
    SimpleWizard*   wizard,
    const QString&  info ) :
        QWidget( wizard )
{
    ui.setupUi( this );
    ui.label->setText( info );
}

void 
WizardBootstrapSelectorPage::populate( std::vector<CPluginInfo>& pluginList )
{
    QVBoxLayout* layout = new QVBoxLayout( ui.blankWidget );
    ui.blankWidget->setLayout( layout );

    mPlugins.clear();

    bool firstItem = true;
    for( std::vector<CPluginInfo>::iterator iter = pluginList.begin(); iter != pluginList.end(); iter++ )
    {
        if( !iter->IsBootstrappable() || !iter->IsPluginInstalled() ) continue;
        
        mPlugins.push_back( *iter );

        QRadioButton* item = new QRadioButton( tr( "Import from " ) + iter->GetPlayerName(), this );

        if( firstItem )
        {
            item->toggle();
            firstItem = false;
        }

        layout->addWidget( item );
    }
    QRadioButton * item = new QRadioButton( tr( "Don't import listening history" ), this );
    layout->addWidget( item );
}

CPluginInfo*
WizardBootstrapSelectorPage::selectedPlugin()
{
    QLayout* layout = ui.blankWidget->layout();
    
    QLayoutItem* item;
    for( int index = 0; ( item = layout->itemAt( index ) ); index++ )
    {
        QRadioButton* radioButton = static_cast<QRadioButton*>( item->widget() );
        if( radioButton->isChecked() )
        {
            if( index < static_cast<int>( mPlugins.size() ) )
                return &mPlugins[ index ];
        }
    }

    return NULL;
}
