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

#ifndef SIMPLEWIZARD_H
#define SIMPLEWIZARD_H

#include <QDialog>
#include <QList>

class QHBoxLayout;
class QPushButton;
class QVBoxLayout;

#ifdef WIN32
    #include "ui_wizarddialog_win.h"
#elif defined Q_OS_MAC
    #undef Q_OS_MAC
    #include "ui_wizarddialog_mac.h"
    #define Q_OS_MAC
#else
    #include "ui_wizarddialog_mac.h"
#endif


class SimpleWizard : public QDialog
{
    Q_OBJECT

public:
    SimpleWizard(QWidget* parent = 0);

    void enableNext(bool enable);
    void enableBack(bool enable);
    void setTitle(const QString& title) { mTitle = title; }

    int currentPage() { return mHistory.size() - 1; }
    int numPages() { return mNumPages; }

protected:
    virtual QWidget* createPage(int index) = 0;
    void setNumPages(int n);
    virtual void switchPage(QWidget* oldPage, QWidget* newPage);
    void updateButtons();

    Ui::WizardDialog ui;

protected slots:
    virtual void backButtonClicked();
    virtual void nextButtonClicked();

private:
    QList<QWidget*> mHistory;
    int mNumPages;

    QString mTitle;
};

#endif
