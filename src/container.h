/***************************************************************************
 *   Copyright (C) 2005 - 2008 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
 *      Jono Cole, Last.fm Ltd <jono@last.fm>                              *
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

#ifndef LAST_FM_CONTAINER_H
#define LAST_FM_CONTAINER_H

#include "ui_container.h"
#include "ui_playcontrols.h"

#include "WebService/fwd.h"
#include "RadioEnums.h"
#include "LastFmSettings.h"

#include <QSystemTrayIcon> //poo :( for enum

class QLabel;

class Container : public QMainWindow
{
    Q_OBJECT

        static Container* s_instance;

    public:
        Container();
        ~Container();

        static Container& instance() { return *s_instance; }
        class ShareDialog& shareDialog() { return *m_shareDialog; }
        int stackIndex() const { return ui.stack->currentIndex(); }

        std::vector<class CPluginInfo>& getPluginList();

    public slots:
        void checkForUpdates( bool invokedByUser = true );
        void showSettingsDialog( int startPage = 0 );
        void showDiagnosticsDialog();
        void showShareDialog();
        void showTagDialog( int defaultTagType = -1 );
        void showTagDialogMD(); //medical doctor??
        void restoreWindow();
        void toggleWindowVisibility();
        void quit();
        void scrobbleManualIpod();

        void showRestState();
        void showMetaDataWidget() { ui.stack->setCurrentIndex( 1 ); }

        /** tray icon balloon on windows, growl on osx */
        void showNotification( const QString& title, const QString& message );

    signals:
        void stackIndexChanged( int );
        void becameVisible();

    protected:
        void closeEvent( QCloseEvent* );
        void dropEvent( QDropEvent* );
        void dragEnterEvent( QDragEnterEvent* );
        void dragMoveEvent( QDragMoveEvent* );
        bool event( QEvent* );

    private:
        struct : Ui::MainWindow
        {
            class ScrobbleLabel* scrobbleLabel;
            class RestStateWidget* restStateWidget;
            class MetaDataWidget* metaDataWidget;
            class SideBarTree* sidebar;

            Ui::PlayControls playcontrols;

        } ui;

        void setupUi();
        void setupTimeBar();
        void applyPlatformSpecificTweaks();
        void applyMenuTweaks();
        void setupConnections();
        void restoreState();
        void removeDMCAWarnings();

        class TrayIcon* m_trayIcon;
        class CAutoUpdater *m_updater;

        /** having a persistent copy means whatever the user entered last time
          * into the input widgets and that, stays, and load time is quicker too */
        class ShareDialog *m_shareDialog;
        class DiagnosticsDialog *m_diagnosticsDialog;

        bool m_userCheck;
        bool m_sidebarEnabled;
        int  m_lastVolume;
        int  m_sidebarWidth;
        QByteArray m_geometry;

        #ifndef Q_WS_MAC
        QStyle* m_styleOverrides;
        #endif

    private slots:
        void setupTrayIcon();
        void about();
        void addUser();
        void deleteUser();
        void getPlugin();
        void onAboutToShowUserMenu();
        void onTrayIconActivated( QSystemTrayIcon::ActivationReason );
        void splitterMoved( int pos ) { m_sidebarWidth = pos; }

        /** opens log in default .log handler */
        void onAltShiftL();
        /** opens MooseUtils::savePath() in default handler */
        void onAltShiftF();

      #ifdef WIN32
        /** opens plugin uninstaller location */
        void onAltShiftP();
      #endif

        void toggleSidebar();
        void toggleScrobbling();
        void toggleDiscoveryMode();

        void onRadioError( RadioError error, const QString& message = "" );
        void onRadioBuffering( int size, int total );

        void displayUrlInStatusBar( const QUrl& url );
        void statusMessage( const QString& message ) { statusBar()->showMessage( message ); }

        void addToMyPlaylist();
        void love();
        void ban();
        void skip();
        void play();
        void stop();
        void volumeUp();
        void volumeDown();
        void mute();

        void showFAQ();
        void showForums();
        void inviteAFriend();

        void onUserSelected( QAction* action );
        void updateCheckDone( bool updatesAvailable, bool error, QString errorMsg );
        void updateWindowTitle( const MetaData& );
        void updateUserStuff( LastFmUserSettings& user );
        void updateAppearance();

        void onScrobblerStatusChange( int, const QVariant& );
        void onRadioStateChanged( RadioState );
        void onAppEvent( int event, const QVariant& data );

        void minimiseToTray();
        void gotoProfile();
        void crash();

        void webServiceSuccess( Request* );
        void webServiceFailure( Request* );
};


/** The Scrobbling on/off label in the right-hand corner of the statusbar */
class ScrobbleLabel : public QWidget
{
    Q_OBJECT

    QLabel* m_label;
    QLabel* m_image;

public:
    ScrobbleLabel();
    void setEnabled( bool );

    QLabel* label() const { return m_label; }

signals:
    void clicked();

protected:
    virtual void mousePressEvent( QMouseEvent* )
    {
        emit clicked();
    }
};


namespace The
{
    inline Container& container() { return Container::instance(); }
}

#endif // CONTAINER_H
