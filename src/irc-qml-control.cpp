#include "irc-qml-control.h"

/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2011, Bernd Stramm
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

#include "deliberate.h"
#include "version.h"
#include "helpview.h"
#include "edit-simple.h"
#include "irc-abstract-channel.h"
#include "irc-socket.h"
#include "irc-qml-sock-static.h"
#include "irc-ctcp.h"
#include "link-mangle.h"
#include <QSize>
#include "cert-store.h"
#include "qml-irc-channel-group.h"
#include "enter-string.h"
#include <QDebug>
#include <QTimer>
#include <QHostAddress>
#include <QTimer>
#include <QFont>
#include <QFile>
#include <QMessageBox>
#include <QDeclarativeContext>

using namespace deliberate;

namespace egalite
{

IrcQmlControl::IrcQmlControl (QWidget *parent)
  :QDialog (parent),
   initDone (false),
   qmlRoot (0),
   isRunning (false),
   isConnected (false),
   hidSelf (false),
   knownServers (this)
{
  ui.setupUi (this);
  dockedChannels = new QmlIrcChannelGroup (0 /*parentWidget ()*/);
  dockedChannels->Start ();
  dockedChannels->hide ();
  commandXform ["MSG"] = IrcQmlSockStatic::TransformPRIVMSG;
  commandXform ["ME"] = IrcQmlSockStatic::TransformME;
  commandXform ["JOIN"] = IrcQmlSockStatic::TransformJOIN;
  receiveHandler ["PING"] = IrcQmlSockStatic::ReceivePING;
  receiveHandler ["PONG"] = IrcQmlSockStatic::ReceiveIgnore;
  receiveHandler ["PRIVMSG"] = IrcQmlSockStatic::ReceivePRIVMSG;
  receiveHandler ["QUIT"] = IrcQmlSockStatic::ReceiveQUIT;
  receiveHandler ["JOIN"] = IrcQmlSockStatic::ReceiveJOIN;
  receiveHandler ["PART"] = IrcQmlSockStatic::ReceivePART;
  receiveHandler ["004"] = IrcQmlSockStatic::Receive004;
  receiveHandler ["332"] = IrcQmlSockStatic::Receive332;
  receiveHandler ["353"] = IrcQmlSockStatic::Receive353;
  receiveHandler ["366"] = IrcQmlSockStatic::Receive366;
  receiveHandler ["TOPIC"] = IrcQmlSockStatic::ReceiveTOPIC;
  receiveHandler ["VERSION"] = IrcQmlSockStatic::ReceiveIgnore;
  ctcpHandler ["ACTION"] = IrcCtcp::ReceiveACTION;
  ctcpHandler ["VERSION"] = IrcCtcp::ReceiveVERSION;
qDebug () << " IrcQmlControl allocated and initialized";
}

void
IrcQmlControl::Show ()
{
  if (isRunning) {
    LoadLists ();
  } else {
    Run ();
  }
  if (hidSelf) {
    resize (oldSize);
    move (oldPos);
    hidSelf = false;
  }
  show ();
}

void
IrcQmlControl::Hide ()
{
qDebug () << " IrcQmlControl::Hide ()";
  oldSize = size ();
  oldPos = pos();
  hidSelf = true;
  hide ();
}

void
IrcQmlControl::ShowGroup ()
{
  if (!isRunning) {
    Run ();
  }
  if (dockedChannels) {
    dockedChannels->Show ();
  }
}

void
IrcQmlControl::HideGroup ()
{
  if (dockedChannels) {
    dockedChannels->Hide ();
  }
}

void
IrcQmlControl::ShowFloats ()
{
  FloatingMapType::iterator fit;
  for (fit=floatingChannels.begin (); fit != floatingChannels.end(); fit++) {
    fit.value()->Show ();
  }
}

void
IrcQmlControl::HideFloats ()
{
  FloatingMapType::iterator fit;
  for (fit=floatingChannels.begin (); fit != floatingChannels.end(); fit++) {
    fit.value()->Hide ();
  }
}

bool
IrcQmlControl::Run ()
{
  if (!isRunning) {
    qDebug () << " Start IrcQmlControl";
    QSize defaultSize = size();
    QSize newsize = Settings().value ("sizes/ircsock", defaultSize).toSize();
    resize (newsize);
    QSize  groupBoxSize = dockedChannels->size();
    groupBoxSize = Settings().value ("sizes/channelgroup", groupBoxSize)
                             .toSize();
    dockedChannels->resize (groupBoxSize);
  }

  QDeclarativeContext * context = ui.qmlView->rootContext ();
  if (context == 0) {
    QMessageBox::critical (this, "Fatal", "QML Context Missing");
    return false;
  }
  context->setContextProperty ("cppKnownServerModel", &knownServers);
  ui.qmlView->setSource (
         QUrl("qrc:///qml/IrcControl.qml"));

  qmlRoot = ui.qmlView->rootObject();
  if (qmlRoot == 0) {
    QMessageBox::critical (this, "Fatal", "QML Load Failure");
    return false;
  }
  ConnectGui ();
  LoadLists ();
  show ();

  isRunning = true;
  return true;
}

void
IrcQmlControl::LoadLists ()
{
  QStringList  servers = CertStore::IF().IrcServers ();
  noNameServer = tr("--- New Server ---");
  servers.append (noNameServer);
  int ns = servers.count();
  for (int i=0; i< ns; i++) {
    knownServers.addServer (servers.at(i), 6667);
  }

  QStringList  nicks = CertStore::IF().IrcNicks ();
  noNameNick = tr ("--- New Nick ---");
  nicks.append (noNameNick);

  QStringList  chans = CertStore::IF().IrcChannels ();
  noNameChannel = tr("--- New Channel ---");
  chans.append (noNameChannel);
  #if 0
  mainUi.chanCombo->clear ();
  mainUi.chanCombo->insertItems (0,chans);
  #endif

  ignoreSources = CertStore::IF().IrcIgnores ();
}

void
IrcQmlControl::ConnectGui ()
{
  connect (qmlRoot, SIGNAL (hideMe()), this, SLOT (Hide()));
  #if 0
  connect (mainUi.connectButton, SIGNAL (clicked()),
           this, SLOT (TryConnect ()));
  connect (mainUi.joinButton, SIGNAL (clicked()),
           this, SLOT (TryJoin ()));
  connect (mainUi.sendButton, SIGNAL (clicked()),
           this, SLOT (Send ()));
  connect (mainUi.partButton, SIGNAL (clicked()),
           this, SLOT (TryPart ()));
  connect (mainUi.sendEdit, SIGNAL (returnPressed()),
           this, SLOT (Send ()));
  connect (mainUi.loginButton, SIGNAL (clicked()),
           this, SLOT (NickLogin ()));
  connect (mainUi.hideButton, SIGNAL (clicked ()),
           this, SLOT (hide ()));
  connect (mainUi.chanList, SIGNAL (itemClicked (QListWidgetItem *)),
           this, SLOT (ChannelClicked (QListWidgetItem *)));
  connect (mainUi.serverTable, SIGNAL (itemClicked (QTableWidgetItem *)),
           this, SLOT (ServerClicked (QTableWidgetItem *)));
  #endif
}

void
IrcQmlControl::Exit ()
{
  dockedChannels->Close ();
  TryDisconnect ();
  CloseCleanup ();
  hide ();
}


void
IrcQmlControl::CloseCleanup ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/ircsock",currentSize);
  QSize  groupBoxSize = dockedChannels->size();
  Settings().setValue ("sizes/channelgroup", groupBoxSize);
  Settings().sync();
}

void
IrcQmlControl::HideAll ()
{
  HideGroup ();
  HideFloats ();
}

void
IrcQmlControl::ShowAll ()
{
  ShowGroup ();
  ShowFloats ();
}

void
IrcQmlControl::Exiting ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/ircsock",currentSize);
  Settings().sync();
}

void
IrcQmlControl::TryConnect ()
{
  QString host; // = mainUi.serverCombo->currentText();
  if (host == noNameServer) {
    EnterString  enter (this);
    bool picked = enter.Choose (tr("IRC Server"), tr("Server:"));
    if (picked) {
      host = enter.Value ();
      if (enter.Save()) {
        CertStore::IF().SaveIrcServer (host);
      }
    } else {
      return;
    }
  }
qDebug () << " try connect to " << host;
  IrcSocket *socket = new IrcSocket (this);
  sockets [socket->Name()] = socket;
  connect (socket, SIGNAL (connected (IrcSocket*)),
           this, SLOT (ConnectionReady (IrcSocket*)));
  connect (socket, SIGNAL (disconnected (IrcSocket*)),
           this, SLOT (ConnectionGone (IrcSocket*)));
  connect (socket, SIGNAL (ReceivedLine (IrcSocket*, QByteArray)),
           this, SLOT (ReceiveLine (IrcSocket *, QByteArray)));
  connect (socket, SIGNAL (ChangedHostName (IrcSocket*, QString)),
           this, SLOT (ChangedHostName (IrcSocket*, QString)));
  quint16 port = 6667; // mainUi.portBox->value ();
  socket->connectToHost (host, port);
}

void
IrcQmlControl::TryJoin ()
{
  QString chan ; //= mainUi.chanCombo->currentText ();
  if (chan == noNameChannel) {
    EnterString enter (this);
    bool picked = enter.Choose (tr("IRC Channel"), tr("Channel:"));
    if (picked) {
      chan = enter.Value ();
      if (enter.Save()) {
        CertStore::IF().SaveIrcChannel (chan);
       // mainUi.chanCombo->clear ();
        QStringList knownChans = CertStore::IF().IrcChannels ();
        knownChans.append (noNameChannel);
qDebug () << " Kknown channel " << noNameChannel;
       // mainUi.chanCombo->addItems (knownChans);
      }
    } else {
      return;
    }
  }
  IrcSocket * sock = 0; //CurrentSock (mainUi.serverTable);
  if (sock == 0) {
    return;
  }
  sock->Send (QString ("JOIN %1").arg(chan));
}

void
IrcQmlControl::TryPart ()
{
  QString chan = 0; //mainUi.chanCombo->currentText ();
  IrcSocket * sock = 0; //CurrentSock (mainUi.serverTable);
  if (sock) {
    sock->Send (QString ("PART %1").arg (chan));
  }
}

int
IrcQmlControl::OpenCount ()
{
  return sockets.size();
}

void
IrcQmlControl::TryDisconnect ()
{
  ChannelMapType::iterator  cit;
  for (cit=channels.begin(); cit != channels.end(); cit++) {
    (*cit)->Part();
  }
  SocketMapType::iterator  sit;
  for (sit = sockets.begin (); sit != sockets.end(); sit++) {
    if (*sit) {
     (*sit)->Disconnect ();
    }
  }
}

void
IrcQmlControl::ReceiveLine (IrcSocket * sock, QByteArray line)
{
  QString data (QString::fromUtf8(line.data()));
qDebug () << " received Line " << data;
qDebug () << "       from " << sock->Name();
  if (data.startsWith(QChar (':'))) {
    QRegExp wordRx ("(\\S+)");
    QString first, cmd, rest;
    int pos, len;
    int totallen (data.length());
    int restlen (totallen);
    pos = wordRx.indexIn (data,1);
    if (pos >= 0) {
      len = wordRx.matchedLength ();
      restlen = totallen - (pos + len);
      first = data.mid (pos,len);
      pos = wordRx.indexIn (data,pos+len+1);
      if (pos >= 0) {
        len = wordRx.matchedLength ();
        restlen = totallen - (pos+len);
        cmd = data.mid (pos,len).toUpper();
      }
    }
    rest = data.right (restlen);
    cmd = cmd.trimmed ();
    QRegExp numericRx ("\\d\\d\\d");
    if (receiveHandler.contains (cmd.toUpper())) {
      (*receiveHandler [cmd]) (this, sock, first, cmd, rest);
    } else if (numericRx.exactMatch (cmd)) {
      IrcQmlSockStatic::ReceiveNumeric (this, sock, first, cmd, rest);
    } else {
      IrcQmlSockStatic::ReceiveDefault (this, sock, first, cmd, rest);
    }
  } else {
    if (data.startsWith ("PING")) {
      QString retName = data.split (QRegExp("(\\s+)")).at(1);
      sock->Send (QString ("PONG :%1").arg(retName));
    }
  }
  qDebug () << " Received line " << line;
  //mainUi.logDisplay->append (QString (line));
}

void
IrcQmlControl::Send ()
{
  int row = 0; //mainUi.serverTable->currentRow ();
  QTableWidgetItem * item = 0; //FindType (mainUi.serverTable, row, int (Cell_Addr));
  if (item) {
    QString sname = item->data (int(Data_ConnName)).toString();
    if (sockets.contains (sname)) {
      QString text ; //= mainUi.sendEdit->text ();
      IrcSocket * sock = sockets [sname];
      TransformSend (sock, "", text);
      sock->Send (text);
    }
  }
}

void
IrcQmlControl::TransformSend (IrcSocket * sock, const QString & chan, 
                           QString & data)
{ 
  if (data.startsWith(QChar ('/'))) {
    QRegExp wordRx ("(\\S+)");
    int pos = wordRx.indexIn (data, 1);
    if (pos >= 0) {
      int len = wordRx.matchedLength ();
      QString first = data.mid (1,len).toUpper();
      QString rest = data.mid (len+1,-1);
      QString copyChan (chan);
      if (commandXform.contains (first)) {
        (*commandXform[first]) (this, sock, data, copyChan, first, rest);
      } else {
        IrcQmlSockStatic::TransformDefault (this, sock, data, copyChan, first, rest);
      }
    }
  }
}


void
IrcQmlControl::ConnectionReady (IrcSocket * sock)
{
 // mainUi.logDisplay->append ("Connection Ready");
  //mainUi.logDisplay->append (QString ("peer address %1")
  //               .arg (sock->peerAddress().toString()));
  qDebug () << " connection ready " << sock->peerAddress().toString();
  AddConnect (sock);
  emit StatusChange ();
}

void
IrcQmlControl::AddConnect (IrcSocket *sock)
{
  int nrows = 0; //mainUi.serverTable->rowCount();
  QTableWidgetItem * item = 
          new QTableWidgetItem (sock->HostName(), int(Cell_Name));
  int row (nrows);
  //mainUi.serverTable->setRowCount (nrows+1);
  //mainUi.serverTable->setItem (row, 1, item);
  item = new QTableWidgetItem (sock->peerAddress().toString(),
                               int (Cell_Addr));
  item->setData (Data_ConnName, sock->Name());
  //mainUi.serverTable->setItem (row, 2, item);
  item = new QTableWidgetItem (QString::number (sock->peerPort()),
                               int (Cell_Port));
  //mainUi.serverTable->setItem (row, 3, item);
  item = new QTableWidgetItem (tr("Disconnect"), int (Cell_Action));
  //mainUi.serverTable->setItem (row, 0, item);
  //mainUi.serverTable->selectRow (row);
}

void
IrcQmlControl::ChangedHostName (IrcSocket * sock, QString name)
{
qDebug () << " have new host name " << name << " for " << sock;
  if (sock == 0) {
    return;
  }
  QString sname = sock->Name();
  int row = 0; //FindRow (mainUi.serverTable, sname);
  QTableWidgetItem * item = 0; //FindType (mainUi.serverTable, row, int(Cell_Name));
  if (item) {
    item->setText (name);
  }
  ChannelMapType::iterator cit;
  for (cit=channels.begin(); cit!=channels.end(); cit++) {
    if (*cit) {
      if ((*cit)->Sock() == sname) {
        (*cit)->SetHost (name);
      }
    }
  }
}

QTableWidgetItem *
IrcQmlControl::FindType (QTableWidget * table, int row, int type)
{
  if (table == 0) {
    return 0;
  }
  int ncols = table->columnCount();
  for (int col=0; col < ncols; col++) {
    QTableWidgetItem * item = table->item (row, col);
    if (item && item->type() == type) {
      return item;
    }
  }
  return 0;
}

int
IrcQmlControl::FindRow (QTableWidget * table, const QString & sname)
{
  int nrows = table->rowCount ();
  int ncols = table->columnCount ();
  for (int row = 0; row< nrows; row++) {
    for (int col=0; col < ncols; col++) {
      QTableWidgetItem * item = table->item (row, col);
      if (item && item->type() == int (Cell_Addr)
          && item->data (int (Data_ConnName)).toString() == sname) {
        return row;
      }
    }
  }
  return -1;
}

void
IrcQmlControl::ConnectionGone (IrcSocket * sock)
{
  qDebug () << " disconnect seen for " << sock;
 // mainUi.logDisplay->append (QString ("!!! disconnected from %1")
  //               .arg (sock->peerAddress().toString()));
  RemoveConnect (sock);
  sockets.remove (sock->Name());
  sock->deleteLater ();
  emit StatusChange ();
}

void
IrcQmlControl::RemoveConnect (IrcSocket * sock)
{
  int row, col;
  int nrows = 0;//mainUi.serverTable->rowCount ();
  int ncols = 0;//mainUi.serverTable->columnCount ();
  QString sname = sock->Name();
  QTableWidgetItem *item;  
  bool looking (true);
  for (row=0; looking && row< nrows; row++) {
    for (col=0; looking && col< ncols; col++) {
      item = 0;// FindType (mainUi.serverTable, row, int (Cell_Addr));
      if (item && item->data (int(Data_ConnName)).toString() == sname) {
        //mainUi.serverTable->removeRow (row);
        looking = false;
      }
    }
  }
}


void
IrcQmlControl::NickLogin ()
{
  IrcSocket * sock = 0; //= CurrentSock (mainUi.serverTable);
  if (sock == 0) {
    return;
  }
  QString nick;//= mainUi.nickCombo->currentText();
  if (nick == noNameNick) {
    EnterString  enter (this);
    bool picked = enter.Choose (tr("IRC Nick"), tr("Nick:"), true);
    if (picked) {
      nick = enter.Value ();
    } else {
      return; 
    }
  }
  QString pass;
  QString real;
  bool  havePass = CertStore::IF().GetIrcIdent (nick, real, pass);
  if (real.length() == 0) {
    real = nick;
  }
  if (havePass) {
    sock->Send (QString ("PASS :%1").arg(pass));
  }
  sock->Send (QString ("USER %1 0 * :%2").arg (nick).arg(real));
  sock->Send (QString ("NICK %1").arg (nick));
  sock->SetNick (nick);
  QString part;
  QString quit;
  bool haveMsgs = CertStore::IF().GetIrcMessages (nick, part, quit);
  if (haveMsgs) { 
    sock->SetPartMsg (part);
    sock->SetQuitMsg (quit);
  }
}

IrcSocket *
IrcQmlControl::CurrentSock (QTableWidget * table)
{
  int row = table->currentRow ();
  if (row < 0) {
    return 0;
  }
  for (int col=0; col < table->columnCount(); col++) {
    QTableWidgetItem * item = FindType (table, row, int(Cell_Addr));
    if (item) {    
      QString sname = item->data (int(Data_ConnName)).toString ();
      if (sockets.contains (sname)) {
        return sockets [sname];
      }
    }
  }
  return 0;
}

void
IrcQmlControl::AddChannel (IrcSocket * sock, const QString & chanName)
{
  if (channels.contains (chanName)) {
    return;
  }
  if (sock == 0) {
    return;
  }
  IrcAbstractChannel * newchan  = new IrcAbstractChannel (chanName, sock->Name(), this);
  channels [chanName] = newchan;
  dockedChannels->AddChannel (newchan);
  dockedChannels->show ();
  newchan->SetHost (sock->HostName());
  newchan->SetPartMsg (sock->PartMsg ());
  newchan->StartWatching (QRegExp (QString ("\\b%1\\b").arg(sock->Nick())));
  connect (newchan, SIGNAL (Outgoing (QString, QString)),
           this, SLOT (Outgoing (QString, QString)));
  connect (newchan, SIGNAL (OutRaw (QString, QString)),
           this, SLOT (SendRaw (QString, QString )));
  connect (newchan, SIGNAL (Active (IrcAbstractChannel *)),
           this, SLOT (ChanActive (IrcAbstractChannel *)));
  connect (newchan, SIGNAL (InUse (IrcAbstractChannel *)),
           this, SLOT (ChanInUse (IrcAbstractChannel *)));
  connect (newchan, SIGNAL (WantFloat (IrcAbstractChannel *)),
           this, SLOT (ChanWantsFloat (IrcAbstractChannel *)));
  connect (newchan, SIGNAL (WantDock (IrcAbstractChannel *)),
           this, SLOT (ChanWantsDock (IrcAbstractChannel *)));
  connect (newchan, SIGNAL (HideAllChannels ()),
           this, SLOT (HideAll ()));
  connect (newchan, SIGNAL (HideDock ()),
           this, SLOT (HideGroup ()));
  connect (newchan, SIGNAL (HideChannel (IrcAbstractChannel *)),
           this, SLOT (HideChannel (IrcAbstractChannel *)));
  connect (newchan, SIGNAL (WantWhois (QString, QString, bool)),
           this, SLOT (WantsWhois (QString, QString, bool)));
  connect (newchan, SIGNAL (WatchAlert (QString, QString)),
           this, SLOT (SeenWatchAlert (QString, QString)));
  //mainUi.chanList->addItem (chanName);
}

void
IrcQmlControl::DropChannel (IrcSocket * sock, const QString & chanName)
{
  Q_UNUSED (sock)
  if (!channels.contains (chanName)) {
    return;
  }
  IrcAbstractChannel * chanBox = channels [chanName];
  disconnect (chanBox, 0,0,0);
  qDebug () << " dropping channel " << chanName << chanBox->Name();
  if (dockedChannels->HaveChannel (chanBox)) {
    dockedChannels->RemoveChannel (chanBox);
  }
  if (floatingChannels.contains (chanBox)) {
    IrcFloat * oldFloat = floatingChannels [chanBox];
    oldFloat->RemoveChannel (chanBox);
    floatingChannels.remove (chanBox);
    oldFloat->deleteLater ();
  }
  channels.remove (chanName);
  chanBox->deleteLater ();
  int nc =0; // mainUi.chanList->count();
  for (int c=nc-1; c>=0; c--) {
    QListWidgetItem *item = 0; //mainUi.chanList->item (c);
    if (item && item->text() == chanName) {
     // mainUi.chanList->takeItem (c);
      delete item;
    }
  }
}

void
IrcQmlControl::WantsWhois (QString channel, QString otherUser, bool wantsit)
{
  if (wantsit) {
    whoisWait [otherUser] = channel;
  } else {
    whoisWait.remove (otherUser);
  }
}

void
IrcQmlControl::WhoisData (const QString & thisUser,
                  const QString & otherUser,
                  const QString & numeric,
                  const QString & data)
{
  if (whoisWait.contains (otherUser)) {
    QString chan = whoisWait[otherUser];
    if (channels.contains (chan)) {
      IrcAbstractChannel * box = channels[chan];
      box->WhoisData (otherUser, numeric, data);
    }
  }
}

void
IrcQmlControl::ChannelClicked (QListWidgetItem *item)
{
  if (item) {
    QString chanName = item->text ();
    qDebug () << " clicked on channel " << chanName;
    IrcAbstractChannel * chanBox = channels [chanName];
    qDebug () << "       is in box " << chanBox;
    ShowChannel (chanBox);
  }
}

void
IrcQmlControl::ShowChannel (IrcAbstractChannel * chanBox)
{
  if (dockedChannels->HaveChannel (chanBox)) {
    dockedChannels->ShowChannel (chanBox);
  } else if (floatingChannels.contains (chanBox)) {
    floatingChannels [chanBox]->Show ();
  } 
}

void
IrcQmlControl::HideChannel (IrcAbstractChannel * chanBox)
{
  if (floatingChannels.contains (chanBox)) {
    floatingChannels [chanBox]->Hide();
  }
}

void
IrcQmlControl::ServerClicked (QTableWidgetItem *item)
{
  if (item) {
    qDebug () << " clicked socked table item " << item 
              << " row " << item->row()
              << " " << item->text ();
    if (item->text () == tr("Disconnect")) {
      QTableWidgetItem * aitem =0; // FindType (mainUi.serverTable, 
                                  //         item->row(), 
                                  //         int (Cell_Addr));
      if (aitem) {
        QString sname = aitem->data (int (Data_ConnName)).toString();
        if (sockets.contains (sname)) {
          PartAll (sname);
          sockets [sname] -> DisconnectLater (3000);
        }
      }
    }
  }
}

void
IrcQmlControl::PartAll (const QString & sockName)
{
  ChannelMapType::iterator cit;
  for (cit=channels.begin(); cit != channels.end(); cit++) {
    if (*cit) {
      IrcAbstractChannel * chan = *cit;
      if (chan->Sock() == sockName) {
        chan->Part ();
      }
    }
  }
}

void
IrcQmlControl::UserQuit (IrcSocket * sock,
                      const QString & user,
                      const QString & msg)
{
  QString sockName = sock->Name();
  QString quitMsg = QString (tr("QUIT ") + msg);
  ChannelMapType::iterator cit;
  for (cit=channels.begin(); cit != channels.end(); cit++) {
    if (*cit) {
      IrcAbstractChannel * chan = *cit;
      if (chan->Sock() == sockName) {
        chan->DropName (user, quitMsg);
      }
    }
  }
}

void
IrcQmlControl::ChanActive (IrcAbstractChannel *chan)
{
  dockedChannels->MarkActive (chan, true);
}

void
IrcQmlControl::ChanInUse (IrcAbstractChannel *chan)
{
  dockedChannels->MarkActive (chan, false);
}

void
IrcQmlControl::ChanWantsFloat (IrcAbstractChannel *chan)
{
  if (dockedChannels->HaveChannel (chan)) {
    dockedChannels->RemoveChannel (chan);
  }
  if (!floatingChannels.contains (chan)) {
    IrcFloat * newFloat = new IrcFloat (this);
    floatingChannels [chan] = newFloat;
    newFloat->AddChannel (chan);
    newFloat->show ();
  }
}

void
IrcQmlControl::ChanWantsDock (IrcAbstractChannel *chan)
{
  if (floatingChannels.contains (chan)) {
    IrcFloat * oldFloat = floatingChannels [chan];
    oldFloat->RemoveChannel (chan);
    floatingChannels.remove (chan);
    oldFloat->deleteLater ();
  }
  if (!dockedChannels->HaveChannel (chan)) {
    dockedChannels->AddChannel (chan);
    dockedChannels->show ();
  }
}

void
IrcQmlControl::SeenWatchAlert (QString chanName, QString data)
{
  emit WatchAlert (tr ("IRC (%2): %1").arg(data).arg(chanName));
}

void
IrcQmlControl::Outgoing (QString chan, QString msg)
{
  QString trim = msg.trimmed ();
  if (trim.length () < 1) {
    return;
  }
  if (!channels.contains (chan)) {
    return;
  }
  QString sname = channels[chan]->Sock ();
  if (!sockets.contains (sname)) {
    return;
  }
  IrcSocket * sock = sockets [sname];
  if (trim.startsWith (QChar ('/')) ){
    QString cooked (trim);
    TransformSend (sock, chan, cooked);
    sock->Send (cooked);
qDebug () << " ==========>618 handed to socket: " << cooked;
    trim.prepend (QString (":%1!%1@localhost ").arg (sock->Nick()));
    ReceiveLine (sock, trim.toUtf8());
  } else {
    QString data (QString ("PRIVMSG %1 :%2").arg (chan). arg (msg));
    sock->SendData (data);
    data.prepend (QString (":%1!%1@localhost ").arg (sock->Nick()));
    ReceiveLine (sock, data.toUtf8());
  }
}

void
IrcQmlControl::InChanMsg (IrcSocket * sock,
                    const QString & chan, 
                    const QString & from, 
                    const QString & msg)
{
  if (ignoreSources.contains (from)) {
    return;
  }
  if (channels.contains (chan)) {
    QString themsg = msg.trimmed();
    if (themsg.startsWith (QChar (':'))) {
      themsg.remove (0,1);
    }
    if (themsg.startsWith (QChar (1))) {
      IncomingCtcpChan (sock, from, chan, themsg);
    } else {
      channels [chan]->Incoming (QString ("<a href=\"ircsender://%1@egalite\">%1</a>: %2")
                                 .arg(from)
                                 .arg(LinkMangle::Sanitize(themsg)),
                                 themsg);
    }
  }
}


void
IrcQmlControl::InUserMsg (IrcSocket * sock, 
                    const QString & from, 
                    const QString & to, 
                    const QString & msg)
{
  if (ignoreSources.contains (from)) {
    return;
  }
  QString themsg = msg.trimmed();
  if (themsg.startsWith (QChar (':'))) {
    themsg.remove (0,1);
  }
  if (themsg.startsWith (QChar (1))) {
    IncomingCtcpUser (sock, from, to, themsg);
  } else {
    IncomingRaw (sock, from, QString ("<a href=\"ircsender://%1@egalite\">%1</a>: %2").
                                arg(from).arg(themsg));
  }
#if 0
  mainUi.logDisplay->append (QString ("Message from %1 to %2 says %3")
                             .arg (from)
                             .arg (to)
                             .arg (msg));
#endif
}

void
IrcQmlControl::IncomingRaw (IrcSocket * sock, 
                         const QString & from, 
                         const QString & msg)
{
  if (!channels.contains (from)) {
    AddChannel (sock, from);
  }
  channels [from]->Incoming (msg);
}

void
IrcQmlControl::SendRaw (QString sockName, QString data)
{
  if (sockets.contains (sockName)) {
    sockets [sockName] -> Send (data);
  }
}

void
IrcQmlControl::IncomingCtcpChan (IrcSocket * sock, 
                    const QString & from, 
                    const QString & chan, 
                    const QString & msg)
{
  QString themsg (msg);
  themsg.chop (1);
  themsg.remove (0,1);
  QRegExp cmdRx ("(\\S+)");
  int pos, len;
qDebug () << " Ctcp CHAN " << msg;
  pos = cmdRx.indexIn (themsg, 0);
  if (pos >=0 ) {
    len = cmdRx.matchedLength ();
    QString cmd = themsg.mid (pos,len);
    if (ctcpHandler.contains (cmd)) {
      (*ctcpHandler [cmd]) (this, sock, from, chan, themsg.mid (pos+len,-1));
      return;
    }
  }
  channels [chan]->Incoming (QString 
                     ("<a href=\"ircsender://%1\">%1</a>:"
                      "<span style=\"font-size:small\">CTCPc</span> %2").
                                  arg(from).arg(LinkMangle::Sanitize(msg)),
                      msg);
}



void
IrcQmlControl::IncomingCtcpUser (IrcSocket * sock, 
                    const QString & from, 
                    const QString & to, 
                    const QString & msg)
{
  QString themsg (msg);
  themsg.chop (1);
  themsg.remove (0,1);
  QRegExp cmdRx ("(\\S+)");
  int pos, len;
  pos = cmdRx.indexIn (themsg, 0);
qDebug () << " Ctcp USER " << msg;
  if (pos >=0 ) {
    len = cmdRx.matchedLength ();
    QString cmd = themsg.mid (pos,len);
    if (ctcpHandler.contains (cmd)) {
      (*ctcpHandler [cmd]) (this, sock, from, from, themsg.mid (pos+len,-1));
      return;
    }
  }
  if (channels.contains (from)) {
    channels [from]->Incoming (QString 
                      ("<a href=\"ircsender://%1\">%1</a>:"
                      "<span style=\"font-size:small\">CTCPu</span> %2")
                          .arg(from).arg(LinkMangle::Sanitize(msg)),
                      msg);
  }
}

void
IrcQmlControl::LogRaw (const QString & raw)
{
// mainUi.logDisplay->append (raw);
  qDebug () << "IrcQmlControl :: LogRaw " << raw;
}

void
IrcQmlControl::AddNames (const QString & chanName, const QString & names)
{
  if (!channels.contains (chanName)) {
    return;
  }
  IrcAbstractChannel * chan = channels [chanName];
  chan->AddNames (names);
}

void
IrcQmlControl::AddName (const QString & chanName, const QString & name)
{
  if (!channels.contains (chanName)) {
    return;
  }
  IrcAbstractChannel * chan = channels [chanName];
  chan->AddName (name);
}

void
IrcQmlControl::DropName (IrcSocket * sock,
                      const QString & chanName, 
                      const QString & name,
                      const QString & msg)
{
  if (!channels.contains (chanName)) {
    return;
  }
  IrcAbstractChannel * chan = channels [chanName];
  chan->DropName (name, msg);
}

void
IrcQmlControl::SetTopic (IrcSocket * sock,
                      const QString & chanName, const QString & topic)
{
  if (!channels.contains (chanName)) {
    return;
  }
  IrcAbstractChannel * chan = channels [chanName];
  chan->SetTopic (topic);
}

void
IrcQmlControl::EditServers ()
{
  EditSimple  edit (tr("IRC Servers"), this);
  edit.SetFuncs (SaveServer, RemoveServer, LoadServers);
  edit.Exec ();
}

void
IrcQmlControl::EditChannels ()
{
  EditSimple  edit (tr("IRC Channels"), this);
  edit.SetFuncs (SaveChannel, RemoveChannel, LoadChannels);
  edit.Exec ();
}

void
IrcQmlControl::EditIgnores ()
{
  EditSimple  edit (tr("IRC Ignores"), this);
  edit.SetFuncs (SaveIgnore, RemoveIgnore, LoadIgnores);
  edit.Exec ();
}

void
IrcQmlControl::SaveServer (const QString & name)
{
  CertStore::IF().SaveIrcServer (name);
}

void
IrcQmlControl::SaveChannel (const QString & name)
{
  CertStore::IF().SaveIrcChannel (name);
}

void
IrcQmlControl::SaveIgnore (const QString & name)
{
  CertStore::IF().SaveIrcIgnore (name);
}

void
IrcQmlControl::RemoveServer (const QString & name)
{
  CertStore::IF().RemoveIrcServer (name);
}

void
IrcQmlControl::RemoveChannel (const QString & name)
{
  CertStore::IF().RemoveIrcChannel (name);
}

void
IrcQmlControl::RemoveIgnore (const QString & name)
{
  CertStore::IF().RemoveIrcIgnore (name);
}

QStringList
IrcQmlControl::LoadServers ()
{
  return CertStore::IF().IrcServers ();
}

QStringList
IrcQmlControl::LoadChannels ()
{
  return CertStore::IF().IrcChannels ();
}

QStringList
IrcQmlControl::LoadIgnores ()
{
  return CertStore::IF().IrcIgnores ();
}

} // namespace

