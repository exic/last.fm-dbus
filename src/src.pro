TEMPLATE = app
TARGET = Last.fm
QT += gui network xml sql

INCLUDEPATH += lib libFingerprint/recommendation-commons

PRECOMPILED_HEADER = precompiled.h
CONFIG += precompile_header

unix {
    # precompiled headers breaks icecream builds for some reason :(
    system( test `ps aux | grep iceccd | wc -l` -gt 1 ): CONFIG -= precompile_header
}

#universal binaries cannot be built from precompiled headers
mac:release:CONFIG -= precompile_header

win32 {
    # LEAVE BEFORE DEFINTIONS IS INCLUDED!!!!!!!
    # Keep the old exe name for backwards compatibility and to avoid potential bugs
    TARGET = LastFM
}
unix:!mac{
    # LEAVE BEFORE DEFINTIONS IS INCLUDED!!!!!!!
    TARGET = last.fm
}

include( ../definitions.pro.inc )

# TODO remove
INCLUDEPATH += $$ROOT_DIR/res/mad
INCLUDEPATH += $$ROOT_DIR/src/libFingerprint/libs/fftw

breakpad {
    LIBS += -lbreakpad$$EXT
}

LIBS += -L$$BIN_DIR -lLastFmFingerprint$$EXT


FORMS   = container.ui \
          settingsdialog.ui \
          settingsdialog_account.ui \
          settingsdialog_radio.ui \
          settingsdialog_scrobbling.ui \
          settingsdialog_connection.ui \
          settingsdialog_mediadevices.ui \
          aboutdialog.ui \
          loginwidget.ui \
          progresswidget.ui \
          confirmwidget.ui \
          mediaDeviceConfirmWidget.ui \
          MediaDeviceConfirmDialog.ui \
          addplayerdialog.ui \
          selectpluginwidget.ui \
          selectupdateswidget.ui \
          dialogshell.ui \
          deleteuserdialog.ui \
          playcontrols.ui \
          failedlogindialog.ui \
          tagdialog.ui \
          ShareDialog.ui \
          MetaDataWidget.ui \
          MetaDataWidgetTuningIn.ui \
          RestStateWidget.ui \
          DiagnosticsDialog.ui \
          BootstrapSelectorWidget.ui \
          majorupdate.ui


HEADERS = container.h \
          settingsdialog.h \
          aboutdialog.h \
          Scrobbler-1.2.h \
          simplewizard.h \
          configwizard.h \
          wizardinfopage.h \
          WizardBootstrapSelectorPage.h \
          wizardmediadeviceconfirmpage.h \
          wizardloginpage.h \
          wizardprogresspage.h \
          wizardselectpluginpage.h \
          wizardbootstrappage.h \
          addplayerdialog.h \
          loginwidget.h \
          updateinfogetter.h \
          componentinfo.h \
          plugininfo.h \
          appinfo.h \
          exceptions.h \
          versionnumber.h \
          lib/FileVersionInfo/FileVersionInfo.h \
          lib/KillProcess/KillProcess.h \
          autoupdater.h \
          updatewizard.h \
          wizardselectupdatespage.h \
          playerlistener.h \
          playercommandparser.h \
          playercommands.h \
          playerconnection.h \
          iconshack.h \
          systray.h \
          progressframe.h \
          checkdirtree.h \
          deleteuserdialog.h \
          failedlogindialog.h \
          toolbarvolumeslider.h \
          tagdialog.h \
          ShareDialog.h \
          lastfmapplication.h \
          AudioController.h \
          Radio.h \
          RadioPlaylist.h \
          XspfResolver.h \
          MediaDeviceScrobbler.h \
          SideBarView.h \
          SideBarModel.h \
          SideBarDelegate.h \
          SideBarTreeStyle.h \
          SideBarToolTipLabel.h \
          SideBarRevealPopup.h \
          MetaDataWidget.h \
          TagListWidget.h \
          version.h \
          TrackProgressFrame.h \
          DiagnosticsDialog.h \
          User.h \
          RestStateWidget.h \
          RestStateMessage.h \
          SpinnerLabel.h \
          ProxyOutput.h \
          Bootstrapper/AbstractBootstrapper.h \
          Bootstrapper/AbstractFileBootstrapper.h \
          Bootstrapper/iTunesBootstrapper.h \
          Bootstrapper/PluginBootstrapper.h \
          Bootstrapper/ITunesDevice/ITunesDevice.h \
          WizardTwiddlyBootstrapPage.h


SOURCES = main.cpp \
          container.cpp \
          settingsdialog.cpp \
          aboutdialog.cpp \
          Scrobbler-1.2.cpp \
          simplewizard.cpp \
          configwizard.cpp \
          wizardinfopage.cpp \
          WizardBootstrapSelectorPage.cpp \
          wizardmediadeviceconfirmpage.cpp \
          wizardloginpage.cpp \
          wizardprogresspage.cpp \
          wizardselectpluginpage.cpp \
          wizardbootstrappage.cpp \
          addplayerdialog.cpp \
          loginwidget.cpp \
          updateinfogetter.cpp \
          componentinfo.cpp \
          plugininfo.cpp \
          appinfo.cpp \
          versionnumber.cpp \
          lib/FileVersionInfo/FileVersionInfo.cpp \
          autoupdater.cpp \
          updatewizard.cpp \
          wizardselectupdatespage.cpp \
          playerlistener.cpp \
          playercommandparser.cpp \
          playerconnection.cpp \
          iconshack.cpp \
          systray.cpp \
          progressframe.cpp \
          checkdirtree.cpp \
          deleteuserdialog.cpp \
          failedlogindialog.cpp \
          toolbarvolumeslider.cpp \
          tagdialog.cpp \
          ShareDialog.cpp \
          lastfmapplication.cpp \
          AudioController.cpp \
          Radio.cpp \
          RadioPlaylist.cpp \
          XspfResolver.cpp \
          MediaDeviceScrobbler.cpp \
          SideBarView.cpp \
          SideBarModel.cpp \
          SideBarDelegate.cpp \
          SideBarTreeStyle.cpp \
          SideBarToolTipLabel.cpp \
          SideBarRevealPopup.cpp \
          MetaDataWidget.cpp \
          TagListWidget.cpp \
          TrackProgressFrame.cpp \
          RestStateWidget.cpp \
          DiagnosticsDialog.cpp \
          User.cpp \
          RestStateMessage.cpp \
          ProxyOutput.cpp \
          Bootstrapper/AbstractBootstrapper.cpp \
          Bootstrapper/AbstractFileBootstrapper.cpp \
          Bootstrapper/iTunesBootstrapper.cpp \
          Bootstrapper/PluginBootstrapper.cpp \
          WizardTwiddlyBootstrapPage.cpp


unix:!mac {
    FORMS += wizarddialog_mac.ui \
             wizardshell_mac.ui

    HEADERS += simplewizard_mac.h \
               winstyleoverrides.h

    SOURCES += simplewizard_mac.cpp \
               winstyleoverrides.cpp

    LIBS += -lmad -lfftw3f

    HEADERS -=  Bootstrapper/iTunesBootstrapper.h \
                Bootstrapper/PluginBootstrapper.h
    
    SOURCES -= Bootstrapper/iTunesBootstrapper.cpp \
               Bootstrapper/PluginBootstrapper.cpp
}


mac {
    FORMS += wizarddialog_mac.ui \
             wizardshell_mac.ui

    HEADERS += simplewizard_mac.h \
               macstyleoverrides.h \
               itunesscript.h \
               ITunesPluginInstaller.h

    SOURCES += simplewizard_mac.cpp \
               macstyleoverrides.cpp \
               itunesscript.cpp \
               ITunesPluginInstaller.cpp

    SOURCES -= SideBarTreeStyle.cpp

    LIBPATH += $$ROOT_DIR/res/mad

    LIBS += -lmad -framework CoreFoundation -framework Carbon -lz
}


win32 {
    FORMS += wizarddialog_win.ui \
             wizardextshell_win.ui \
             wizardintshell_win.ui

    LIBS += -luser32 -lshell32 -lversion -lWs2_32 -lGdi32 -lzlibwapi

    HEADERS += simplewizard_win.h \
               winstyleoverrides.h

    SOURCES += simplewizard_win.cpp \
               winstyleoverrides.cpp

    RC_FILE = container.rc
}

RESOURCES = ../res/qrc/last.fm.qrc

QMAKE_CXX = $$QMAKE_CXX -IlibMoose/QtOverrides
