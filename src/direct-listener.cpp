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

#include "direct-listener.h"
#include "server-socket.h"
#include <QFile>
#include <QSslConfiguration>

namespace egalite
{

DirectListener::DirectListener (QObject *parent)
  :QTcpServer(parent)
{}

DirectListener::~DirectListener ()
{
  SocketList::iterator sockit;
  for (sockit = sockets.begin(); sockit != sockets.end(); sockit++) {
    (*sockit)->Done();
    delete *sockit;
  }
}

void
DirectListener::Listen (const QHostAddress &addr, int port)
{
  listen (addr,port);
}

void
DirectListener::incomingConnection (int socketDescriptor)
{
  qDebug () << " egal server " << this << "new connection " << socketDescriptor;
  ServerSocket * newsock = new ServerSocket (socketDescriptor,
      key,cert) ;
  connect (newsock, SIGNAL (Exiting (ServerSocket*)),
           this, SLOT (SocketExit (ServerSocket*)));
  sockets << newsock;
  newsock->Start ();
  qDebug () << " new server side sock has certs: " << newsock->caCertificates();
//  qDebug () << " new server side sock has conf: " ;
//  ShowConfig (newsock->Socket()->sslConfiguration());
}

void
DirectListener::SocketExit (ServerSocket * goner)
{
  sockets.removeAll (goner);
}

void
DirectListener::Init (QString certHost, QString pass)
{
  QFile keyfile (QString ("/home/bernd/ssl-cert/%1/key.pem").arg(certHost));
  keyfile.open (QFile::ReadOnly);
  QSslKey skey (&keyfile,QSsl::Rsa,
                QSsl::Pem, QSsl::PrivateKey, pass.toUtf8());
  keyfile.close();
  key = skey;

  QFile certfile (QString ("/home/bernd/ssl-cert/%1/cert.pem").arg(certHost));
  certfile.open (QFile::ReadOnly);
  QSslCertificate scert (&certfile);
  certfile.close();
  cert = scert;
}


} // namespace