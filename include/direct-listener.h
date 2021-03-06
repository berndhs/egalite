#ifndef DIRECT_LISTENER_H
#define DIRECT_LISTENER_H

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

#include <QTcpServer>
#include <QList>
#include <QSslCertificate>
#include <QSslKey>
#include <QHostAddress>
#include <QByteArray>

namespace egalite 
{

class SymmetricSocket;

class DirectListener : public QTcpServer
{
  Q_OBJECT

public:

  DirectListener (QObject *parent = 0);
  ~DirectListener ();

  void Init (QString certHost, QString pass);
  void Init (QString iname, QSslKey ikey, QSslCertificate icert);
  bool Listen (const QString & thisHost, int port);
  void Close ();

  bool TakeSocket (SymmetricSocket * sock);

  QString  HostName ();
  QString  Address  ();
  int      Port ();

public slots:

  void IsReady (SymmetricSocket *sock);
  void SocketExit (SymmetricSocket *sock);

signals:

  void Receive (const QByteArray &data);

  void SocketReady (SymmetricSocket * sock, QString localName);
  void SocketClosed (SymmetricSocket *sock);

protected:

  void incomingConnection(int socketDescriptor);

private:

  typedef QList <SymmetricSocket*> SocketList;

  SocketList  sockets;

  QSslCertificate  mCert;
  QSslKey          mKey;
  QString          mName;

  QString          hostName;
  QString          hostAddress;
  int              hostPort;

} ;


} // namespace

#endif
