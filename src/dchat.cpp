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
#include <QDeclarativeContext>
#include <QXmppConfiguration.h>
#include <QXmppMessage.h>
#include <QXmppRosterManager.h>
#include <QXmppRosterIq.h>
#include <QXmppPresence.h>
#include <QString>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>
#include <QModelIndex>
#include <QXmlStreamWriter>
#include <QDateTime>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <set>
#include <QUuid>

#include "direct-listener.h"
#include "direct-caller.h"
#include "symmetric-socket.h"
#include "pick-string.h"
#include "chat-box.h"
#include "chat-content.h"
#include "server-contact.h"
#include "add-listener.h"

using namespace deliberate;

namespace egalite {

DChatMain::DChatMain (QWidget *parent)
  :QDeclarativeView (parent),
   pApp (0),
   haveOldPos (false),
   xcontactModel (this),
   configEdit (this),
   helpView (this),
   subscriptionDialog (this),
   serverAccountEdit (this),
   certListEdit (this),
   publicPort (29999),
   defaultPort (29999),
   passdial (0),
   subscribeDial (0),
   callnum (0),
   debugTimer (0),
   xmppTimer (0),
   announceHeartbeat (0),
   statusTimer (0),
   directHeartPeriod (60),
   isPhone (false)
{
  SetupToolbar ();
  CreateSystemTrayStuff ();
  Connect ();
  debugTimer = new QTimer (this);
  connect (debugTimer, SIGNAL (timeout()), this, SLOT (DebugCheck()));
  debugTimer->start (10 * 1000); // 15 secs
  xmppTimer = new QTimer (this);
  connect (xmppTimer, SIGNAL (timeout()), this, SLOT (XmppPoll ()));
  xmppTimer->start (4* 30 * 1000); // 1/2 mins * 4 == 2 mins
  announceHeartbeat = new QTimer (this);
  connect (announceHeartbeat, SIGNAL (timeout()), this, SLOT (AnnounceMe()));
  announceHeartbeat->start (1000*60*2); // 2 minutes
  statusTimer = new QTimer (this);
  connect (statusTimer, SIGNAL (timeout()), this, SLOT (StatusUpdate()));
  statusTimer->start (30*1000);
  QTimer::singleShot (1500, this, SLOT (StatusUpdate ()));
  xclientMap.clear ();
  trayIcon->show ();
  oldPos = pos();
}

void
DChatMain::SetupToolbar ()
{
#if 0
  directMenu = new QMenu (this);
  directMenu->addAction (tr("Start Direct Chat"),
                         this, SLOT (CallDirect ()));
  directMenu->addAction (tr("Add Direct Listener"), 
                         this, SLOT (ListenerAdd ()));
  directMenu->addAction (tr("Drop Direct Listener"),
                         this, SLOT (ListenerDrop ()));

  xmppMenu = new QMenu (this);
  xmppMenu->addAction (tr("Log In Xmpp"),
                       this, SLOT (Login()));
  xmppMenu->addAction (tr("Log Out Xmpp"),
                       this, SLOT (Logout()));
#endif
}

void
DChatMain::Init (QApplication *pap, bool phone)
{
  pApp = pap;
  isPhone = phone;
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
  QString backPic (":/treeback.png");
  backPic = Settings().value ("style/background-image",backPic).toString ();
  Settings().setValue ("style/background-image",backPic);
  if (backPic.length() > 0) {
    QString stylePattern  ("QTreeView { background-image: url(%1) }");
    pApp->setStyleSheet (stylePattern.arg (backPic));
  }
  directHeartPeriod = Settings().value ("direct/heartperiodsecs",
                                      directHeartPeriod).toInt();
  Settings().setValue ("direct/heartperiodsecs",directHeartPeriod);
  QDeclarativeContext * context = rootContext();
  context->setContextProperty("cppContactModel",&xcontactModel);
  setSource(QUrl("qrc:///qml/Main.qml"));
  setResizeMode(QDeclarativeView::SizeRootObjectToView);
  qmlRoot = qobject_cast<QDeclarativeItem*> (rootObject());
  connect (qmlRoot,SIGNAL(doQuit()), this, SLOT(Quit()));
  connect (qmlRoot,SIGNAL(doLogin()), this, SLOT (Login()));
  connect (qmlRoot, SIGNAL(wantChat(const QString&,const QString &)),
           this,SLOT(StartServerChat(const QString&,const QString&)));
  
  if (isPhone) {
    showFullScreen ();
  } else {
    resize (QSize (800,500));
    showNormal ();
  }
  show ();
  SetupListener ();
  Settings().sync ();
}

void
DChatMain::StatusUpdate ()
{
  QString directMsg = tr("%1 Direct").arg (inDirect.size());
  QString xmppMsg = tr("%1 Xmpp").arg (xclientMap.size());
  if (trayIcon) {
    QString trayMsg = "Egalite\n" + directMsg + "\n" + xmppMsg ;
    trayIcon->setToolTip (trayMsg);
  }
}

void
DChatMain::SetupListener ()
{
  QString ownAddress ("::1");
  ownAddress = Settings().value ("direct/listenAddress",ownAddress).toString();
  Settings().setValue ("direct/listenAddress",ownAddress);
  if (ownAddress.length () < 1) {
    return;
  }
  publicPort = Settings().value ("direct/listenPort",publicPort).toInt ();
  Settings().setValue ("direct/listenPort",publicPort);

  directIdentity = Settings().value ("direct/identity",directIdentity).toString();
  Settings().setValue ("direct/identity",directIdentity);

  StartListener (ownAddress, directIdentity, publicPort);
}

void
DChatMain::StartListener (QString ownAddress, 
                          QString directIdentity, 
                          int     publicPort)
{
  DirectListener * listen (0);
  bool             isListening (false);
  bool             wantStart (true);
  
  if (CertStore::IF().HaveCert (directIdentity)) {
    CertRecord hostCert = CertStore::IF().Cert (directIdentity);
    QString pass = hostCert.Password ();
    if (pass.length() == 0) {
      SimplePass  getPass (this);
      pass = getPass.GetPassword (tr("Listener Password for ")
                                   + directIdentity);
      wantStart = getPass.GotPassword ();
    }
    if (wantStart) {
      if (inDirect.find (ownAddress) == inDirect.end ()) {
        listen = new DirectListener (this);
        inDirect [ownAddress] = listen;
      } else {
        listen = inDirect [ownAddress];
      }
      QSslKey skey (hostCert.Key().toAscii(),QSsl::Rsa,
                  QSsl::Pem, QSsl::PrivateKey, pass.toUtf8());
      QSslCertificate scert (hostCert.Cert().toAscii());
      listen->Init (directIdentity, skey, scert);
      isListening = listen->Listen (ownAddress, publicPort);
    }
  } else {
qDebug () << " cannot listen for identity " << directIdentity;
  }
  if (isListening) {
    connect (listen, SIGNAL (Receive (const QByteArray &)),
           this, SLOT (GetRaw (const QByteArray&)));
    connect (listen, SIGNAL (SocketReady (SymmetricSocket *, QString)),
           this, SLOT (ConnectDirect (SymmetricSocket *, QString)));
  } else {
    if (listen) {
      disconnect (listen, 0,0,0);
      inDirect.erase (ownAddress);
      delete listen;
    }
  }
  StatusUpdate ();
}

void
DChatMain::ListenerAdd ()
{
  PickString  picker (this);
  picker.SetTitle (tr("Pick Identity for Listener"));
  QStringList list = CertStore::IF().NameList ();
  int picked = picker.Pick (list);
  if (picked) {
    QString ident = picker.Choice();
    AddListener  Al (this);
    if (Al.SelectStart (ident)) {
      StartListener (Al.Address(), ident, Al.Port());
    }
  }
}

void
DChatMain::ListenerDrop ()
{
  PickString  picker (this);
  picker.SetTitle (tr("Pick Listener to Drop"));
  QStringList  list;
  std::map <QString, DirectListener*>::const_iterator lit;
  for (lit = inDirect.begin(); lit != inDirect.end(); lit++) {
    list << lit->first;
  }
  QString choice;
  int picked = picker.Pick (list);
  if (picked) {
    choice = picker.Choice ();
    if (inDirect.find (choice) != inDirect.end()) {
      DirectListener * closeThis = inDirect[choice];
      closeThis->Close ();
      //delete inDirect [choice];
      inDirect.erase (choice);
    }
  }
  StatusUpdate ();
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
qDebug () << "DChaiMain Quit ";
  if (pApp) {
    pApp->quit ();
  }
}

void
DChatMain::Connect ()
{
}

void
DChatMain::EditSettings ()
{
  configEdit.Exec ();
  SetSettings ();
  QTimer::singleShot (500, this, SLOT (XmppPoll ()));
}

void
DChatMain::Login ()
{
  QString oldUser = user;
  if (GetPass()) {
    XEgalClient * xclient = xclientMap[user];
    if (!xclient) {
      xclient = new XEgalClient (this, user);
      xclientMap[user] = xclient; 
    }
    QXmppConfiguration  & xconfig = xclient->configuration();
    xconfig.setJid (user);
    xconfig.setHost (server);
    xconfig.setPasswd(password);
    xconfig.setResource (QString ("Egalite.%1").arg(genResourceTag()));
    qDebug () << " after setting resource before connect " << xconfig.resource();
    xclient->connectToServer (xconfig);
    qDebug () << " after setting resource after connect " << xconfig.resource();
    xmppUser = user;
    connect (xclient, SIGNAL (messageReceived  (const QXmppMessage  &)),
             this, SLOT (GetMessage (const QXmppMessage &)));
    connect (xclient, SIGNAL (error (XEgalClient::Error)),
             this, SLOT (XmppError (XEgalClient::Error)));
    connect (xclient, SIGNAL (connected ()), 
             this, SLOT (XmppConnected ()));
    connect (xclient, SIGNAL (disconnected ()),
             this, SLOT (XmppDisconnected ()));
    connect (xclient, SIGNAL (UpdateState (QString, QString, QString, 
                                          QXmppPresence::Status)),
             this, SLOT (XUpdateState (QString, QString, QString, 
                                          QXmppPresence::Status)));
    connect (xclient, SIGNAL (ChangeRequest (QString, QXmppPresence::Status)),
             this, SLOT (XChangeRequest (QString, QXmppPresence::Status)));
    connect (xclient, SIGNAL (iqReceived (const QXmppIq &)),
             this, SLOT (XmppIqReceived (const QXmppIq &)));
    connect (xclient, SIGNAL (discoveryIqReceived (const QXmppDiscoveryIq &)),
             this, SLOT (XmppDiscoveryIqReceived (const QXmppDiscoveryIq &)));
    QTimer::singleShot (2500, this, SLOT (XmppPoll ()));
    StatusUpdate ();
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
  QString version (deliberate::ProgramVersion::Version());
  QStringList messages;
  messages.append (version);
  messages.append (configMessages);

  QMessageBox  box;
  box.setText (version);
  box.setDetailedText (messages.join ("\n"));
  QTimer::singleShot (30000, &box, SLOT (accept()));
  box.exec ();
}


void
DChatMain::AnnounceMe ()
{
  std::map <QString, XEgalClient*> :: iterator xit;
  for (xit = xclientMap.begin(); xit != xclientMap.end(); xit++) {
    if (xit->second) {
      xit->second->Announce (QXmppPresence::Available,
                             QXmppPresence::Status::Online,
                             isPhone ? QString ("Egalite! mobile") : QString ("Egalite!"));
    }
  }
}


bool
DChatMain::GetPass ()
{
  bool picked = PickServerAccount (user, server, password);
  if (picked == 0) {
    return false;
  }
  if (passdial == 0) {
    passdial = new QDialog (this);
    passui.setupUi (passdial);
  }
  passui.userText->setText (user);
  passui.serverText->setText (server);
  passui.passwordText->setText (password);
  connect (passui.okButton, SIGNAL (clicked()), this, SLOT (PassOK()));
  connect (passui.cancelButton, SIGNAL (clicked()), this, SLOT (PassCancel()));
  int haveit = passdial->exec ();
  if (haveit == 1) {
    password = passui.passwordText->text ();
    user = passui.userText->text ();
    server = passui.serverText->text ();
    return true;
  } else {
    return false;
  }
}

bool
DChatMain::PickServerAccount (QString &jid, QString &server, QString &pass)
{
  PickString  picker (this);
  picker.move (this->pos());
  picker.SetTitle (tr("Pick Server Account "));
  QStringList list = CertStore::IF().AccountList ();
  QString newAccount (tr ("--- New Account ---"));
  list << newAccount;
  int picked = picker.Pick (list);
  if (picked) {
    QString ident = picker.Choice();
    if (ident != newAccount) {
      jid = ident;
      return CertStore::IF().RecallAccount (jid, server, pass);
    } else {
      jid.clear() ; 
      server.clear (); 
      pass.clear ();
    }
  }
  return false;
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
  QStringList partsFrom = from.split('/');
  QString remoteId = partsFrom.at(0);
  if (serverChats.find (remoteId) != serverChats.end()) {
    serverChats[remoteId]->Incoming (msg);
  } else {
    QStringList partsTo = to.split ('/');
    QString login = partsTo.at (0);
    StartServerChat (remoteId, login);
    if (serverChats.find (remoteId) != serverChats.end()) {
      serverChats[remoteId]->Incoming (msg);
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
  QXmppMessage outMsg (msg);
  QStringList parts = outMsg.to().split("/");
  QString toUser = parts.at(0);
  outMsg.setTo (toUser);
  parts = outMsg.from().split("/");
  QString fromUser = parts.at(0);
  if (xclientMap[fromUser]) {
    xclientMap[fromUser]->sendMessage (outMsg.to(), outMsg.body());
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
  QString newDest = tr (" --- New Destination --- ");
  choiceList << newDest;
  pickString.SetTitle (tr("Choose Destination"));
  choice = pickString.Pick (choiceList);
  if (choice != 1) {
    return;
  }
  QString dest = pickString.Choice ();
  QString destaddr;
  int     destPort (0);
  bool    wantConnect (false);
  if (dest == newDest) {
    SimplePass  getDest (this);
    destaddr = getDest.GetPlainString (tr("Direct Connect Destination"),
                                   tr("Enter Destination Address or URL"));
    wantConnect = getDest.GotPlainString ();
  } else {
    destaddr = CertStore::IF().ContactAddress (dest);
    destPort = CertStore::IF().ContactPort (dest);
    wantConnect = true;
  }
  if (!wantConnect) {
    return;
  }
  if (destPort == 0) {
    destPort = defaultPort;
  }
  callnum++;
  DirectCaller * newcall = new DirectCaller (this);

  if (newcall) {
    newcall->Setup (outCert, destPort, originNick);
    outDirect[callnum] = newcall;
qDebug () << " start direct connect " << callnum << " call " << newcall;
    newcall->Connect (destaddr, dest, callnum);
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
  xcontactModel.removeMyJid (expired);
  XEgalClient * xclient = xclientMap [expired];
  if (xclient) {
    xclient->Disconnect ();
    disconnect (xclient, 0,0,0);
    delete xclient;
  }
  xclientMap.erase (expired);
  QTimer::singleShot (1500, this, SLOT (StatusUpdate()));
}

void
DChatMain::RequestSubscribe ()
{
  PickString   pickString (this);
  QStringList  choiceList;
  std::map <QString, XEgalClient*>::iterator xit;
  for (xit = xclientMap.begin(); xit != xclientMap.end(); xit++) {
    choiceList << xit->first;
  }
  pickString.SetTitle (tr("Request Subscription from Account"));
  int choice = pickString.Pick (choiceList);
  if (choice != 1) {
    return;
  }
  QString from = pickString.Choice ();
  if (subscribeDial == 0) {
    subscribeDial = new QDialog (this);
    reqSubUi.setupUi (subscribeDial);
    connect (reqSubUi.requestButton, SIGNAL (clicked()),
             this, SLOT (DoRequestSubscribe()));
    connect (reqSubUi.cancelButton, SIGNAL (clicked()),
             subscribeDial, SLOT(reject()));
  }
  reqSubUi.fromAccount->setText (from);
  reqSubUi.toAccount->clear ();
  subscribeDial->show ();
}

void
DChatMain::DoRequestSubscribe ()
{
  if (subscribeDial) {
    QString toAccount = reqSubUi.toAccount->text();
    QString fromAccount = reqSubUi.fromAccount->text();
qDebug () << " subscribe request from " << fromAccount << " to " << toAccount;
    int done(0);
    if (xclientMap.find (fromAccount) != xclientMap.end()) {
      QXmppClient * xclient = xclientMap[fromAccount];
      QXmppPresence  request;
      request.setTo (toAccount);
      request.setType (QXmppPresence::Subscribe);
qDebug () << " sending subscribe request with xclient" << xclient;
      xclient->sendPacket (request);
      done = 1;
    }
    subscribeDial->done (done);
  }
}

void
DChatMain::ConnectDirect (SymmetricSocket * sock, QString localNick)
{
qDebug () << " have connection with " << sock;
Q_UNUSED (localNick);
  if (sock) {
    QString other = sock->PeerName();
    ChatBox * newBox = new ChatBox (this);
    newBox->SetTitle (tr("direct ") + sock->RemoteName());
    newBox->Add (sock->Dialog(),tr("Status")); 
    ChatContent * newCont = new ChatContent (this);
    newBox->Add (newCont,tr("Direct"));
    directChats [other] = newBox;
    connect (newCont, SIGNAL (Activity (QWidget*)),
             newBox, SLOT (WidgetActivity (QWidget*)));
    connect (newCont, SIGNAL (Outgoing (const QByteArray&)),
             sock, SLOT (SendData (const QByteArray&)));
    connect (newCont, SIGNAL (Disconnect (QString)),
             sock, SLOT (Close()));
    connect (newCont, SIGNAL (ChangeProto (QWidget*, QString)),
             newBox, SLOT (ContentProto (QWidget*, QString)));
    connect (sock, SIGNAL (Exiting(SymmetricSocket *)), 
             this, SLOT (ClearDirect (SymmetricSocket *)));
    connect (sock, SIGNAL (Exiting(SymmetricSocket *)),
              newCont, SLOT (Stop ()));
    connect (sock, SIGNAL (ReadyRead ()), 
              newCont, SLOT (InputAvailable ()));
    newBox->Run ();
    newCont->SetInput (sock->Socket());
    sock->SetDoOwnRead (false);
    newCont->SetProtoVersion ("0.1");
    newCont->SetHeartbeat (directHeartPeriod);
    newCont->Start (ChatContent::ChatModeRaw,
                    sock->RemoteName(),
                    sock->LocalName());
    Show ();
  }
}

void
DChatMain::StartServerChat (const QString & remoteName, const QString & serverLogin)
{
qDebug () << " starting new server chat for remote " << remoteName;
qDebug () << "                              local  " << serverLogin;
  ChatBox * newBox = new ChatBox (this);
  newBox->SetTitle (tr("Xmpp ") + remoteName);
  ChatContent * newCont = new ChatContent (this);
  newCont->SetMode (ChatContent::ChatModeXmpp);
  newCont->SetRemoteName (remoteName);
  newCont->SetLocalName (serverLogin);
  newBox->Add (newCont, tr("Xmpp Chat"));
  serverChats [remoteName] = newBox;
  connect (newCont, SIGNAL (Activity (const QWidget*)),
           newBox, SLOT (WidgetActivity (const QWidget*)));
  connect (newCont, SIGNAL (Outgoing (const QXmppMessage&)),
           this, SLOT (Send (const QXmppMessage&)));
  connect (newBox, SIGNAL (HandoffIncoming (const QXmppMessage&)),
            newCont, SLOT (IncomingXmpp (const QXmppMessage&)));
  connect (newCont, SIGNAL (Disconnect(QString)),
            this, SLOT (CloseServerChat (QString)));
  connect (newCont, SIGNAL (ChangeProto (QWidget*, QString)),
            newBox, SLOT (ContentProto (QWidget*, QString)));
  newBox->Run ();
  newCont->SetProtoVersion ("0.1");
  newCont->SetHeartbeat (0);
  newCont->Start ();
  Show ();
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
}


void
DChatMain::XChangeRequest (QString login, QXmppPresence presence)
{
  subscriptionDialog.RemoteAskChange (xclientMap[login], presence);
}

void
DChatMain::XUpdateState (QString remoteName, 
                         QString ownId,
                         QString remoteId,
                         QXmppPresence::Status status)
{
  xcontactModel.updateState (remoteId, ownId, remoteName, 
                            status.getStatusText(), status.type());
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
  QByteArray msgbytes;
  QXmlStreamWriter  dump (&msgbytes);
  iq.toXml (&dump);
  qDebug () << " uncooked IQ: " << msgbytes;
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
qDebug () << " polling " << xclient << " at " 
          << QDateTime::currentDateTime().toString();
  QStringList  contactJids;
  if (xclient == 0) {
    return;
  }
  QXmppRosterManager & rosterMgr = xclient->rosterManager();
  contactJids = rosterMgr.getRosterBareJids();
  xmppConfig = xclient->configuration ();
  xmppConfig.setResource ("Egalite.");
  //QStringList::const_iterator stit;
  QString thisUser = xmppConfig.jidBare ();

  for (auto stit = contactJids.begin (); stit != contactJids.end (); stit++) {
    QString id = *stit;
    QStringList resources = rosterMgr.getResources (id);
    QString res;
    QXmppRosterIq::Item entry = rosterMgr.getRosterEntry (id);
    QString remoteName = entry.name();
qDebug () << " id " << id << " is " << remoteName;
    QStringList::const_iterator   rit;
    if (resources.size () == 0) {
qDebug () << "    model-update setting offline " << id << " for user " << thisUser;
      xcontactModel.updateStateAll(id,QXmppPresence::Status::Offline);
    }
    for (rit = resources.begin (); rit != resources.end (); rit++) {
      res = *rit;
      QString bigId = id + QString("/") + res;
      QXmppPresence pres = rosterMgr.getPresence (id,res);
      QXmppPresence::Status status = pres.status();
      qDebug () << "    model-update status of " << bigId << status.type();
      xcontactModel.updateState (thisUser,bigId,remoteName,status.getStatusText(),status.type());
    } 
  }
}

void
DChatMain::ExpandAccountView (QModelIndex accountIndex)
{
}


void
DChatMain::EditServerLogin ()
{
  serverAccountEdit.Exec ();
}

void
DChatMain::Show ()
{
  if (haveOldPos) {
    move (oldPos);
    haveOldPos = false;
  }
  show ();
  showNormal ();
}

void
DChatMain::hide ()
{
  oldPos = pos ();
  haveOldPos = true;
  QDeclarativeView::hide ();
}

void 
DChatMain::CreateSystemTrayStuff ()
{
  trayMenu = new QMenu(this);
  trayMenu->addAction (tr("Show %1").arg (QString::fromUtf8("Egalite")),
                       this, SLOT (Show()));
  trayMenu->addAction (tr ("Hide %1").arg (QString::fromUtf8("Egalite")),
                       this, SLOT (hide()));
  trayMenu->addAction (tr ("Quit %1").arg (QString::fromUtf8("Egalite")),
                       this, SLOT (Quit()));
  trayIcon = new QSystemTrayIcon(QIcon (":/dchatlogo.png"),this);
  trayIcon->setContextMenu(trayMenu);
  trayIcon->setToolTip (QString::fromUtf8("Egalite"));
  connect (trayIcon, SIGNAL (activated (QSystemTrayIcon::ActivationReason)),
             this, SLOT(TrayActivated(QSystemTrayIcon::ActivationReason)));
  connect (trayIcon, SIGNAL (messageClicked ()),
             this, SLOT (TrayMessageClicked ()));
}

void
DChatMain::TrayMenu ()
{
  QPoint here = QCursor::pos();
  trayMenu->exec (here);
}

void 
DChatMain::TrayActivated (QSystemTrayIcon::ActivationReason reason)
{
qDebug () << " Tray Icon activated reason " << reason;
  switch (reason) {
  case QSystemTrayIcon::Trigger:
    Show ();
    break;
  default:
    TrayMenu ();
  }
}

void
DChatMain::ShowTrayMessage (const QString & msg)
{
  if (trayIcon) { 
    QString title (QString::fromUtf8("Egalite"));
    trayIcon->showMessage (title,msg,QSystemTrayIcon::NoIcon);                           
  }
}

void
DChatMain::TrayMessageClicked ()
{
  Show ();
}

void 
DChatMain::closeEvent(QCloseEvent *event)
{
 QDeclarativeView::closeEvent (event);
}

bool
DChatMain::event (QEvent *evt)
{
  //qDebug () << " DChaiMain event " << evt;
  return QDeclarativeView::event (evt);
}

QString
DChatMain::genResourceTag()
{
  QString uniqueS (QUuid::createUuid().toString().replace(QRegExp("[{}-]"),""));
  quint64 sixBytes (0);
  for (int d=0; d<uniqueS.length(); d++) {
    bool ok;
    QString uC (uniqueS.mid(d,1));
    quint8 uByte = static_cast<quint8> (uC.toUInt(&ok,16) & 0xff);
    qDebug () << " uniq: uC " << uC << hex << uByte << dec;
    quint64 hi = (sixBytes & 0xff0000000000LLU) >> 32;
    sixBytes <<= 6;
    sixBytes = sixBytes & 0x7fffffffffffLLU;
    sixBytes = sixBytes 
               ^ static_cast<quint16> (hi << 8) 
               ^ uByte; // put old high byte back in
  }
  return QString::number(sixBytes,16);
}


} // namespace

