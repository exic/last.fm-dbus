/***************************************************************************
 *   Copyright 2008 Last.fm Ltd.                                           *
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

#ifndef WIZARD_TWIDDLY_BOOTSTRAP_PAGE_H
#define WIZARD_TWIDDLY_BOOTSTRAP_PAGE_H

#include "ui_progresswidget.h"


/** @author <max@last.fm> 
  */
class WizardTwiddlyBootstrapPage : public QWidget
{
    Q_OBJECT
    
public:
    WizardTwiddlyBootstrapPage( QWidget* parent );
    
signals:
    void done();
    
private slots:
    void onStdOut();
    void onFinished();
    void onError();

private:
    virtual void showEvent( QShowEvent* );

private:
    Ui::ProgressWidget ui;
    class QProcess* m_process;
};

#endif
