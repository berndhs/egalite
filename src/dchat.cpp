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
#include "simple-pass.h"
#include <QDebug>
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
   contactListModel (this),
   configEdit (this),
   helpView (this),
   subscriptionDialog (this),
   publicPort (29999),
   defaultPort (29999),
   passdial (0),
   callnum (0),
   debugTimer (0),
   xmppTimer (0),
   announceHeartbeat (0)
{
  ui.setupUi (this);
  ui.contactView->setModel (&contactListModel);
  Connect ();
  debugTimer = new QTimer (this);
  connect (debugTimer, SIGNAL (timeout()), this, SLOT (DebugCheck()));
  debugTimer->start (10 * 1000); // 15 secs
  xmppTimer = new QTimer (this);
  connect (xmppTimer, SIGNAL (timeout()), this, SLOT (XmppPoll ()));
  xmppTimer->start (30 * 1000); // 1/2 mins
  announceHeartbeat = new QTimer (this);
  connect (announceHeartbeat, SIGNAL (timeout()), this, SLOT (AnnounceMe()));
  announceHeartbeat->start (1000*60*2); // 2 minutes
  xclientMap.clear ();
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
  CertStore::IF().Init (this);
  SetSettings ();
  contactListModel.Setup ();
  QStringList contactHeaders;
  contactHeaders << tr("Status")
                 << tr("Name")
                 << tr("Login");
  contactListModel.setHorizontalHeaderLabels (contactHeaders);
  show ();
  SetupListener ();
}

void
DChatMain::SetupListener ()
{
  QString ownAddress ("0::1");
  ownAddress = Settings().value ("direct/listenAddress",ownAddress).toString();
  Settings().setValue ("direct/listenAddress",ownAddress);

  publicPort = Settings().value ("direct/listenPort",publicPort).toInt ();
  Settings().setValue ("direct/listenPort",publicPort);

  directIdentity = Settings().value ("direct/identity",directIdentity).toString();
  Settings().setValue ("direct/identity",directIdentity);
  DirectListener * listen (0);
  if (inDirect.find (directIdentity) == inDirect.end ()) {
    listen = new DirectListener (this);
    inDirect [directIdentity] = listen;
  } else {
    listen = inDirect [directIdentity];
  }

  if (CertStore::IF().HaveCert (directIdentity)) {
    CertRecord hostCert = CertStore::IF().Cert (directIdentity);
    QString pass = hostCert.Password ();
    if (pass.length() == 0) {
      SimplePass  getPass (this);
      pass = getPass.GetPassword (tr("Listener Password:"));
    }
    QSslKey skey (hostCert.Key().toAscii(),QSsl::Rsa,
                QSsl::Pem, QSsl::PrivateKey, pass.toUtf8());
    QSslCertificate scert (hostCert.Cert().toAscii());
    listen->Init (directIdentity, skey, scert);
    listen->Listen (QHostAddress (ownAddress),publicPort);
qDebug () << " listen at " << QHostAddress (ownAddress) << " port " << publicPort;
  } else {
qDebug () << " cannot listen for identity " << directIdentity;
  }
  connect (listen, SIGNAL (Receive (const QByteArray &)),
           this, SLOT (GetRaw (const QByteArray&)));
  connect (listen, SIGNAL (SocketReady (SymmetricSocket *, QString)),
           this, SLOT (ConnectDirect (SymmetricSocket *, QString)));

}

void
DChatMain::SetSettings ()
{
  user = Settings().value ("network/user", user).toString();
  Settings().setValue ("network/user",user);
  server = Settings().value ("network/server",server).toString();
  Settings().setValue ("network/server",server);

  defaultPort = Settings().value ("direct/defaultPort",defaultPort).toInt ();
  Settings().setValue ("direct/defaultPort",defaultPort);
  
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
           this, SLOT (EditSettings ()));
  connect (ui.actionLog_In, SIGNAL (triggered()),
           this, SLOT (Login()));
  connect (ui.actionLog_Out, SIGNAL (triggered()),
           this, SLOT (Logout()));
  connect (ui.actionDirect, SIGNAL (triggered()),
           CertStore::Object(), SLOT (CertDialog ()));
  connect (ui.contactDirectAction, SIGNAL (triggered()),
           CertStore::Object(), SLOT (ContactDialog ()));
  connect (ui.contactView, SIGNAL (activated (const QModelIndex &)),
           &contactListModel, SLOT (PickedItem (const QModelIndex &)));
  connect (ui.actionLicense, SIGNAL (triggered()),
           this, SLOT (License()));
  connect (ui.actionManual, SIGNAL (triggered()),
           this, SLOT (Manual ()));
  connect (ui.actionAbout, SIGNAL (triggered()),
           this, SLOT (About ()));
  connect (&contactListModel, SIGNAL (StartServerChat (QString)),
           this, SLOT (StartServerChat (QString)));
}

void
DChatMain::EditSettings ()
{
  configEdit.Exec ();
  SetSettings ();
}

void
DChatMain::Login ()
{
  QString oldUser = user;
  if (GetPass()) {
    XEgalClient * xclient = xclientMap[user];
qDebug () << " old user " << oldUser << " new user " << user << " xclient " << xclient;
    if (!xclient) {
      xclient = new XEgalClient (this);
      contactListModel.AddAccount (user);
      xclientMap[user] = xclient;
    }
    QXmppConfiguration & xconfig = xclient->getConfiguration();
    xconfig.setResource ("Egalite.");
    xclient->connectToServer (server,user, password);
    xmppUser = user;
    connect (xclient, SIGNAL (messageReceived  (const QXmppMessage  &)),
             this, SLOT (GetMessage (const QXmppMessage &)));
    connect (xclient, SIGNAL (error (XEgalClient::Error)),
             this, SLOT (XmppError (XEgalClient::Error)));
    connect (xclient, SIGNAL (presenceReceived (const QXmppPresence &)),
             this, SLOT (XPresenceChange (const QXmppPresence &)));
    connect (xclient, SIGNAL (connected ()), 
             this, SLOT (XmppConnected ()));
    connect (xclient, SIGNAL (disconnected ()),
             this, SLOT (XmppDisconnected ()));
    connect (xclient, SIGNAL (elementReceived (const QDomElement &, bool &)),
             this, SLOT (XmppElementReceived (const QDomElement &, bool &)));
    connect (xclient, SIGNAL (iqReceived (const QXmppIq &)),
             this, SLOT (XmppIqReceived (const QXmppIq &)));
    connect (xclient, SIGNAL (discoveryIqReceived (const QXmppDiscoveryIq &)),
             this, SLOT (XmppDiscoveryIqReceived (const QXmppDiscoveryIq &)));
    Poll (xclient);
  }
}

void
DChatMain::License ()
{
  helpView.Show ("qrc:/help/LICENSE.txt");
}

void
DChatMain::Manual ()
{
  helpView.Show ("qrc:/help/helpman.html");
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
                      QString ("Egalite!"));
  QXmppPresence pres (QXmppPresence::Available, status);
  if (xclientMap[user]) {
    xclientMap[user]->setClientPresence (pres);
  }
}

bool
DChatMain::GetPass ()
{
  if (passdial == 0) {
    passdial = new QDialog (this);
    passui.setupUi (passdial);
  }
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
qDebug () << " GetPass new     +++++++++++++++++++++ ";
qDebug () << " new user " << user << " server " << server;
qDebug () << " userText " << passui.userText->text ();
qDebug () << " serverText " << passui.serverText->text ();
    return true;
  } else {
qDebug () << " GetPass no change ------------------- ";
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
DChatMain::XmppError (XEgalClient::Error err)
{
  switch (err) {
  case XEgalClient::SocketError: 
    qDebug () << " xmpp socket error";
    break;
  case XEgalClient::KeepAliveError:
    qDebug () << " xmpp keep alive arror";
    break;
  case XEgalClient::XmppStreamError:
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
  if (body.length() == 0) {
    return;
  }
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
  if (xclientMap[user]) {
    QStringList parts = outMsg.to().split("/");
    outMsg.setTo (parts.at(0));
    xclientMap[user]->sendMessage (outMsg.to(), outMsg.body());
  }
}

/** \brief CallDirect - set up a direct connection */
void
DChatMain::CallDirect ()
{
  PickString     pickString (this);
  QStringList    choiceList = CertStore::IF().NameList();
  pickString.SetTitle (tr("Choose Direct Identity"));
  int choice = pickString.Pick (choiceList);
  if (choice != 1) {
    return;
  }
  QString  originNick = pickString.Choice ();
  CertRecord outCert = CertStore::IF().Cert (originNick);
  
  choiceList = CertStore::IF().ContactList ();
  pickString.SetTitle (tr("Choose Destination"));
  choice = pickString.Pick (choiceList);
  if (choice != 1) {
    return;
  }
  QString dest = pickString.Choice ();
  QString destaddr = CertStore::IF().ContactAddress (dest);
  int destPort = CertStore::IF().ContactPort (dest);
  if (destPort == 0) {
    destPort = defaultPort;
  }
  callnum++;
  DirectCaller * newcall = new DirectCaller (this);

  if (newcall) {
    newcall->Setup (outCert, destPort, originNick);
    outDirect[callnum] = newcall;
qDebug () << " start direct connect " << callnum << " call " << newcall;
    newcall->ConnectAddress (destaddr, dest, callnum);
    connect (newcall, SIGNAL (Finished (int)), this, SLOT (ClearCall (int)));
    connect (newcall, SIGNAL (ConnectionReady (SymmetricSocket*, QString)),
             this, SLOT (ConnectDirect (SymmetricSocket*, QString)));
  }
}

void
DChatMain::Logout ()
{
  PickString   pickString (this);
  QStringList  choiceList;
  std::map <QString, XEgalClient*>::iterator xit;
  for (xit = xclientMap.begin(); xit != xclientMap.end(); xit++) {
    choiceList << xit->first;
  }
  pickString.SetTitle (tr("Choose Account to Log Out"));
  int choice = pickString.Pick (choiceList);
  if (choice != 1) {
    return;
  }
  QString expired = pickString.Choice();
  contactListModel.RemoveAccount (expired);
  XEgalClient * xclient = xclientMap [expired];
  if (xclient) {
    xclient->disconnect ();
    disconnect (xclient, 0,0,0);
    delete xclient;
  }
  xclientMap.erase (expired);
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
    connect (newCont, SIGNAL (Disconnect (QString)),
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
  connect (newCont, SIGNAL (Disconnect(QString)),
            this, SLOT (CloseServerChat (QString)));
  newChat->Run ();
}

void
DChatMain::CloseServerChat (QString remoteName)
{
  if (serverChats.find(remoteName) != serverChats.end()) {
    serverChats [remoteName]->Close();
    serverChats.erase (remoteName);
  }
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


QString
DChatMain::PresenceTypeString (QXmppPresence::Type t)
{
  switch (t) {
    case QXmppPresence::Error: 
      return tr ("pres-error");
    case QXmppPresence::Available:
      return tr ("Available");
    case QXmppPresence::Unavailable:
      return tr ("Not Available");
    case QXmppPresence::Subscribe:
      return tr ("Want to Subscribed");
    case QXmppPresence::Subscribed:
      return tr ("Done Subscribed");
    case QXmppPresence::Unsubscribe:
      return tr ("Want to Un-Subscribe");
    case QXmppPresence::Unsubscribed:
      return tr ("Done Un-Subscribed");
    case QXmppPresence::Probe:
      return tr ("Probe");
    default:
      return QString ("invalid type");
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
DChatMain::XmppPoll ()
{
  std::map <QString, XEgalClient*> :: iterator mapit;
  for (mapit = xclientMap.begin (); mapit != xclientMap.end (); mapit++) {
    Poll (mapit->second);
  }
}


void
DChatMain::DebugCheck ()
{
  if (!xclientMap[user]) {
    qDebug () <<" DEBUG: no xclient";
  }
}

void
DChatMain::XPresenceChange (const QXmppPresence & presence)
{
  QXmppPresence::Type   presType = presence.type ();
  QXmppPresence::Status status = presence.status();
qDebug () << " receive presence form " << presence.from () << " to "
          << presence.to () << " type " << PresenceTypeString (presType);
  if (presType == QXmppPresence::Available 
      || presType == QXmppPresence::Unavailable) {
    contactListModel.UpdateState (presence.to(), presence.from(), status);
  } else {
    subscriptionDialog.RemoteAskChange (xclientMap[user], presence);
  }
}

void
DChatMain::XmppDiscoveryIqReceived (const QXmppDiscoveryIq & disIq)
{
  Q_UNUSED (disIq);
  QXmppIq::Type  iqtype = disIq.type ();
  QString msg;
  switch (iqtype) {
  case QXmppIq::Error:
     msg = "Error";
     break;
  case QXmppIq::Get:
     msg = "Get";
     break;
   case QXmppIq::Set:
     msg = "Set";
     break;
   case QXmppIq::Result:
     msg = "Result";
     break;
   default:
     msg = "Bad IQ type";
  }
  qDebug () << " received Discovery Iq type " << msg;
}
void
DChatMain::XmppElementReceived (const QDomElement & elt, bool & handled)
{
  qDebug () << " received DOM elt from xclient ";
  qDebug () << elt.tagName();
  handled = false;
}

void
DChatMain::XmppIqReceived (const QXmppIq & iq)
{
  Q_UNUSED (iq);
  qDebug () << " received IQ ";
}


void
DChatMain::XmppConnected ()
{
  XmppPoll ();
  AnnounceMe ();
}

void
DChatMain::XmppDisconnected ()
{
  qDebug () << " xmpp client disconnected";
}


void
DChatMain::Poll (XEgalClient * xclient)
{
  QStringList  contactJids;
  if (xclient == 0) {
    return;
  }
  contactJids = xclient->getRoster().getRosterBareJids();
  xmppConfig = xclient->getConfiguration ();
  QStringList::const_iterator stit;
  QString thisUser = xmppConfig.jidBare ();

  for (stit = contactJids.begin (); stit != contactJids.end (); stit++) {
    QString id = *stit;
    QStringList resources = xclientMap[user]->getRoster().getResources (id);
    QString res;
    QStringList::const_iterator   rit;
    if (resources.size () == 0) {
      contactListModel.UpdateState (thisUser, id, 
                                  QXmppPresence::Status::Offline);
    }
    for (rit = resources.begin (); rit != resources.end (); rit++) {
      res = *rit;
      QString bigId = id + QString("/") + res;
      QXmppPresence pres = xclient->getRoster().getPresence (id,res);
      QXmppPresence::Status status = pres.status();
      contactListModel.UpdateState (thisUser, bigId, status);
    } 
  }
}



} // namespace

