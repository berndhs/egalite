
#include "irc-socket.h"

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

#include <QByteArray>
#include <QStringList>
#include <QTimer>
#include <QHostAddress>

namespace egalite
{

int IrcSocket::sockCount (0);

IrcSocket::IrcSocket (QObject *parent)
  :QTcpSocket (parent),
   waitForName (false)
{
  sockCount++;
  setObjectName (QString("IrcSocket-%1").arg(sockCount));
  hostName = objectName ();
  pingTimer = new QTimer (this);
  scriptTimer = new QTimer (this);
  connect (pingTimer, SIGNAL (timeout()),
           this, SLOT (SendPing()));
  connect (scriptTimer, SIGNAL (timeout()), 
           this, SLOT (SendScriptHead()));
  connect (this, SIGNAL (error (QAbstractSocket::SocketError)),
           this, SLOT (SockError (QAbstractSocket::SocketError)));
  connect (this, SIGNAL (connected()),
           this, SLOT (DidConnect ()));
  connect (this, SIGNAL (disconnected ()),
           this, SLOT (DidDisconnect ()));
  connect (this, SIGNAL (readyRead ()),
           this, SLOT (Receive ()));
qDebug () << " IrcSocket " << objectName();
}

void
IrcSocket::connectToHost ( const QString & hostName, 
                          quint16 port)
{
  QTcpSocket::connectToHost (hostName, port, QTcpSocket::ReadWrite);
}

void
IrcSocket::DisconnectLater (int msecs)
{
  QTimer::singleShot (msecs, this, SLOT (Disconnect()));
}

void
IrcSocket::Disconnect ()
{
  disconnectFromHost ();
}

QString
IrcSocket::Name ()
{
  return objectName();
}

QString
IrcSocket::HostName ()
{
  return hostName;
}

void
IrcSocket::SetHostName (const QString & name)
{
  hostName = name;
}

void
IrcSocket::DidConnect ()
{
  waitForName = true;
  hostName = peerAddress().toString();
  emit connected (this);
  emit ChangedHostName (this, hostName);
  scriptTimer->start (1000);
  pingTimer->start (90*1000);
}

void
IrcSocket::DidDisconnect ()
{
  emit disconnected (this);
}

void
IrcSocket::Receive ()
{
  QByteArray bytes = readAll ();
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
      if (waitForName) {
        GetHostName (lineData);
      }
      emit ReceivedLine (this, lineData);
      lineData.clear ();
    }
  }
}

void
IrcSocket::Send (QString data)
{
  scriptLines += data;
  RollScript ();
}

void
IrcSocket::SendData (const QString & data)
{
  QString copyStr (data);
  copyStr.append ("\r\n");
  QByteArray copy = copyStr.toUtf8();
  qint64 written = write (copy);
qDebug () << " sent " << written << " bytes to socket: " << copy;
}

void
IrcSocket::RollScript ()
{
  SendScriptHead ();
  if (!scriptTimer->isActive()) {
    scriptTimer->start (1000);
  }
}

void
IrcSocket::SendScriptHead ()
{
  if (scriptLines.isEmpty()) {
    scriptTimer->stop ();
    return;
  }
  QString line = scriptLines.takeFirst();
  SendData (line);
}

void
IrcSocket::SendPing ()
{
  if (knowHostName) {
    qDebug () << "send ping goes to " << peerAddress();  
    QString msg (QString ("PING %1").arg (hostName));
    SendData (msg);
  } else {
    qDebug () << " cannot PING, unknown host name for " << peerAddress();
  }
}


void 
IrcSocket::SockError (QAbstractSocket::SocketError err)
{
  qDebug () << " socket error " << err;
  qDebug () << " socket error text " << errorString ();
}

void
IrcSocket::GetHostName (const QString & lineData)
{
qDebug () << " try to get host from " << lineData;
  QRegExp sourceRx (":(\\S+)");
  int pos = sourceRx.indexIn (lineData, 0);
  if (pos >= 0) {
    int len = sourceRx.matchedLength ();
    QString host = lineData.mid (pos+1,len-1);
    if (host.length () > 0) {
      hostName = host;
      waitForName = false;
      emit ChangedHostName (this, hostName);
qDebug () << " got host " << hostName;
    }
  }
}

} // namespace

