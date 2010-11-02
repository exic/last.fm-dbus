/**************************************************************************
*   Copyright (C) 2005 - 2007 by                                          *
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

#include "loginwidget.h"
#include "MooseCommon.h"
#include "UnicornCommon.h"
#include "LastFmSettings.h"
#include "lastfmapplication.h"
#include "URLLabel.h"
#include "failedlogindialog.h"
#include "logger.h"
#include "LastMessageBox.h"

#include "WebService/Request.h"

#include "ui_dialogshell.h"

#ifdef Q_WS_MAC
    extern void qt_mac_secure_keyboard(bool); //qapplication_mac.cpp
#endif

// These are functions so that the translator will have had time to get
// initialised before tr is called.
inline static QString badUserErrorString()
{
    return LoginWidget::tr( "No Last.fm user with that username was found.\n"
        "Please enter the username you used to sign up at Last.fm." );
}

inline static QString badPassErrorString()
{
    return LoginWidget::tr( "The password isn't correct.\n"
        "Please enter the password you used to sign up at Last.fm." );
}


///////////////////////////////////////////////////////////////////////////////>
LoginWidget::LoginWidget( QWidget* parent, Mode mode, QString defaultUser )
        : QWidget( parent ),
          m_Mode( mode ),
          m_saveLowerPass( false )
{
    ui.setupUi( this );

    ui.signUpLink->setText( tr( "Sign up for a Last.fm account" ) );
    ui.signUpLink->setURL( "http://" + UnicornUtils::localizedHostName( The::settings().appLanguage() ) + "/join/" );
    ui.signUpLink->setFloat( true );
    ui.signUpLink->setUnderline( true );
    ui.signUpLink->setHighlightedColor( QColor( 0, 0, 0 ) );
    ui.signUpLink->setSelectedColor( QColor( 100, 100, 100 ) );

    ui.forgotLink->setText( tr( "Forgot your password?" ) );
    ui.forgotLink->setFloat( true );
    ui.forgotLink->setUnderline( true );
    ui.forgotLink->setHighlightedColor( QColor( 0, 0, 0 ) );
    ui.forgotLink->setSelectedColor( QColor( 100, 100, 100 ) );

    // If we redirect the user to a different host than last.fm, we do _not_ want to
    // use the https protocol, since our certificate only matches the last.fm domain.
    ui.forgotLink->setURL( ( UnicornUtils::localizedHostName( The::settings().appLanguage() ).endsWith( "last.fm" ) ? "https://" : "http://" ) +
                           UnicornUtils::localizedHostName( The::settings().appLanguage() ) + "/settings/lostpassword/" );

    resetWidget( defaultUser );

    connect( ui.userCombo, SIGNAL( currentIndexChanged( QString ) ),
             this,         SLOT  ( userComboChanged( QString ) ) );

    connect( ui.userEdit, SIGNAL( textChanged( const QString& ) ),
             this,        SIGNAL( widgetChanged() ) );

    connect( ui.passwordEdit, SIGNAL( textChanged( const QString& ) ),
             this,            SIGNAL( widgetChanged() ) );

    connect( ui.rememberCheck, SIGNAL( stateChanged( int ) ),
             this,             SIGNAL( widgetChanged() ) );
}


LoginWidget::~LoginWidget()
{
#ifdef Q_WS_MAC
    //SecureInput QT bug workaround
    qt_mac_secure_keyboard( false );
#endif
}


/******************************************************************************
verify
******************************************************************************/
void
LoginWidget::verify()
{
    LOGL(4, "");

    QString username = m_Mode == LOGIN 
        ? ui.userCombo->currentText()
        : ui.userEdit->text();

    //Some "interesting" boolean logic that could probably be tidied up
    if ( ( !The::settings().isFirstRun() &&
           m_Mode != CHANGE_PASS  &&
           The::settings().isExistingUser( username ) &&
           The::settings().user( username ).rememberPass() ) ||
           ( m_Mode == CHANGE_PASS && !detailsChanged() ) )
    {
        // This happens when flipping the user in the drop-down to one
        // with remembered pass and we fill in the password for them.
        emit verifyResult( true, false ); //FIXME mxcl check old code
        emit verifySuccess();
        return;
    }

    QString password = ui.passwordEdit->text();
    QString passwordLower = password.toLower();
    QString pwMD5 = UnicornUtils::md5Digest( password.toUtf8() );
    QString pwMD5Lower = UnicornUtils::md5Digest( passwordLower.toUtf8() );

    VerifyUserRequest *verify = new VerifyUserRequest;
    verify->setUsername( username );
    verify->setPasswordMd5( pwMD5 );
    verify->setPasswordMd5Lower( pwMD5Lower );

    connect( verify, SIGNAL(result( Request* )), SLOT(verifyResult( Request* )), Qt::QueuedConnection );

    verify->start();
}


/******************************************************************************
save
******************************************************************************/
void
LoginWidget::save( bool reconnect )
{
    QString username = m_Mode == LOGIN 
        ? ui.userCombo->currentText()
        : ui.userEdit->text();

    // This allows a login screen to be used as an add user screen transparently
    if ( !The::settings().isExistingUser( username ) )
    {
        m_Mode = ADD_USER;
    }

    // This creates a new user if the user is not found
    LastFmUserSettings& userSettings = The::settings().user( username );

    QString pw = m_saveLowerPass ? ui.passwordEdit->text().toLower() :
        ui.passwordEdit->text();
    userSettings.setPassword( pw );
    userSettings.setRememberPass( ui.rememberCheck->checkState() == Qt::Checked );

    if ( m_Mode == ADD_USER )
    {
        userSettings.setIcon( The::settings().getFreeColour() );
    }

    if (reconnect)
        The::app().setUser( username );
    else
        The::settings().setCurrentUsername(username);
}


/******************************************************************************
verifyResult
******************************************************************************/
void
LoginWidget::verifyResult( Request *request )
{
    VerifyUserRequest* verify = static_cast<VerifyUserRequest*>(request);

    // If the request failed, the auth code doesn't get filled in properly
    // since the ws refactor, so we need to check for it here.
    UserAuthCode result = verify->failed() ? AUTH_ERROR : verify->userAuthCode();

    LOGL( 4, "Verify result: " << result );

    bool bootstrap = (verify->bootStrapCode() == BOOTSTRAP_ALLOWED);

    if (result == AUTH_OK)
    {
        m_saveLowerPass = false;
        emit verifySuccess();
        emit verifyResult( true, bootstrap );
    }
    else if ( result == AUTH_OK_LOWER )
    {
        m_saveLowerPass = true;
        emit verifySuccess();
        emit verifyResult( true, bootstrap );
    }
    else
    {
        QString msg;

        if ( result == AUTH_BADUSER )
        {
            LastMessageBox::critical( tr( "Login Failed" ), badUserErrorString() );
        }
        else if ( result == AUTH_BADPASS )
        {
            LastMessageBox::critical( tr( "Login Failed" ), badPassErrorString() );
        }
        else if ( result == AUTH_ERROR )
        {
            // If we're adding a user or changing password, we must verify
            // against the internet so we fire up FailedLoginDialog.
            // If it's just a login, we can fall back on verifying against
            // the password in the registry instead.
            if ( m_Mode == LOGIN || m_Mode == CHANGE_PASS )
            {
                LOGL( 3, "Couldn't authenticate via web, falling back on local" );

                if ( verifyLocally() )
                {
                    emit verifySuccess();
                    emit verifyResult( true, false );
                    return;
                }
                else
                {
                    LastMessageBox::critical( tr("Login Failed"), badPassErrorString() );
                }
            }
            else
            {
                LOGL( 4, "Got an AUTH_ERROR, will launch FailedDialog" );

                FailedLoginDialog( this ).exec();
            }
        }

        emit verifyFail();
        emit verifyResult( false, false );
    }
}


/******************************************************************************
verifyLocally
******************************************************************************/
bool
LoginWidget::verifyLocally()
{
    QString username = m_Mode == LOGIN 
        ? ui.userCombo->currentText()
        : ui.userEdit->text();
    QString enteredPass = ui.passwordEdit->text();
    enteredPass = UnicornUtils::md5Digest(enteredPass.toUtf8());

    if (!The::settings().isExistingUser( username ))
    {
        return false;
    }

    QString storedPass = The::settings().user( username ).password();

    return enteredPass == storedPass;
}


/******************************************************************************
userComboChanged
******************************************************************************/
void
LoginWidget::userComboChanged( QString username )
{
    if (The::settings().user(username).rememberPass())
    {
        ui.rememberCheck->setCheckState( Qt::Checked );
        ui.passwordEdit->setText( "********" );
    }
    else
    {
        ui.rememberCheck->setCheckState(Qt::Unchecked);
        ui.passwordEdit->clear();
    }
    emit widgetChanged();
}


void
LoginWidget::resetWidget( QString defaultUser )
{
    QString username;
    Qt::CheckState check = Qt::Checked;
    if ( !defaultUser.isEmpty() )
    {
        LastFmUserSettings &user = The::settings().user( defaultUser );
        username = user.username();
        check = user.rememberPass() ? Qt::Checked : Qt::Unchecked;
    }

    switch ( m_Mode )
    {
        // Fill drop down with all users
    case LOGIN:
        {
            LOGL( 3, "Launching login dialog mode LOGIN" );

            ui.stack->setCurrentIndex( 0 );

            // Populate dropdown
            int index = -1;
            QStringList allUsers = The::settings().allUsers();
            for ( int i = 0; i < allUsers.size(); ++i )
            {
                ui.userCombo->addItem( allUsers.at( i ) );
                if ( allUsers.at( i ) == username )
                {
                    index = i;
                    break;
                }
            }

            ui.userCombo->setCurrentIndex( index );
            ui.rememberCheck->setCheckState( check );
            if ( index != -1 )
            {
                ui.passwordEdit->setFocus();
            }
            else
            {
                ui.userCombo->setFocus();
            }

        }
        break;

        // Display empty line edit
    case ADD_USER:
        {
            LOGL( 3, "Launching login dialog mode ADD_USER" );

            ui.stack->setCurrentIndex( 1 );

            // AARGH, none of these work
            ui.userEdit->setFocus();
            //ui.userCombo->setFocus();
            //ui.stack->setFocus();

            ui.rememberCheck->setCheckState( Qt::Checked );
        }
        break;

        // Display only current user in edit
    case CHANGE_PASS:
        {
            LOGL( 3, "Launching login dialog mode CHANGE_PASS" );

            // Adapt to usage inside settings dialog
            QVBoxLayout* widgetlayout = dynamic_cast<QVBoxLayout*>(this->layout());

            //Double check that the layout type hasn't changed
            Q_ASSERT( widgetlayout );

            widgetlayout->setMargin( 0 );
            ui.enterLabel->hide();
            ui.spacerItem->changeSize( 0, 0 );
            ui.spacerItem3->changeSize( 0, 0 );

            ui.stack->setCurrentIndex( 1 );
            ui.userEdit->setText( username );
            if ( check == Qt::Checked )
                ui.passwordEdit->setText( "********" );
            else
                ui.passwordEdit->setText( "" );

            ui.passwordEdit->setFocus();
            ui.rememberCheck->setCheckState( check );
        }
        break;

    } // end switch
}


QDialog&
LoginWidget::createDialog()
{
    QDialog* dialog = new QDialog( parentWidget() );

    // note that the ui object is created on the stack as other than initialising 
    // the UI, it is not needed so can be binned (it is not a QObject so it can't 
    // be created on the heap without watching out for memory leaks!)

    Ui::DialogShell dialogUi;
    dialogUi.setupUi( dialog );

    // Want etched, not flat
    dialogUi.line->setFrameShadow( QFrame::Sunken );

    dialogUi.vboxLayout->insertWidget( 0, this );
    dialog->setFixedSize( dialog->sizeHint() );

    switch (m_Mode) 
    {
    case LOGIN:
        dialog->setWindowTitle( tr("Log In") );

        if( The::settings().allUsers().size() > 0 )
            ui.passwordEdit->setFocus();
        else
            ui.userCombo->setFocus();

        break;

    case ADD_USER:
        dialog->setWindowTitle( tr("Add User") );
        ui.userEdit->setFocus();
        break;

    case CHANGE_PASS:
        dialog->setWindowTitle( tr("Change Password") );
        break;
    }

    disconnect( dialogUi.buttonBox->button( QDialogButtonBox::Ok ), SIGNAL( clicked() ), dialog, SLOT( accept() ) );
    connect( dialogUi.buttonBox->button( QDialogButtonBox::Ok ), SIGNAL( clicked() ), this, SLOT( onDialogOk() ) );
    connect( dialogUi.buttonBox, SIGNAL( rejected() ), dialog, SLOT( reject() ) );
    connect( dialog, SIGNAL( accepted() ), this, SLOT( save() ) );
    connect( this, SIGNAL( verifySuccess() ), dialog, SLOT( accept() ) );
    //connect( this, SIGNAL( verifyFail() ), dialog, SLOT( reject() ) );

    return *dialog;
}


void
LoginWidget::onDialogOk()
{
    verify();
}


bool
LoginWidget::detailsChanged()
{
    return ui.passwordEdit->text() != "********"  && !ui.passwordEdit->text().isEmpty();
}
