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
#include "ui_dchat.h"
#include <QMainWindow>
#include <QXmppConfiguration.h>
#include <QXmppPresence.h>
#include <QXmppIq.h>
#include <QXmppDiscoveryIq.h>
#include <QDomElement>
#include "contact-list-model.h"

#include "config-edit.h"
#include "cert-store.h"
#include "server-contact.h"
#include "helpview.h"
#include "subscription-change.h"
#include "xegal-client.h"
#include "ui_getpassword.h"

#include <map>

class QTimer;
class QApplication;
class QModelIndex;

namespace egalite 
{

class DirectListener;
class DirectCaller;
class SymmetricSocket;
class ChatBox;

typedef  std::map <QString, ChatBox*>         ChatMap;

/** \brief The main messenger/chat application.
  */

class DChatMain : public QMainWindow 
{
Q_OBJECT

public:

  DChatMain (QWidget * parent = 0);

  void Init (QApplication *pap);

  void Run ();

public slots:

  void Quit ();
  void GetMessage (const QXmppMessage  & msg);
  void GetRaw (const QByteArray &data);
  void Send (const QXmppMessage & msg);
  void XPresenceChange (const QXmppPresence & 	presence );

private slots:

  void About ();
  void License ();
  void Manual ();
  void PassOK ();
  void PassCancel ();
  void Login ();
  void Logout ();
  void EditSettings ();
  void CallDirect ();
  void ClearCall (int callid);
  void ConnectDirect (SymmetricSocket *direct, QString localNick);
  void StartServerChat (QString remoteName);
  void CloseServerChat (QString remoteName);
  void ClearDirect (SymmetricSocket *direct);
  void XmppPoll ();
  void DebugCheck ();
  void XmppError (QXmppClient::Error err);
  void AnnounceMe ();
  void XmppConnected ();
  void XmppDisconnected ();
  void XmppElementReceived (const QDomElement & elt, bool & handled);
  void XmppIqReceived (const QXmppIq & iq);
  void XmppDiscoveryIqReceived (const QXmppDiscoveryIq & disIq);
  void ExpandAccountView (QModelIndex accountIndex);

private:

  void    SetSettings ();
  void    SetupListener ();
  void    Connect ();
  bool    GetPass ();
  void    Poll (XEgalClient * xclient);
  void    CallDirectFrom (QString nick);
  void    ResetContactSeen (ContactMap & contacts);
  void    FlushStaleContacts (ContactMap & contacts,
                              QStandardItemModel & model);
  QString PresenceTypeString (QXmppPresence::Type t);

  Ui_DChatMain    ui;
  Ui_GetString  passui;
  QApplication   *pApp;

  ContactListModel  contactListModel;

  ConfigEdit            configEdit;
  deliberate::HelpView  helpView;
  SubscriptionChange    subscriptionDialog;

  QXmppConfiguration  xmppConfig;
  int           publicPort;
  int           defaultPort;
  QString       directIdentity;
  QString       user;
  QString       xmppUser;
  QString       server;
  QString       password;
  QDialog      *passdial;
  int           callnum;
  QTimer       *debugTimer;
  QTimer       *xmppTimer;
  QTimer       *announceHeartbeat;


  std::map <QString, XEgalClient*>    xclientMap;
  std::map <QString, DirectListener*> inDirect;
  std::map <int, DirectCaller*>       outDirect;

  ChatMap directChats;
  ChatMap serverChats;

};

} // namespace

#endif
