#include "ircsock.h"

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
   logIncoming (0),
   logOutgoing (0),
   pingTimer (0),
   scriptTimer (0),
   waitFirstReceive (false)
{
  mainUi.setupUi (this);
  channelGroup = new IrcChannelGroup (this);
  channelGroup->hide ();
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
  QSize  groupBoxSize = channelGroup->size();
  groupBoxSize = Settings().value ("sizes/channelgroup", groupBoxSize)
                           .toSize();
  channelGroup->resize (groupBoxSize);
  Settings().setValue ("sizes/ircsock",newsize);
  QString defServ = Settings().value ("defaults/ircserver", 
                     QString ("chat.freenode.net")).toString();
  Settings().setValue ("defaults/ircserver",defServ);
  mainUi.serverEdit->setText (defServ);
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
  connect (mainUi.sendScriptButton, SIGNAL (clicked()),
           this, SLOT (SendScript ()));
  connect (mainUi.loginButton, SIGNAL (clicked()),
           this, SLOT (FakeLogin ()));
}


void
IrcSock::CloseCleanup ()
{
  if (logIncoming) {
    logIncoming->write (QByteArray("Finished Logging Incoming "));
    logIncoming->write (QDateTime::currentDateTime().toString().toUtf8());
    logIncoming->write (QByteArray ("\n"));
  }
  if (logOutgoing) {
    logOutgoing->write (QByteArray("Finished Logging Outgoing "));
    logOutgoing->write (QDateTime::currentDateTime().toString().toUtf8());
    logOutgoing->write (QByteArray ("\n"));
  }
  QSize currentSize = size();
  Settings().setValue ("sizes/main",currentSize);
  QSize  groupBoxSize = channelGroup->size();
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
IrcSock::SetInLog (const QString & filename)
{
  logIncoming = new QFile (filename, this);
  bool isopen = logIncoming->open (QFile::WriteOnly);
  if (isopen) {
    qDebug () << " Logging to file " << logIncoming->fileName();
    logIncoming->write (QByteArray ("Start logging"));
    logIncoming->write (QDateTime::currentDateTime().toString().toUtf8());
  } else {
    qDebug () << " Cannot open log file to write " << filename;
  }
}

void
IrcSock::SetOutLog (const QString & filename)
{
  logOutgoing = new QFile (filename, this);
  bool isopen = logOutgoing->open (QFile::WriteOnly);
  if (isopen) {
    qDebug () << " Logging to file " << logOutgoing->fileName();
    logOutgoing->write (QByteArray ("Start logging"));
    logOutgoing->write (QDateTime::currentDateTime().toString().toUtf8());
  } else {
    qDebug () << " Cannot open log file to write " << filename;
  }
}

void
IrcSock::TryConnect ()
{
  QString host = mainUi.serverEdit->text();
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
  mainUi.inLog->append (QString ("!!! data ready %1 bytes")
                          .arg (socket->bytesAvailable ()));
  QByteArray bytes = socket->readAll ();
  QString data (bytes);
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
  mainUi.inLog->append (QString (bytes));
  if (logIncoming) {
    logIncoming->write (bytes);
  }
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
  mainUi.outLog->append (data);
  mainUi.outLog->append (QString ("!!! wrote %1 bytes").arg (written));
  if (logOutgoing) {
    logOutgoing->write (copy);
  }
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
  } else {
    QString prefix ("PRIVMSG %1 :");
qDebug () << " prefix " << prefix << " room " << currentChan << " data " << data;
    data.prepend (prefix.arg (currentChan));
  }
  SendData (data);
}

void
IrcSock::SendScript ()
{
  scriptLines += mainUi.scriptEdit->toPlainText().split ("\n");
  mainUi.scriptEdit->clear ();
  scriptTimer->start (2000);
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
  mainUi.outLog->append (QString ("!!! did send %1 bytes").arg(bytes));
}

void
IrcSock::ConnectionReady ()
{
  pingTimer->start (2*60*1000);
  mainUi.outLog->append ("!!! Connection Ready");
  mainUi.outLog->append (QString ("!!! peer address %1")
                 .arg (socket->peerAddress().toString()));
  mainUi.peerAddressLabel->setText (socket->peerAddress().toString());
  qDebug () << " connection ready " << socket->peerAddress().toString();
  QFont font = mainUi.peerAddressLabel->font ();
  font.setStrikeOut (false);
  mainUi.peerAddressLabel->setFont (font);
}

void
IrcSock::ConnectionGone ()
{
  mainUi.outLog->append (QString ("!!! disconnected from %1")
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
  QString defLogin = Settings().value ("defaults/login", 
                     QString ("./login-script")).toString();
  Settings().setValue ("defaults/login",defLogin);
  QFile script (defLogin);
  bool ok = script.open (QFile::ReadOnly);
  if (ok) {
    while (!script.atEnd()) {
      QString line = script.readLine (1024);
      mainUi.scriptEdit->append (line);
    }
  }
}

void
IrcSock::AddChannel (const QString & chanName)
{
  IrcChannelBox * newchan  = new IrcChannelBox (chanName, this);
  channels [chanName] = newchan;
  channelGroup->AddChannel (newchan);
  channelGroup->show ();
  connect (newchan, SIGNAL (Outgoing (QString, QString)),
           this, SLOT (Outgoing (QString, QString)));
  connect (newchan, SIGNAL (Active (IrcChannelBox *)),
           this, SLOT (ChanActive (IrcChannelBox *)));
  connect (newchan, SIGNAL (InUse (IrcChannelBox *)),
           this, SLOT (ChanInUse (IrcChannelBox *)));
}

void
IrcSock::ChanActive (IrcChannelBox *chan)
{
  channelGroup->MarkActive (chan, true);
}

void
IrcSock::ChanInUse (IrcChannelBox *chan)
{
  channelGroup->MarkActive (chan, false);
}

void
IrcSock::Outgoing (QString chan, QString msg)
{
  QString trim = msg.trimmed ();
qDebug () << " outgoing " << msg << trim;
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
  if (channels.contains (chan)) {
    QString themsg = msg.trimmed();
    if (themsg.startsWith (QChar (':'))) {
      themsg.remove (0,1);
    }
    channels [chan]->Incoming (QString ("%1: %2").arg(from).arg(themsg));
  }
}


void
IrcSock::InUserMsg (const QString & from, 
                    const QString & to, 
                    const QString & msg)
{
  mainUi.inLog->append (QString ("Message from %1 to %2 says %3")
                             .arg (from)
                             .arg (to)
                             .arg (msg));
}

void
IrcSock::LogRaw (const QString & raw)
{
  mainUi.inLog->append (raw);
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
  context->currentChan = rest.split (",").at(0);
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
  Q_UNUSED (first)
  Q_UNUSED (cmd)
  QRegExp wordRx ("(\\S+)");
  int pos = wordRx.indexIn (rest,0);
  if (pos >= 0) {
    int len = wordRx.matchedLength ();
    QString chan = rest.mid (pos,len);
qDebug () << " JOIN received, rest is " << rest;
qDebug () << " chan " << chan;
    chan.remove (0,1);
    if (!context->channels.contains (chan)) {
      context->AddChannel (chan);
    }
    context->currentChan = chan;
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
      context->InUserMsg (first, dest, msg);
    }
  }
}

void
IrcSock::ReceiveNumeric (IrcSock * context,
                        const QString & first,
                        const QString & num,
                        const QString & rest)
{
  context->LogRaw (QString ("numeric %1  %2 %2").arg(first).arg(num).arg(rest));
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



} // namespace

