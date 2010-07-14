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
#include "version.h"
#include <QDebug>
#include "ui_getpassword.h"
#include <QXmppConfiguration.h>
#include <QXmppMessage.h>
#include <QXmppRoster.h>
#include <QXmppPresence.h>
#include <QString>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>
#include <QModelIndex>
#include <set>

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
   helpView (this),
   xclient (0),
   publicPort (29999),
   passdial (0),
   callnum (0),
   debugTimer (0),
   xmppTimer (0),
   announceHeartbeat (0)
{
  ui.setupUi (this);
  ui.contactView->setModel (&contactModel);
  Connect ();
  debugTimer = new QTimer (this);
  connect (debugTimer, SIGNAL (timeout()), this, SLOT (DebugCheck()));
  debugTimer->start (10 * 1000); // 15 secs
  xmppTimer = new QTimer (this);
  connect (xmppTimer, SIGNAL (timeout()), this, SLOT (XmppPoll ()));
  xmppTimer->start (15 * 1000); // 15 secs
  announceHeartbeat = new QTimer (this);
  connect (announceHeartbeat, SIGNAL (timeout()), this, SLOT (AnnounceMe()));
  announceHeartbeat->start (1000*60*2); // 2 minutes
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
  
  QString directHost ("reflect");
  directHost = Settings().value ("direct/host",directHost).toString();
  Settings().setValue ("direct/host",directHost);
  DirectListener * listen = new DirectListener (this);
  inDirect[directHost] = listen;
  QString ownAddress ("0::1");
  ownAddress = Settings().value ("direct/address",ownAddress).toString();
  Settings().setValue ("direct/address",ownAddress);
  if (certStore.HaveCert (directHost)) {
    CertRecord hostCert = certStore.Cert (directHost);
    QString pass ("enkhuizen");
    QSslKey key (hostCert.Key().toAscii(),QSsl::Rsa,
                QSsl::Pem, QSsl::PrivateKey, pass.toUtf8());
    QSslCertificate scert (hostCert.Cert().toAscii());
    listen->Init (directHost, key, scert);
    listen->Listen (QHostAddress (ownAddress),publicPort);
  }
  connect (listen, SIGNAL (Receive (const QByteArray &)),
           this, SLOT (GetRaw (const QByteArray&)));
  connect (listen, SIGNAL (SocketReady (SymmetricSocket *, QString)),
           this, SLOT (ConnectDirect (SymmetricSocket *, QString)));
  QStringList contactHeaders;
  contactHeaders << tr("Status")
                 << tr("Name")
                 << tr("Login");
  contactModel.setHorizontalHeaderLabels (contactHeaders);
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
  connect (ui.actionLicense, SIGNAL (triggered()),
           this, SLOT (License()));
  connect (ui.actionManual, SIGNAL (triggered()),
           this, SLOT (Manual ()));
  connect (ui.actionAbout, SIGNAL (triggered()),
           this, SLOT (About ()));
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
    connect (xclient, SIGNAL (presenceReceived (const QXmppPresence &)),
             this, SLOT (XPresenceChange (const QXmppPresence &)));
    QTimer::singleShot (3000, this, SLOT (XmppPoll()));
    QTimer::singleShot (2500, this, SLOT (AnnounceMe ()));
  }
}

void
DChatMain::License ()
{
  helpView.Show ("qrc:/LICENSE.txt");
}

void
DChatMain::Manual ()
{
  helpView.Show ("qrc:/helpman.html");
}

void
DChatMain::About ()
{
  deliberate::ProgramVersion::ShowVersionWindow ();
}


void
DChatMain::AnnounceMe ()
{
  QXmppPresence::Status status (QXmppPresence::Status::Online,
                      QString ("testing Egalite"));
  QXmppPresence pres (QXmppPresence::Available, status);
  if (xclient) {
    xclient->sendPacket (pres);
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
    connect (newCont, SIGNAL (Activity (QWidget*)),
             newChat, SLOT (WidgetActivity (QWidget*)));
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
  connect (newCont, SIGNAL (Activity (const QWidget*)),
           newChat, SLOT (WidgetActivity (const QWidget*)));
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
    if (chase->second->WidgetIndex (sock->Dialog()) >= 0) {
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
DChatMain::XPresenceChange (const QXmppPresence & presence)
{
  qDebug () << " someone came or left";
  QString remoteId = presence.from ();
  QXmppPresence::Status status = presence.status();
  QXmppPresence::Status::Type stype = status.type ();
  QString statusText = status.statusText (); 
  QStringList parts = remoteId.split ('/',QString::SkipEmptyParts);
  QString id = parts.at(0);
  QString resource;
  if (parts.size () > 1) {
    resource = parts.at(1);
  }
  ContactMap::iterator contactit = serverContacts.find (remoteId);
  if (contactit != serverContacts.end()) {
    if (contactit->second) {
      SetStatus (contactit->second->modelRow, stype, statusText);
      contactit->second->recentlySeen = true;
    }
  } else {
    AddContact (id, resource, stype, statusText);
  }
}

void
DChatMain::ResetContactSeen (ContactMap & contacts)
{
  ContactMap::iterator  contactit;
  for (contactit = contacts.begin (); 
       contactit != contacts.end ();
       contactit ++) {
    if (contactit->second) {
      contactit->second->recentlySeen = false;
    }
  }
}

void
DChatMain::FlushStaleContacts (ContactMap & contacts, 
                               QStandardItemModel & model)
{
  // determine outdated rows and remove them
  std::set <int> staleRows;
  std::set <QString> staleContacts;
  ContactMap::iterator  contactit;
  for (contactit = contacts.begin (); 
       contactit != contacts.end ();
       contactit ++) {
    if (contactit->second) {
      if (!(contactit->second->recentlySeen)) {
        staleRows.insert (contactit->second->modelRow);
        staleContacts.insert (contactit->second->name 
                              + QString("/")
                              + contactit->second->resource);
      }
    }
  }  
  std::set <int>::const_reverse_iterator  rowrit;
  for (rowrit = staleRows.rbegin(); rowrit != staleRows.rend(); rowrit++) {
    model.removeRows (*rowrit, 1);
  }
  // recalculate row numbers
  int nrows = model.rowCount();
  QStandardItem * nameItem, * resourceItem (0);
  for (int r=0; r<nrows; r++) {
    nameItem = model.item (r,1);
    resourceItem = model.item (r,2);
    if (nameItem && resourceItem) {
      QString bigId = nameItem->text () + QString ("/") + resourceItem->text();
      contactit = contacts.find (bigId);
      if (contactit != contacts.end()) {
        contactit->second->modelRow = r;
      }
    }
  }
  std::set <QString>::const_iterator bigIdIt;
  for (bigIdIt = staleContacts.begin (); bigIdIt != staleContacts.end();
       bigIdIt++) {
    contacts.erase (*bigIdIt);
  }
}

void 
DChatMain::SetStatus (int row, 
                      QXmppPresence::Status::Type stype,
                      QString statusText)
{
  QStandardItem * stateItem = contactModel.item (row,0);
  if (stateItem == 0) {
    stateItem = new QStandardItem;
    contactModel.setItem (row,0,stateItem);
  }
  if (statusText.length() == 0) {
    stateItem->setText (StatusName (stype));
  } else {
    stateItem->setText (statusText);
  }
}

QString
DChatMain::StatusName (QXmppPresence::Status::Type stype)
{
  switch (stype) {
  case QXmppPresence::Status::Offline: 
    return tr ("-");
  case QXmppPresence::Status::Online:
    return tr ("On");
  case QXmppPresence::Status::Away:
    return tr ("away");
  case QXmppPresence::Status::XA:
    return tr ("XA");
  case QXmppPresence::Status::DND:
    return tr ("DND");
  case QXmppPresence::Status::Chat:
    return tr ("Chatty");
  case QXmppPresence::Status::Invisible:
    return tr ("Hiding");
  default:
    return tr ("?");
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
  ResetContactSeen (serverContacts);
  QStringList::const_iterator stit;
  for (stit = contactJids.begin (); stit != contactJids.end (); stit++) {
    QString id = *stit;
    QStringList resources = xclient->getRoster().getResources (id);
    QString res;
    QStringList::const_iterator   rit;
    for (rit = resources.begin (); rit != resources.end (); rit++) {
      res = *rit;
      QString bigId = id + QString("/") + res;
      QXmppPresence pres = xclient->getRoster().getPresence (id,res);
      QXmppPresence::Status::Type stype = pres.status().type ();

      ContactMap::iterator contactit = serverContacts.find (bigId);
      if (contactit == serverContacts.end ()) {
        AddContact (id, res, stype, pres.status().statusText());
      } else {
        SetStatus (contactit->second->modelRow, 
                   stype, pres.status().statusText());
        contactit->second->recentlySeen = true;
      }
    } 
  }
  FlushStaleContacts (serverContacts, contactModel);
}

void
DChatMain::AddContact (QString id, 
                       QString res, 
                       QXmppPresence::Status::Type stype,
                       QString statusText)
{
  ServerContact * newContact = new ServerContact;
  newContact->name = id;
  newContact->state = QString("?");
  newContact->resource = res;
  newContact->recentlySeen = true;
  QStandardItem * nameItem = new QStandardItem (newContact->name);
  QStandardItem * stateItem = new QStandardItem (newContact->state);
  QStandardItem * resourceItem = new QStandardItem (newContact->resource);
  QList <QStandardItem*> row;
  row << stateItem;
  row << nameItem;
  row << resourceItem;
  contactModel.appendRow (row);
  newContact->modelRow = stateItem->row ();
  if (statusText.length() == 0) {
    stateItem->setText (StatusName (stype));
  } else {
    stateItem->setText (statusText);
  }
  QString  bigId = id + QString("/") + res;
  serverContacts [bigId] = newContact;
}

void
DChatMain::DebugCheck ()
{
  if (!xclient) {
    qDebug () <<" DEBUG: no xclient";
  }
}

} // namespace

