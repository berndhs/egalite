#ifndef DCHAT_H
#define DCHAT_H

/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2010, Bernd Stramm
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <QDeclarativeView>
#include <QDeclarativeItem>
#include <QXmppConfiguration.h>
#include <QXmppPresence.h>
#include <QXmppIq.h>
#include <QXmppDiscoveryIq.h>
#include <QDomElement>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QEvent>

#include "xcontact-model.h"

#include "config-edit.h"
#include "cert-store.h"
#include "server-contact.h"
#include "helpview.h"
#include "subscription-change.h"
#include "xegal-client.h"
#include "ui_getpassword.h"
#include "ui_request-subscribe.h"
#include "account-edit.h"
#include "cert-list-edit.h"

#include <map>

class QTimer;
class QApplication;
class QModelIndex;
class QMenu;

namespace egalite 
{

class DirectListener;
class DirectCaller;
class SymmetricSocket;
class ChatBox;

typedef  std::map <QString, ChatBox*>         ChatMap;

/** \brief The main messenger/chat application.
  */

class DChatMain : public QDeclarativeView 
{
Q_OBJECT

public:

  DChatMain (QWidget * parent = 0);

  void Init (QApplication *pap, bool phone=false);

  void Run ();

  void AddConfigMessage (QString msg) { configMessages.append (msg); }
  void AddConfigMessages (QStringList ml) { configMessages.append (ml); }

public slots:

  void Quit ();
  void GetMessage (const QXmppMessage  & msg);
  void GetRaw (const QByteArray &data);
  void Show ();
  void hide ();
  void ShowTrayMessage (const QString & msg);

private slots:

  void About ();
  void License ();
  void Manual ();
  void PassOK ();
  void PassCancel ();
  void Login ();
  void Logout ();
  void ListenerAdd ();
  void ListenerDrop ();
  void EditSettings ();
  void EditServerLogin ();
  void RequestSubscribe ();
  void DoRequestSubscribe ();
  void CallDirect ();
  void ClearCall (int callid);
  void ConnectDirect (SymmetricSocket *direct, QString localNick);
  void StartServerChat (const QString & remoteName, const QString & login);
  void CloseServerChat (QString remoteName);
  void ClearDirect (SymmetricSocket *direct);
  void XmppPoll ();
  void StatusUpdate ();
  void DebugCheck ();
  void Send (const QXmppMessage & msg);
  void XmppError (QXmppClient::Error err);
  void AnnounceMe ();
  void XmppConnected ();
  void XmppDisconnected ();
  void XmppElementReceived (const QDomElement & elt, bool & handled);
  void XChangeRequest (QString name, QXmppPresence presence);
  void XUpdateState (QString remoteName, 
                     QString ownId,
                     QString remoteId,
                     QXmppPresence::Status status);
  void XmppIqReceived (const QXmppIq & iq);
  void XmppDiscoveryIqReceived (const QXmppDiscoveryIq & disIq);
  void ExpandAccountView (QModelIndex accountIndex);

  void TrayActivated(QSystemTrayIcon::ActivationReason);
  void TrayMenu ();
  void TrayMessageClicked ();

protected:

  void closeEvent (QCloseEvent * event);
  bool event (QEvent *event);

private:

  void    SetSettings ();
  void    SetupListener ();
  void    Connect ();
  void    SetupToolbar ();
  bool    GetPass ();
  void    Poll (XEgalClient * xclient);
  void    CallDirectFrom (QString nick);
  bool    PickServerAccount (QString &jid, QString &server, QString &pass);
  QString PresenceTypeString (QXmppPresence::Type t);
  void    StartListener (QString ownAddress, 
                         QString directIdentity, 
                         int     publicPort);
  void    CreateSystemTrayStuff ();
  
  QString genResourceTag ();

  Ui_GetString          passui;
  Ui_RequestSubscribe   reqSubUi;
  QApplication         *pApp;
  QPoint                oldPos;
  bool                  haveOldPos;

  QStringList           configMessages;

  XContactModel         xcontactModel;

  ConfigEdit            configEdit;
  deliberate::HelpView  helpView;
  SubscriptionChange    subscriptionDialog;
  AccountEdit           serverAccountEdit;
  CertListEdit          certListEdit;

  QXmppConfiguration  xmppConfig;
  int           publicPort;
  int           defaultPort;
  QString       directIdentity;
  QString       user;
  QString       xmppUser;
  QString       server;
  QString       password;
  QDialog      *passdial;
  QDialog      *subscribeDial;
  int           callnum;
  QTimer       *debugTimer;
  QTimer       *xmppTimer;
  QTimer       *announceHeartbeat;
  QTimer       *statusTimer;
  int           directHeartPeriod;
  bool          isPhone;
  QSystemTrayIcon  *trayIcon;
  QMenu            *trayMenu;
  
  QDeclarativeItem *qmlRoot;


  std::map <QString, XEgalClient*>    xclientMap;
  std::map <QString, DirectListener*> inDirect;
  std::map <int, DirectCaller*>       outDirect;

  ChatMap directChats;
  ChatMap serverChats;

};

} // namespace

#endif
