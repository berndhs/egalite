#include "irc-control.h"

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

#include <QApplication>
#include "deliberate.h"
#include "version.h"
#include "helpview.h"
#include "edit-simple.h"
#include "irc-channel-box.h"
#include "irc-socket.h"
#include "irc-sock-static.h"
#include "irc-ctcp.h"
#include <QSize>
#include "cert-store.h"
#include "irc-channel-group.h"
#include "enter-string.h"
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QCursor>
#include <QHostAddress>
#include <QTimer>
#include <QFont>
#include <QFile>
#include <QListWidgetItem>

using namespace deliberate;

namespace egalite
{

IrcControl::IrcControl (QWidget *parent)
  :QDialog (parent),
   initDone (false),
   isRunning (false),
   isConnected (false),
   hidSelf (false)
{
  mainUi.setupUi (this);
  dockedChannels = new IrcChannelGroup (parentWidget ());
  dockedChannels->hide ();
  ConnectGui ();
  commandXform ["MSG"] = IrcSockStatic::TransformPRIVMSG;
  commandXform ["ME"] = IrcSockStatic::TransformME;
  commandXform ["JOIN"] = IrcSockStatic::TransformJOIN;
  receiveHandler ["PING"] = IrcSockStatic::ReceivePING;
  receiveHandler ["PONG"] = IrcSockStatic::ReceiveIgnore;
  receiveHandler ["PRIVMSG"] = IrcSockStatic::ReceivePRIVMSG;
  receiveHandler ["QUIT"] = IrcSockStatic::ReceiveQUIT;
  receiveHandler ["JOIN"] = IrcSockStatic::ReceiveJOIN;
  receiveHandler ["PART"] = IrcSockStatic::ReceivePART;
  receiveHandler ["004"] = IrcSockStatic::Receive004;
  receiveHandler ["332"] = IrcSockStatic::Receive332;
  receiveHandler ["353"] = IrcSockStatic::Receive353;
  receiveHandler ["366"] = IrcSockStatic::Receive366;
  receiveHandler ["TOPIC"] = IrcSockStatic::ReceiveTOPIC;
  receiveHandler ["VERSION"] = IrcSockStatic::ReceiveIgnore;
  ctcpHandler ["ACTION"] = IrcCtcp::ReceiveACTION;
  ctcpHandler ["VERSION"] = IrcCtcp::ReceiveVERSION;
qDebug () << " IrcControl allocated and initialized";
}

void
IrcControl::Show ()
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
IrcControl::Hide ()
{
  oldSize = size ();
  oldPos = pos();
  hidSelf = true;
  hide ();
}

void
IrcControl::ShowGroup ()
{
  if (!isRunning) {
    Run ();
  }
  if (dockedChannels) {
    dockedChannels->Show ();
  }
}

void
IrcControl::HideGroup ()
{
  if (dockedChannels) {
    dockedChannels->Hide ();
  }
}

void
IrcControl::ShowFloats ()
{
  FloatingMapType::iterator fit;
  for (fit=floatingChannels.begin (); fit != floatingChannels.end(); fit++) {
    fit.value()->Show ();
  }
}

void
IrcControl::HideFloats ()
{
  FloatingMapType::iterator fit;
  for (fit=floatingChannels.begin (); fit != floatingChannels.end(); fit++) {
    fit.value()->Hide ();
  }
}

bool
IrcControl::Run ()
{
  if (!isRunning) {
    qDebug () << " Start IrcControl";
    QSize defaultSize = size();
    QSize newsize = Settings().value ("sizes/ircsock", defaultSize).toSize();
    resize (newsize);
    QSize  groupBoxSize = dockedChannels->size();
    groupBoxSize = Settings().value ("sizes/channelgroup", groupBoxSize)
                             .toSize();
    dockedChannels->resize (groupBoxSize);
  }

  LoadLists ();
  show ();

  isRunning = true;
  return true;
}

void
IrcControl::LoadLists ()
{
  QStringList  servers = CertStore::IF().IrcServers ();
  noNameServer = tr("--- New Server ---");
  servers.append (noNameServer);
  mainUi.serverCombo->clear ();
  mainUi.serverCombo->insertItems (0,servers);

  QStringList  nicks = CertStore::IF().IrcNicks ();
  noNameNick = tr ("--- New Nick ---");
  nicks.append (noNameNick);
  mainUi.nickCombo->clear ();
  mainUi.nickCombo->insertItems (0,nicks);

  QStringList  chans = CertStore::IF().IrcChannels ();
  noNameChannel = tr("--- New Channel ---");
  chans.append (noNameChannel);
  mainUi.chanCombo->clear ();
  mainUi.chanCombo->insertItems (0,chans);

  ignoreSources = CertStore::IF().IrcIgnores ();
}

void
IrcControl::ConnectGui ()
{
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
}

void
IrcControl::Exit ()
{
  dockedChannels->Close ();
  TryDisconnect ();
  CloseCleanup ();
  hide ();
}


void
IrcControl::CloseCleanup ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/ircsock",currentSize);
  QSize  groupBoxSize = dockedChannels->size();
  Settings().setValue ("sizes/channelgroup", groupBoxSize);
  Settings().sync();
}

void
IrcControl::HideAll ()
{
  HideGroup ();
  HideFloats ();
}

void
IrcControl::ShowAll ()
{
  ShowGroup ();
  ShowFloats ();
}

void
IrcControl::Exiting ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/ircsock",currentSize);
  Settings().sync();
}

void
IrcControl::TryConnect ()
{
  QString host = mainUi.serverCombo->currentText();
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
  quint16 port = mainUi.portBox->value ();
  socket->connectToHost (host, port);
}

void
IrcControl::TryJoin ()
{
  QString chan = mainUi.chanCombo->currentText ();
  if (chan == noNameChannel) {
    EnterString enter (this);
    bool picked = enter.Choose (tr("IRC Channel"), tr("Channel:"));
    if (picked) {
      chan = enter.Value ();
      if (enter.Save()) {
        CertStore::IF().SaveIrcChannel (chan);
        mainUi.chanCombo->clear ();
        QStringList knownChans = CertStore::IF().IrcChannels ();
        knownChans.append (noNameChannel);
        mainUi.chanCombo->addItems (knownChans);
      }
    } else {
      return;
    }
  }
  IrcSocket * sock = CurrentSock (mainUi.serverTable);
  if (sock == 0) {
    return;
  }
  sock->Send (QString ("JOIN %1").arg(chan));
}

void
IrcControl::TryPart ()
{
  QString chan = mainUi.chanCombo->currentText ();
  IrcSocket * sock = CurrentSock (mainUi.serverTable);
  sock->Send (QString ("PART %1").arg (chan));
}

int
IrcControl::OpenCount ()
{
  return sockets.size();
}

void
IrcControl::TryDisconnect ()
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
IrcControl::ReceiveLine (IrcSocket * sock, QByteArray line)
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
      IrcSockStatic::ReceiveNumeric (this, sock, first, cmd, rest);
    } else {
      IrcSockStatic::ReceiveDefault (this, sock, first, cmd, rest);
    }
  } else {
    if (data.startsWith ("PING")) {
      QString retName = data.split (QRegExp("(\\s+)")).at(1);
      sock->Send (QString ("PONG :%1").arg(retName));
    }
  }
  mainUi.logDisplay->append (QString (line));\
}

void
IrcControl::Send ()
{
  int row = mainUi.serverTable->currentRow ();
  QTableWidgetItem * item = FindType (mainUi.serverTable, row, int (Cell_Addr));
  if (item) {
    QString sname = item->data (int(Data_ConnName)).toString();
    if (sockets.contains (sname)) {
      QString text = mainUi.sendEdit->text ();
      IrcSocket * sock = sockets [sname];
      TransformSend (sock, "", text);
      sock->Send (text);
    }
  }
}

void
IrcControl::TransformSend (IrcSocket * sock, const QString & chan, 
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
        IrcSockStatic::TransformDefault (this, sock, data, copyChan, first, rest);
      }
    }
  }
}


void
IrcControl::ConnectionReady (IrcSocket * sock)
{
  mainUi.logDisplay->append ("Connection Ready");
  mainUi.logDisplay->append (QString ("peer address %1")
                 .arg (sock->peerAddress().toString()));
  qDebug () << " connection ready " << sock->peerAddress().toString();
  AddConnect (sock);
  emit StatusChange ();
}

void
IrcControl::AddConnect (IrcSocket *sock)
{
  int nrows = mainUi.serverTable->rowCount();
  QTableWidgetItem * item = 
          new QTableWidgetItem (sock->HostName(), int(Cell_Name));
  int row (nrows);
  mainUi.serverTable->setRowCount (nrows+1);
  mainUi.serverTable->setItem (row, 1, item);
  item = new QTableWidgetItem (sock->peerAddress().toString(),
                               int (Cell_Addr));
  item->setData (Data_ConnName, sock->Name());
  mainUi.serverTable->setItem (row, 2, item);
  item = new QTableWidgetItem (QString::number (sock->peerPort()),
                               int (Cell_Port));
  mainUi.serverTable->setItem (row, 3, item);
  item = new QTableWidgetItem (tr("Disconnect"), int (Cell_Action));
  mainUi.serverTable->setItem (row, 0, item);
  mainUi.serverTable->selectRow (row);
}

void
IrcControl::ChangedHostName (IrcSocket * sock, QString name)
{
qDebug () << " have new host name " << name << " for " << sock;
  if (sock == 0) {
    return;
  }
  QString sname = sock->Name();
  int row = FindRow (mainUi.serverTable, sname);
  QTableWidgetItem * item = FindType (mainUi.serverTable, row, int(Cell_Name));
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
IrcControl::FindType (QTableWidget * table, int row, int type)
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
IrcControl::FindRow (QTableWidget * table, const QString & sname)
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
IrcControl::ConnectionGone (IrcSocket * sock)
{
  qDebug () << " disconnect seen for " << sock;
  mainUi.logDisplay->append (QString ("!!! disconnected from %1")
                 .arg (sock->peerAddress().toString()));
  RemoveConnect (sock);
  sockets.remove (sock->Name());
  sock->deleteLater ();
  emit StatusChange ();
}

void
IrcControl::RemoveConnect (IrcSocket * sock)
{
  int row, col;
  int nrows = mainUi.serverTable->rowCount ();
  int ncols = mainUi.serverTable->columnCount ();
  QString sname = sock->Name();
  QTableWidgetItem *item;  
  bool looking (true);
  for (row=0; looking && row< nrows; row++) {
    for (col=0; looking && col< ncols; col++) {
      item = FindType (mainUi.serverTable, row, int (Cell_Addr));
      if (item && item->data (int(Data_ConnName)).toString() == sname) {
        mainUi.serverTable->removeRow (row);
        looking = false;
      }
    }
  }
}


void
IrcControl::NickLogin ()
{
  IrcSocket * sock = CurrentSock (mainUi.serverTable);
  if (sock == 0) {
    return;
  }
  QString nick = mainUi.nickCombo->currentText();
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
IrcControl::CurrentSock (QTableWidget * table)
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
IrcControl::AddChannel (IrcSocket * sock, const QString & chanName)
{
  if (channels.contains (chanName)) {
    return;
  }
  if (sock == 0) {
    return;
  }
  IrcChannelBox * newchan  = new IrcChannelBox (chanName, sock->Name(), this);
  channels [chanName] = newchan;
  dockedChannels->AddChannel (newchan);
  dockedChannels->show ();
  newchan->SetHost (sock->HostName());
  newchan->SetPartMsg (sock->PartMsg ());
  connect (newchan, SIGNAL (Outgoing (QString, QString)),
           this, SLOT (Outgoing (QString, QString)));
  connect (newchan, SIGNAL (Active (IrcChannelBox *)),
           this, SLOT (ChanActive (IrcChannelBox *)));
  connect (newchan, SIGNAL (InUse (IrcChannelBox *)),
           this, SLOT (ChanInUse (IrcChannelBox *)));
  connect (newchan, SIGNAL (WantFloat (IrcChannelBox *)),
           this, SLOT (ChanWantsFloat (IrcChannelBox *)));
  connect (newchan, SIGNAL (WantDock (IrcChannelBox *)),
           this, SLOT (ChanWantsDock (IrcChannelBox *)));
  connect (newchan, SIGNAL (HideAllChannels ()),
           this, SLOT (HideAll ()));
  connect (newchan, SIGNAL (HideDock ()),
           this, SLOT (HideGroup ()));
  connect (newchan, SIGNAL (HideChannel (IrcChannelBox *)),
           this, SLOT (HideChannel (IrcChannelBox *)));
  mainUi.chanList->addItem (chanName);
}

void
IrcControl::DropChannel (IrcSocket * sock, const QString & chanName)
{
  Q_UNUSED (sock)
  if (!channels.contains (chanName)) {
    return;
  }
  IrcChannelBox * chanBox = channels [chanName];
  disconnect (chanBox, 0,0,0);
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
  int nc = mainUi.chanList->count();
  for (int c=nc-1; c>=0; c--) {
    QListWidgetItem *item = mainUi.chanList->item (c);
    if (item && item->text() == chanName) {
      mainUi.chanList->takeItem (c);
      delete item;
    }
  }
}

void
IrcControl::ChannelClicked (QListWidgetItem *item)
{
  if (item) {
    QString chanName = item->text ();
    qDebug () << " clicked on channel " << chanName;
    IrcChannelBox * chanBox = channels [chanName];
    qDebug () << "       is in box " << chanBox;
    ShowChannel (chanBox);
    chanBox->raise ();
  }
}

void
IrcControl::ShowChannel (IrcChannelBox * chanBox)
{
  if (dockedChannels->HaveChannel (chanBox)) {
    dockedChannels->ShowChannel (chanBox);
  } else if (floatingChannels.contains (chanBox)) {
    floatingChannels [chanBox]->Show ();
  } 
}

void
IrcControl::HideChannel (IrcChannelBox * chanBox)
{
  if (floatingChannels.contains (chanBox)) {
    floatingChannels [chanBox]->Hide();
  }
}

void
IrcControl::ServerClicked (QTableWidgetItem *item)
{
  if (item) {
    qDebug () << " clicked socked table item " << item 
              << " row " << item->row()
              << " " << item->text ();
    if (item->text () == tr("Disconnect")) {
      QTableWidgetItem * aitem = FindType (mainUi.serverTable, 
                                           item->row(), 
                                           int (Cell_Addr));
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
IrcControl::PartAll (const QString & sockName)
{
  ChannelMapType::iterator cit;
  for (cit=channels.begin(); cit != channels.end(); cit++) {
    if (*cit) {
      IrcChannelBox * chan = *cit;
      if (chan->Sock() == sockName) {
        chan->Part ();
      }
    }
  }
}

void
IrcControl::UserQuit (IrcSocket * sock,
                      const QString & user,
                      const QString & msg)
{
  QString sockName = sock->Name();
  QString quitMsg = QString (tr("QUIT ") + msg);
  ChannelMapType::iterator cit;
  for (cit=channels.begin(); cit != channels.end(); cit++) {
    if (*cit) {
      IrcChannelBox * chan = *cit;
      if (chan->Sock() == sockName) {
        chan->DropName (user, quitMsg);
      }
    }
  }
}

void
IrcControl::ChanActive (IrcChannelBox *chan)
{
  dockedChannels->MarkActive (chan, true);
}

void
IrcControl::ChanInUse (IrcChannelBox *chan)
{
  dockedChannels->MarkActive (chan, false);
}

void
IrcControl::ChanWantsFloat (IrcChannelBox *chan)
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
IrcControl::ChanWantsDock (IrcChannelBox *chan)
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
IrcControl::Outgoing (QString chan, QString msg)
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
qDebug () << " handed to socket: " << cooked;
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
IrcControl::InChanMsg (IrcSocket * sock,
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
      channels [chan]->Incoming (QString ("<a href=\"ircsender://%1\">%1</a>: %2").
                                  arg(from).arg(themsg));
    }
  }
}


void
IrcControl::InUserMsg (IrcSocket * sock, 
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
    IncomingRaw (sock, from, QString ("<a href=\"ircsender://%1\">%1</a>: %2").
                                arg(from).arg(themsg));
  }
  mainUi.logDisplay->append (QString ("Message from %1 to %2 says %3")
                             .arg (from)
                             .arg (to)
                             .arg (msg));
}

void
IrcControl::IncomingRaw (IrcSocket * sock, 
                         const QString & from, 
                         const QString & msg)
{
  if (!channels.contains (from)) {
    AddChannel (sock, from);
  }
  channels [from]->Incoming (msg);
}

void
IrcControl::IncomingCtcpChan (IrcSocket * sock, 
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
                                  arg(from).arg(msg));
}



void
IrcControl::IncomingCtcpUser (IrcSocket * sock, 
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
                      "<span style=\"font-size:small\">CTCPu</span> %2").
                                  arg(from).arg(msg));
  }
}

void
IrcControl::LogRaw (const QString & raw)
{
  mainUi.logDisplay->append (raw);
}

void
IrcControl::AddNames (const QString & chanName, const QString & names)
{
  if (!channels.contains (chanName)) {
    return;
  }
  IrcChannelBox * chan = channels [chanName];
  chan->AddNames (names);
}

void
IrcControl::AddName (const QString & chanName, const QString & name)
{
  if (!channels.contains (chanName)) {
    return;
  }
  IrcChannelBox * chan = channels [chanName];
  chan->AddName (name);
}

void
IrcControl::DropName (IrcSocket * sock,
                      const QString & chanName, 
                      const QString & name,
                      const QString & msg)
{
  if (!channels.contains (chanName)) {
    return;
  }
  IrcChannelBox * chan = channels [chanName];
  chan->DropName (name, msg);
}

void
IrcControl::SetTopic (IrcSocket * sock,
                      const QString & chanName, const QString & topic)
{
  if (!channels.contains (chanName)) {
    return;
  }
  IrcChannelBox * chan = channels [chanName];
  chan->SetTopic (topic);
}

void
IrcControl::EditServers ()
{
  EditSimple  edit (tr("IRC Servers"), this);
  edit.SetFuncs (SaveServer, RemoveServer, LoadServers);
  edit.Exec ();
}

void
IrcControl::EditChannels ()
{
  EditSimple  edit (tr("IRC Channels"), this);
  edit.SetFuncs (SaveChannel, RemoveChannel, LoadChannels);
  edit.Exec ();
}

void
IrcControl::EditIgnores ()
{
  EditSimple  edit (tr("IRC Ignores"), this);
  edit.SetFuncs (SaveIgnore, RemoveIgnore, LoadIgnores);
  edit.Exec ();
}

void
IrcControl::SaveServer (const QString & name)
{
  CertStore::IF().SaveIrcServer (name);
}

void
IrcControl::SaveChannel (const QString & name)
{
  CertStore::IF().SaveIrcChannel (name);
}

void
IrcControl::SaveIgnore (const QString & name)
{
  CertStore::IF().SaveIrcIgnore (name);
}

void
IrcControl::RemoveServer (const QString & name)
{
  CertStore::IF().RemoveIrcServer (name);
}

void
IrcControl::RemoveChannel (const QString & name)
{
  CertStore::IF().RemoveIrcChannel (name);
}

void
IrcControl::RemoveIgnore (const QString & name)
{
  CertStore::IF().RemoveIrcIgnore (name);
}

QStringList
IrcControl::LoadServers ()
{
  return CertStore::IF().IrcServers ();
}

QStringList
IrcControl::LoadChannels ()
{
  return CertStore::IF().IrcChannels ();
}

QStringList
IrcControl::LoadIgnores ()
{
  return CertStore::IF().IrcIgnores ();
}

} // namespace

