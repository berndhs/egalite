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

#include <iostream>
#include "dchat.h"
#include "delib-debug.h"
#include "deliberate.h"
#include <QDebug>
#include "ui_getpassword.h"
#include <QXmppConfiguration.h>
#include <QXmppMessage.h>
#include <QXmppRoster.h>
#include <QString>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>
#include <QModelIndex>

#include "direct-listener.h"
#include "direct-caller.h"
#include "symmetric-socket.h"
#include "pick-string.h"
#include "chat-box.h"
#include "chat-content.h"
#include "server-contact.h"

using namespace deliberate;

namespace egalite {

DChatMain::DChatMain (QWidget *parent)
  :QMainWindow (parent),
   pApp (0),
   contactModel (this),
   configEdit (this),
   certStore (this),
   xclient (0),
   publicPort (29999),
   passdial (0),
   callnum (0),
   debugTimer (0),
   xmppTimer (0)
{
  ui.setupUi (this);
  ui.contactView->setModel (&contactModel);
  Connect ();
  debugTimer = new QTimer (this);
  connect (debugTimer, SIGNAL (timeout()), this, SLOT (DebugCheck()));
  debugTimer->start (15000);
  xmppTimer = new QTimer (this);
  connect (xmppTimer, SIGNAL (timeout()), this, SLOT (XmppPoll ()));
  xmppTimer->start (10000);
}

void
DChatMain::Init (QApplication *pap)
{
  pApp = pap;
}

void
DChatMain::Run ()
{
  if (Settings().contains("sizes/main")) {
    QSize defaultSize = size();
    QSize newsize = Settings().value ("sizes/main", defaultSize).toSize();
    resize (newsize);
  }
  certStore.Init ();
  user = Settings().value ("network/user", user).toString();
  Settings().setValue ("network/user",user);
  server = Settings().value ("network/server",server).toString();
  Settings().setValue ("network/server",server);

  publicPort = Settings().value ("network/publicport",publicPort).toInt ();
  Settings().setValue ("network/publicport",publicPort);
  
  QString directHost ("");
  directHost = Settings().value ("direct/host",directHost).toString();
  Settings().setValue ("direct/host",directHost);
  QString directPass ("");
  directPass = Settings().value ("direct/password",directPass).toString();
  Settings().setValue ("direct/password",directPass);
  DirectListener * listen = new DirectListener (this);
  inDirect[directHost] = listen;
  QString ownAddress ("0::1");
  ownAddress = Settings().value ("direct/address",ownAddress).toString();
  Settings().setValue ("direct/address",ownAddress);
  if (certStore.HaveCert (directHost)) {
    CertRecord hostCert = certStore.Cert (directHost);
    QSslKey key (hostCert.Key().toAscii(),QSsl::Rsa,
                QSsl::Pem, QSsl::PrivateKey, directPass.toUtf8());
    QSslCertificate scert (hostCert.Cert().toAscii());
    listen->Init (directHost, key, scert);
    listen->Listen (QHostAddress (ownAddress),publicPort);
  }
  connect (listen, SIGNAL (Receive (const QByteArray &)),
           this, SLOT (GetRaw (const QByteArray&)));
  connect (listen, SIGNAL (SocketReady (SymmetricSocket *, QString)),
           this, SLOT (ConnectDirect (SymmetricSocket *, QString)));
  show ();
}

void
DChatMain::Quit ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/main",currentSize);
  Settings().sync();
  if (pApp) {
    pApp->quit ();
  }
}

void
DChatMain::Connect ()
{
  connect (ui.quitButton, SIGNAL (clicked()), this, SLOT (Quit()));
  connect (ui.sendButton, SIGNAL (clicked()), this, SLOT (Send()));
  connect (ui.directButton, SIGNAL (clicked()), this, SLOT (CallDirect()));
  connect (ui.actionQuit, SIGNAL (triggered()), this, SLOT (Quit()));
  connect (ui.actionSettings, SIGNAL (triggered()),
           &configEdit, SLOT (Exec()));
  connect (ui.actionLog_In, SIGNAL (triggered()),
           this, SLOT (Login()));
  connect (ui.actionDirect, SIGNAL (triggered()),
           &certStore, SLOT (CertDialog ()));
  connect (ui.contactDirectAction, SIGNAL (triggered()),
           &certStore, SLOT (ContactDialog ()));
  connect (ui.contactView, SIGNAL (activated (const QModelIndex &)),
           this, SLOT (PickedItem (const QModelIndex &)));
}

void
DChatMain::Login ()
{
  if (GetPass()) {
    if (!xclient) {
      xclient = new QXmppClient (this);
    }
    xclient->connectToServer (server,user, password);
    xmppUser = user;
qDebug () << " after connect attempt: " << xclient->isConnected ();
    connect (xclient, SIGNAL (messageReceived  (const QXmppMessage  &)),
             this, SLOT (GetMessage (const QXmppMessage &)));
    connect (xclient, SIGNAL (error (QXmppClient::Error)),
             this, SLOT (XmppError (QXmppClient::Error)));
    QTimer::singleShot (3000, this, SLOT (XmppPoll()));
  }
}

bool
DChatMain::GetPass ()
{
  if (passdial == 0) {
    passdial = new QDialog (this);
  }
  Ui_GetString  passui;
  passui.setupUi (passdial);
  passui.userText->setText (user);
  passui.serverText->setText (server);
  passui.passwordText->setText ("");
  connect (passui.okButton, SIGNAL (clicked()), this, SLOT (PassOK()));
  connect (passui.cancelButton, SIGNAL (clicked()), this, SLOT (PassCancel()));
  int haveit = passdial->exec ();
  if (haveit == 1) {
    password = passui.passwordText->text ();
    user = passui.userText->text ();
    server = passui.serverText->text ();
    Settings().setValue ("network/user",user);
    Settings().setValue ("network/server",server);
    return true;
  } else {
    return false;
  }
}

void
DChatMain::PassOK ()
{
  if (passdial) {
    passdial->done (1);
  }
}

void
DChatMain::PassCancel ()
{
  if (passdial) {
    passdial->done (0);
  }
}

void
DChatMain::XmppError (QXmppClient::Error err)
{
  switch (err) {
  case QXmppClient::SocketError: 
    qDebug () << " xmpp socket error";
    break;
  case QXmppClient::KeepAliveError:
    qDebug () << " xmpp keep alive arror";
    break;
  case QXmppClient::XmppStreamError:
    qDebug () << " xmpp stream error";
    break;
  }
}

void
DChatMain::GetMessage (const QXmppMessage & msg)
{
  QString from = msg.from ();
  QString to   = msg.to ();
  QString body = msg.body ();
  QString pattern ("%1 says to %2: %3");
  QString msgtext = pattern.arg(from).arg(to).arg(body);
  qDebug () << " message from " << from << " to " << to << 
               " is " << body;
  if (serverChats.find (from) != serverChats.end()) {
    serverChats[from]->Incoming (msg);
  } else {
    StartServerChat (from);
    if (serverChats.find (from) != serverChats.end()) {
      serverChats[from]->Incoming (msg);
    }
  }
}


void
DChatMain::GetRaw (const QByteArray &data)
{
  QDomDocument msgDoc;
  msgDoc.setContent (data);
  QXmppMessage msg;
  msg.parse (msgDoc.documentElement());
  GetMessage (msg);
qDebug () << " received raw message " << data;
}

void
DChatMain::Send ()
{
  QString body = ui.inputLine->text();
  QString to ("bernd.stramm@gmail.com");
  if (xclient) {
    xclient->sendMessage (to,body);
  }
  QXmppMessage msg (user,to,body);
  QByteArray outbuf("<?xml version='1.0'>");
  QXmlStreamWriter out (&outbuf);
  msg.toXml (&out);
  std::cout << " message 1:"  << std::endl;
  std::cout << outbuf.data() << std::endl;

  QDomDocument msgdoc;
  msgdoc.setContent (outbuf);
  std::cout << " message doc string " << msgdoc.toString().toStdString() << std::endl;
  QByteArray outb2;
  QXmlStreamWriter out2 (&outb2);
  QXmppMessage msg2;
  msg2.parse (msgdoc.documentElement());
  msg2.toXml (&out2);
  std::cout << " message 2: " << std::endl;
  std::cout << outb2.data() << std::endl;
}

void
DChatMain::Send (const QXmppMessage & msg)
{
qDebug () << " DChat Main send xmpp";
  QXmppMessage outMsg (msg);
  if (xclient) {
    QStringList parts = outMsg.to().split("/");
    outMsg.setTo (parts.at(0));
    xclient->sendMessage (outMsg.to(), outMsg.body());
  }
}

/** \brief CallDirect - set up a direct connection */
void
DChatMain::CallDirect ()
{
  PickString     pickString (this);
  QStringList    choiceList = certStore.NameList();
  pickString.SetTitle (tr("Choose Direct Identity"));
  int choice = pickString.Pick (choiceList);
  if (choice != 1) {
    return;
  }
  QString  originNick = pickString.Choice ();
  CertRecord outCert = certStore.Cert (originNick);
  
  choiceList = certStore.ContactList ();
  pickString.SetTitle (tr("Choose Destination"));
  choice = pickString.Pick (choiceList);
  if (choice != 1) {
    return;
  }
  QString dest = pickString.Choice ();
  QString destaddr = certStore.ContactAddress (dest);
  callnum++;
  DirectCaller * newcall = new DirectCaller (this);

  if (newcall) {
    newcall->Setup (outCert, publicPort, originNick);
    outDirect[callnum] = newcall;
qDebug () << " start direct connect " << callnum << " call " << newcall;
    newcall->ConnectAddress (destaddr, dest, callnum);
    connect (newcall, SIGNAL (Finished (int)), this, SLOT (ClearCall (int)));
    connect (newcall, SIGNAL (ConnectionReady (SymmetricSocket*, QString)),
             this, SLOT (ConnectDirect (SymmetricSocket*, QString)));
  }
}

void
DChatMain::ConnectDirect (SymmetricSocket * sock, QString localNick)
{
qDebug () << " have connection with " << sock;
Q_UNUSED (localNick);
  if (sock) {
    QString other = sock->PeerName();
    ChatBox * newChat = new ChatBox (this);
    newChat->SetTitle (tr("direct ") + localNick);
    newChat->Add (sock->Dialog(),tr("Status")); 
    ChatContent * newCont = new ChatContent (this);
    newCont->SetMode (ChatContent::ChatModeRaw);
    newCont->SetRemoteName (sock->RemoteName());
    newCont->SetLocalName (sock->LocalName());
    newChat->Add (newCont,tr("Chat"));
    directChats [other] = newChat;
    connect (newCont, SIGNAL (Outgoing (const QByteArray&)),
             sock, SLOT (SendData (const QByteArray&)));
    connect (sock, SIGNAL (ReceiveData (const QByteArray&)),
             newCont, SLOT (Incoming (const QByteArray&)));
    connect (newCont, SIGNAL (Disconnect ()),
             sock, SLOT (Close()));
    connect (sock, SIGNAL (Exiting(SymmetricSocket *)), 
             this, SLOT (ClearDirect (SymmetricSocket *)));
    newChat->Run ();
  }
}

void
DChatMain::StartServerChat (QString remoteName)
{
  ChatBox * newChat = new ChatBox (this);
  newChat->SetTitle (tr("Xmpp ") + remoteName);
  ChatContent * newCont = new ChatContent (this);
  newCont->SetMode (ChatContent::ChatModeXmpp);
  newCont->SetRemoteName (remoteName);
  newCont->SetLocalName (xmppUser);
  newChat->Add (newCont, tr("Chat"));
  serverChats [remoteName] = newChat;
  connect (newCont, SIGNAL (Outgoing (const QXmppMessage&)),
           this, SLOT (Send (const QXmppMessage&)));
  connect (newChat, SIGNAL (HandoffIncoming (const QXmppMessage&)),
            newCont, SLOT (Incoming (const QXmppMessage&)));
  connect (newCont, SIGNAL (Disconnect()),
            newChat, SLOT (Close ()));
  newChat->Run ();
}

void
DChatMain::ClearDirect (SymmetricSocket * sock)
{
  if (sock == 0) {
    return;
  }
  if (sock->Dialog() == 0) {
    return;
  }
  std::map <QString, ChatBox *>::iterator chase, foundit;
  foundit = directChats.end();
  for (chase = directChats.begin (); chase != directChats.end(); chase++) {
    if (chase->second->HaveWidget (sock->Dialog())) {
      foundit = chase;
      break;
    }
  }
  if (foundit != directChats.end()) {
    ChatBox *deadChat = foundit->second;
    directChats.erase (foundit);
    deadChat->Close ();
  }
}


void
DChatMain::ClearCall (int callid)
{
  std::map <int,DirectCaller*>::iterator callit;
  callit = outDirect.find (callid);
  if (callit != outDirect.end()) {
    outDirect.erase (callit);
  }
}

void
DChatMain::PickedItem (const QModelIndex &index)
{
  qDebug () << " picked model item " << index;
  QMap <int, QVariant> itemData = contactModel.itemData (index);
  qDebug () << " item data: " << itemData;
  int row = index.row ();
  QStandardItem *nameCell = contactModel.item (row,1);
  QStandardItem *resoCell = contactModel.item (row,2);
  if (nameCell) {
    QString  target (nameCell->text());
    if (resoCell) {
      target.append ("/");
      target.append (resoCell->text());
    }
    StartServerChat (target);
  }
}

void
DChatMain::XmppPoll ()
{
  QStringList  contactJids;
  if (xclient == 0) {
    return;
  }
  contactJids = xclient->getRoster().getRosterBareJids();
  xmppConfig = xclient->getConfiguration ();

  QStringList::const_iterator stit;
  for (stit = contactJids.begin (); stit != contactJids.end (); stit++) {
    QString id = *stit;
    QStringList resources = xclient->getRoster().getResources (id);
    QString res;
    QStringList::const_iterator   rit;
    for (rit = resources.begin (); rit != resources.end (); rit++) {
      res = *rit;
      QString bigId = id + QString("/") + res;
      if (serverContacts.find (bigId) == serverContacts.end ()) {
        ServerContact * newContact = new ServerContact;
        newContact->name = id;
        newContact->state = QString("?");
        newContact->resource = res;
        QStandardItem * nameItem = new QStandardItem (newContact->name);
        QStandardItem * stateItem = new QStandardItem (newContact->state);
        QStandardItem * resourceItem = new QStandardItem (newContact->resource);
        QList <QStandardItem*> row;
        row << stateItem;
        row << nameItem;
        row << resourceItem;
        contactModel.appendRow (row);
        serverContacts [bigId] = newContact;
      }
    }
    
  }
}

void
DChatMain::DebugCheck ()
{
  if (!xclient) {
    qDebug () <<" DEBUG: no xclient";
  }
}

} // namespace

