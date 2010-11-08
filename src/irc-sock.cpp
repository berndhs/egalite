#include "irc-sock.h"

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
#include "irc-channel-box.h"
#include "irc-channel-group.h"
#include "cert-store.h"
#include "enter-string.h"
#include <QSize>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QCursor>
#include <QHostAddress>
#include <QTimer>
#include <QFont>
#include <QFile>

using namespace deliberate;

namespace egalite
{

IrcSock::IrcSock (QWidget *parent)
  :QDialog (parent),
   initDone (false),
   socket (0),
   pingTimer (0),
   scriptTimer (0),
   waitFirstReceive (false)
{
  mainUi.setupUi (this);
  dockedChannels = new IrcChannelGroup (this);
  dockedChannels->hide ();
  socket = new QTcpSocket (this);
  pingTimer = new QTimer (this);
  scriptTimer = new QTimer (this);
  connect (pingTimer, SIGNAL (timeout()),
           this, SLOT (SendPing()));
  connect (scriptTimer, SIGNAL (timeout()), 
           this, SLOT (SendScriptHead()));
  Connect ();
  commandXform ["MSG"] = IrcSock::TransformPRIVMSG;
  commandXform ["JOIN"] = IrcSock::TransformJOIN;
  receiveHandler ["PING"] = IrcSock::ReceivePING;
  receiveHandler ["PONG"] = IrcSock::ReceiveIgnore;
  receiveHandler ["PRIVMSG"] = IrcSock::ReceivePRIVMSG;
  receiveHandler ["JOIN"] = IrcSock::ReceiveJOIN;
  receiveHandler ["PART"] = IrcSock::ReceivePART;
  receiveHandler ["332"] = IrcSock::Receive332;
  receiveHandler ["353"] = IrcSock::Receive353;
  receiveHandler ["366"] = IrcSock::Receive366;
  receiveHandler ["VERSION"] = IrcSock::ReceiveIgnore;
qDebug () << " IrcSock allocated and initialized";
}

bool
IrcSock::Run ()
{
  qDebug () << " Start IrcSock";
  QSize defaultSize = size();
  QSize newsize = Settings().value ("sizes/ircsock", defaultSize).toSize();
  resize (newsize);
  QSize  groupBoxSize = dockedChannels->size();
  groupBoxSize = Settings().value ("sizes/channelgroup", groupBoxSize)
                           .toSize();
  dockedChannels->resize (groupBoxSize);
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

  show ();
  return true;
}

void
IrcSock::Connect ()
{
  connect (socket, SIGNAL (readyRead()),
           this, SLOT (Receive ()));
  connect (socket, SIGNAL (connected ()),
           this, SLOT (ConnectionReady ()));
  connect (socket, SIGNAL (disconnected ()),
           this, SLOT (ConnectionGone ())); 
  connect (mainUi.connectButton, SIGNAL (clicked()),
           this, SLOT (TryConnect ()));
  connect (mainUi.disconnectButton, SIGNAL (clicked()),
           this, SLOT (TryDisconnect ()));
  connect (mainUi.sendButton, SIGNAL (clicked()),
           this, SLOT (Send ()));
  connect (mainUi.sendEdit, SIGNAL (returnPressed()),
           this, SLOT (Send ()));
  connect (mainUi.loginButton, SIGNAL (clicked()),
           this, SLOT (FakeLogin ()));
  connect (mainUi.exitButton, SIGNAL (clicked ()),
           this, SLOT (Exit ()));
}

void
IrcSock::Exit ()
{
  TryDisconnect ();
  CloseCleanup ();
  dockedChannels->Close ();
  hide ();
}


void
IrcSock::CloseCleanup ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/ircsock",currentSize);
  QSize  groupBoxSize = dockedChannels->size();
  Settings().setValue ("sizes/channelgroup", groupBoxSize);
  Settings().sync();
}

void
IrcSock::Exiting ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/ircsock",currentSize);
  Settings().sync();
}

void
IrcSock::TryConnect ()
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
  quint16 port = mainUi.portBox->value ();
  socket->connectToHost (host, port, QTcpSocket::ReadWrite);
  waitFirstReceive = true;
}

void
IrcSock::TryDisconnect ()
{
  socket->disconnectFromHost ();
}

void
IrcSock::Receive ()
{
  mainUi.logDisplay->append (QString ("!!! data ready %1 bytes")
                          .arg (socket->bytesAvailable ()));
  QByteArray bytes = socket->readAll ();
qDebug () << " got " << bytes.size() << " bytes " << bytes;
  QByteArray last2 = lineData.right(2);
  if (last2.size () < 2) {
    last2.prepend ("??");
    last2 = last2.right (2);
  }
  int nb = bytes.size();
  char byte;
  char last0, last1;
  last0 = last2[0];
  last1 = last2[1];
  for (int b=0; b< nb; b++) {
    byte = bytes[b];
    lineData.append (byte);
    last0 = last1;
    last1 = byte;
    if (last0 == '\r' && last1 == '\n') {
      QByteArray lineCopy = lineData;
      ReceiveLine (lineCopy);
      lineData.clear ();
    }
  }
}

void
IrcSock::ReceiveLine (const QByteArray & line)
{
  QString data (QString::fromUtf8(line.data()));
qDebug () << " received " << data;
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
      if (waitFirstReceive) {
        waitFirstReceive = false;
        currentServer = first;
        qDebug () << " set current server " << currentServer;
      }
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
      (*receiveHandler [cmd]) (this, first, cmd, rest);
    } else if (numericRx.exactMatch (cmd)) {
      ReceiveNumeric (this, first, cmd, rest);
    } else {
      ReceiveDefault (this, first, cmd, rest);
    }
  }  
  mainUi.logDisplay->append (QString (line));\
}

void
IrcSock::SendPing ()
{
  qDebug () << "send ping goes to " << currentServer;
  QString msg (QString ("PING %1").arg (currentServer));
  SendData (msg);
  
}

void
IrcSock::SendData (const QString & data)
{
  QString copyStr (data);
  copyStr.append ("\r\n");
  QByteArray copy = copyStr.toUtf8();
  qint64 written = socket->write (copy);
qDebug () << " sent to socket: " << copy;
  mainUi.logDisplay->append (data);
  mainUi.logDisplay->append (QString ("!!! wrote %1 bytes").arg (written));
}

void
IrcSock::Send ()
{
  Send (mainUi.sendEdit->text());
}


void
IrcSock::Send (QString data)
{
  if (data.startsWith(QChar ('/'))) {
    QRegExp wordRx ("(\\S+)");
    int pos = wordRx.indexIn (data, 1);
    if (pos >= 0) {
      int len = wordRx.matchedLength ();
      QString first = data.mid (1,len).toUpper();
      QString rest = data.mid (len+1,-1);
      if (commandXform.contains (first)) {
        (*commandXform[first]) (this, data, first, rest);
      } else {
        TransformDefault (this, data, first, rest);
      }
    }
  }
  scriptLines += data;
  RollScript ();
}

void
IrcSock::RollScript ()
{
  SendScriptHead ();
  scriptTimer->start (1000);
}

void
IrcSock::SendScriptHead ()
{
  if (scriptLines.isEmpty()) {
    scriptTimer->stop ();
    return;
  }
  QString line = scriptLines.takeFirst();
  SendData (line);
}

void
IrcSock::DidSend (qint64 bytes)
{
  mainUi.logDisplay->append (QString ("!!! did send %1 bytes").arg(bytes));
}

void
IrcSock::ConnectionReady ()
{
  waitFirstReceive = true;
  pingTimer->start (1*60*1000);
  QTimer::singleShot (30000, this, SLOT (SendPing()));
  mainUi.logDisplay->append ("!!! Connection Ready");
  mainUi.logDisplay->append (QString ("!!! peer address %1")
                 .arg (socket->peerAddress().toString()));
  mainUi.peerAddressLabel->setText (socket->peerAddress().toString());
  qDebug () << " connection ready " << socket->peerAddress().toString();
  QFont font = mainUi.peerAddressLabel->font ();
  font.setStrikeOut (false);
  mainUi.peerAddressLabel->setFont (font);
  ignoreSources = CertStore::IF().IrcIgnores ();
}

void
IrcSock::ConnectionGone ()
{
  pingTimer->stop ();
  qDebug () << " disconnect seen for " << socket;
  mainUi.logDisplay->append (QString ("!!! disconnected from %1")
                 .arg (socket->peerAddress().toString()));
  QFont font = mainUi.peerAddressLabel->font ();
  font.setStrikeOut (true);
  mainUi.peerAddressLabel->setFont (font);
}

void 
IrcSock::SockError (QAbstractSocket::SocketError err)
{
  qDebug () << " socket error " << err;
  qDebug () << " socket error text " << socket->errorString ();
}

void
IrcSock::FakeLogin ()
{
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
  bool  havePass = CertStore::IF().GetIrcIdent (nick, pass, real);
  if (real.length() == 0) {
    real = nick;
  }
  if (havePass) {
    scriptLines.append (QString ("PASS :%1").arg(pass));
  }
  scriptLines.append (QString ("USER %1 0 * :%2").arg (nick).arg(real));
  scriptLines.append (QString ("NICK %1").arg (nick));
  QTimer::singleShot (100, this, SLOT (RollScript ()));
  currentUser = nick;
}

void
IrcSock::AddChannel (const QString & chanName)
{
  if (channels.contains (chanName)) {
    return;
  }
  IrcChannelBox * newchan  = new IrcChannelBox (chanName, this);
  channels [chanName] = newchan;
  dockedChannels->AddChannel (newchan);
  dockedChannels->show ();
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
}

void
IrcSock::DropChannel (const QString & chanName)
{
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
}

void
IrcSock::ChanActive (IrcChannelBox *chan)
{
  dockedChannels->MarkActive (chan, true);
}

void
IrcSock::ChanInUse (IrcChannelBox *chan)
{
  dockedChannels->MarkActive (chan, false);
}

void
IrcSock::ChanWantsFloat (IrcChannelBox *chan)
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
IrcSock::ChanWantsDock (IrcChannelBox *chan)
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
IrcSock::Outgoing (QString chan, QString msg)
{
  QString trim = msg.trimmed ();
  if (trim.length () < 1) {
    return;
  }
  if (trim.startsWith (QChar ('/')) ){
    Send (trim);
  } else {
    QString data (QString ("PRIVMSG %1 :%2").arg (chan). arg (msg));
    SendData (data);
  }
}

void
IrcSock::InChanMsg (const QString & chan, 
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
    channels [chan]->Incoming (QString ("<a href=\"ircsender://%1\">%1</a>: %2").
                                  arg(from).arg(themsg));
  }
}


void
IrcSock::InUserMsg (const QString & from, 
                    const QString & to, 
                    const QString & msg)
{
  if (ignoreSources.contains (from)) {
    return;
  }
  if (!channels.contains (from)) {
    AddChannel (from);
  }
  if (channels.contains (from)) {
    QString themsg = msg.trimmed();
    if (themsg.startsWith (QChar (':'))) {
      themsg.remove (0,1);
    }
    channels [from]->Incoming (QString ("<a href=\"ircsender://%1\">%1</a>: %2").
                                  arg(from).arg(themsg));
  }
  mainUi.logDisplay->append (QString ("Message from %1 to %2 says %3")
                             .arg (from)
                             .arg (to)
                             .arg (msg));
}

void
IrcSock::LogRaw (const QString & raw)
{
  mainUi.logDisplay->append (raw);
}

void
IrcSock::AddNames (const QString & chanName, const QString & names)
{
  if (!channels.contains (chanName)) {
    return;
  }
  IrcChannelBox * chan = channels [chanName];
  chan->AddNames (names);
}

void
IrcSock::AddName (const QString & chanName, const QString & name)
{
  if (!channels.contains (chanName)) {
    return;
  }
  IrcChannelBox * chan = channels [chanName];
  chan->AddName (name);
}

void
IrcSock::DropName (const QString & chanName, const QString & name)
{
  if (!channels.contains (chanName)) {
    return;
  }
  IrcChannelBox * chan = channels [chanName];
  chan->DropName (name);
}

void
IrcSock::SetTopic (const QString & chanName, const QString & topic)
{
  if (!channels.contains (chanName)) {
    return;
  }
  IrcChannelBox * chan = channels [chanName];
  chan->SetTopic (topic);
}

void
IrcSock::TransformDefault (IrcSock * context,
                           QString & result, 
                           QString & first, 
                           QString & rest)
{
  Q_UNUSED (context)
  result = first + rest;
}

void
IrcSock::TransformPRIVMSG (IrcSock * context,
                           QString & result, 
                           QString & first, 
                           QString & rest)
{
  Q_UNUSED (context)
  first = "PRIVMSG";
  QRegExp wordRx ("(\\S+)");
  int pos = wordRx.indexIn (rest, 0);
  if (pos >= 0) {
    int len = wordRx.matchedLength ();
    rest.insert (pos + len, " :");
  }
  result = first + " " + rest;
}

void 
IrcSock::TransformJOIN (IrcSock * context,
                        QString & result,
                        QString & first,
                        QString & rest)
{
  result = first + rest;
}

void
IrcSock::ReceivePING (IrcSock * context,
                      const QString & first,
                      const QString & cmd,
                      const QString & rest)
{
  Q_UNUSED (cmd)
  Q_UNUSED (rest)
  QString answer = QString ("PONG %1 %2").arg(first);
  context->SendData (answer);
}

void
IrcSock::ReceiveJOIN (IrcSock * context,
                      const QString & first,
                      const QString & cmd,
                      const QString & rest)
{
  Q_UNUSED (cmd)
  QRegExp wordRx ("(\\S+)");
  int pos, len;
  QString user, chan;
  pos = wordRx.indexIn (first, 0);
  if (pos >= 0) {
    len = wordRx.matchedLength ();
    user = first.mid (pos,len);
    QRegExp leadRx ("([^!]+)");
    pos = leadRx.indexIn (user,0);
    if (pos >= 0) {
      user = user.mid (pos, leadRx.matchedLength());
    }
    if (user.startsWith (QChar (':'))) {
      user.remove (0,1);
    }
  }
  pos = wordRx.indexIn (rest,0);
  if (pos >= 0) {
    len = wordRx.matchedLength ();
    chan = rest.mid (pos,len);
qDebug () << " JOIN received,  " << first << cmd << rest;
qDebug () << "user " << user << " currentUser " 
          << context->currentUser << " chan " << chan;
    if (chan.startsWith (QChar(':'))) {
      chan.remove (0,1);
    }
    if (user == context->currentUser) {
      if (!context->channels.contains (chan)) {
        context->AddChannel (chan);
      }
    } else {
      context->AddName (chan, user);
      context->mainUi.logDisplay->append (QString ("user %1 JOINs %2")
                                    .arg (user) . arg (chan));
    }
  }
}

void
IrcSock::ReceivePART (IrcSock * context,
                     const QString & first,
                     const QString & cmd,
                     const QString & rest)
{
  qDebug () << " PART received " << first << cmd << rest;
  QRegExp wordRx ("(\\S+)");
  int pos, len;
  QString user, chan;
  pos = wordRx.indexIn (first, 0);
  if (pos >= 0) {
    len = wordRx.matchedLength ();
    user = first.mid (pos,len);
    QRegExp leadRx ("([^!]+)");
    pos = leadRx.indexIn (user,0);
    if (pos >= 0) {
      user = user.mid (pos, leadRx.matchedLength());
    }
    if (user.startsWith (QChar (':'))) {
      user.remove (0,1);
    }
  }
  pos = wordRx.indexIn (rest,0);
  if (pos >= 0) {
    len = wordRx.matchedLength ();
    chan = rest.mid (pos,len);
    if (chan.startsWith (QChar (':'))) {
      chan.remove (0,1);
    }
qDebug () << " PART received for channel " << chan;
qDebug () << "user " << user << " currentUser " 
          << context->currentUser << " chan " << chan;
    if (user == context->currentUser) {
      context->DropChannel (chan);
    } else {
      context->DropName (chan, user);
      context->mainUi.logDisplay->append (QString ("user %1 PARTs %2")
                                    .arg (user) . arg (chan));
    }
  }
}

void
IrcSock::ReceivePRIVMSG (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest)
{
  Q_UNUSED (cmd)
  int pos, len;
  QRegExp sourceRx ("(\\S+)!");
  QString source (first);
  QString dest;
  QString msg;
  QRegExp wordRx ("(\\S+)");
  pos = sourceRx.indexIn (first,0);
  if (pos >= 0) {
    len = sourceRx.matchedLength ();
    source = first.mid (pos,len);
    source.chop (1);
    if (source.startsWith (QChar (':'))) {
      source.remove (0,1);
    }
  }
  pos = wordRx.indexIn (rest,0);
  if (pos >= 0) {
    len = wordRx.matchedLength ();
    dest = rest.mid (pos,len);
    msg = rest.mid (pos+len,-1);
    if (context->channels.contains (dest)) {
      context->InChanMsg (dest, source, msg);
    } else {
      context->InUserMsg (source, dest, msg);
    }
  }
}

void
IrcSock::ReceiveNumeric (IrcSock * context,
                        const QString & first,
                        const QString & num,
                        const QString & rest)
{
  context->LogRaw (QString ("numeric %1  %2 %3").arg(first).arg(num).arg(rest));
}

void
IrcSock::ReceiveDefault (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest)
{
  qDebug () << "Default Receiver " << context << first << cmd << rest;
}

void
IrcSock::ReceiveIgnore (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest)
{
  qDebug () << " Ignoring command " << cmd;
}

void
IrcSock::Receive332 (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest)
{
  qDebug () << " Received 332 " << first << cmd << rest;
  QRegExp wordRx ("(\\S+)");
  int pos, len;
  QString user, chan, topic;
  pos = wordRx.indexIn (rest, 0);
  if (pos >= 0) {
    len = wordRx.matchedLength();
    user = rest.mid (pos, len);
    pos = wordRx.indexIn (rest, pos+len);
    if (pos >= 0) {
      len = wordRx.matchedLength ();
      chan = rest.mid (pos, len);
      topic = rest.mid (pos+len, -1).trimmed ();
      if (topic.startsWith (QChar (':'))) {
        topic.remove (0,1);
      }
      context->SetTopic (chan, topic);
    }
  }
}

void
IrcSock::Receive353 (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest)
{
  qDebug () << " Received 353 " << first << cmd << rest;
  QRegExp wordRx ("(\\S+)");
  int pos, len;
  QString user, marker, chan;
  pos = wordRx.indexIn (rest, 0);
  if (pos >= 0) {
    len = wordRx.matchedLength();
    user = rest.mid (pos, len);
    pos = wordRx.indexIn (rest, pos+len);
    if (pos >= 0) {
      len = wordRx.matchedLength ();
      marker = rest.mid (pos, len);
      pos = wordRx.indexIn (rest, pos+len);
      if (pos >= 0) {
        len = wordRx.matchedLength ();
        chan = rest.mid (pos,len);
        QString nameData = rest.mid (pos+len,-1).trimmed ();
        if (nameData.startsWith (QChar(':'))) {
          nameData.remove (0,1);
        }
        context->AddNames (chan, nameData);
      }
    }
  }
}


void
IrcSock::Receive366 (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest)
{
  qDebug () << " Received 366 " << first << cmd << rest;
}

} // namespace
