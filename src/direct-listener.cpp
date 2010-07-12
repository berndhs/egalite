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
#include "symmetric-socket.h"
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
  qDebug () << " Direct Listener " << this << "new connection " << socketDescriptor;
  qDebug () << " Listener adding mCert " << mCert;
  SymmetricSocket * newsock = new SymmetricSocket (socketDescriptor,
      mKey,mCert) ;
  connect (newsock, SIGNAL (Exiting (SymmetricSocket*)),
           this, SLOT (SocketExit (SymmetricSocket*)));
  connect (newsock, SIGNAL (Ready (SymmetricSocket*)),
           this, SLOT (IsReady (SymmetricSocket*)));
  newsock->Init ();
  newsock->Socket()->setPeerVerifyMode (QSslSocket::VerifyPeer);
  newsock->Socket()->startServerEncryption ();
  sockets << newsock;
  qDebug () << " new server side sock has certs: " << newsock->caCertificates();
//  qDebug () << " new server side sock has conf: " ;
//  ShowConfig (newsock->Socket()->sslConfiguration());
}

void
DirectListener::Init (QString certHost, QString pass)
{
  QFile keyfile (QString ("/home/bernd/ssl-mCert/%1/mKey.pem").arg(certHost));
  keyfile.open (QFile::ReadOnly);
  QSslKey skey (&keyfile,QSsl::Rsa,
                QSsl::Pem, QSsl::PrivateKey, pass.toUtf8());
  keyfile.close();
  mKey = skey;

  QFile certfile (QString ("/home/bernd/ssl-mCert/%1/mCert.pem").arg(certHost));
  certfile.open (QFile::ReadOnly);
  QSslCertificate scert (&certfile);
  certfile.close();
  mCert = scert;
}

void
DirectListener::Init (QString iname, QSslKey ikey, QSslCertificate icert)
{
  mName = iname;
  mKey = ikey;
  mCert = icert;
}

void
DirectListener::IsReady (SymmetricSocket * sock)
{
  qDebug () << " Listener socket ready " << sock;
  emit SocketReady (sock, mName);
}

bool
DirectListener::TakeSocket (SymmetricSocket * sock)
{
  if (sockets.contains (sock)) {
    sockets.removeAll (sock);
    return true;
  }
  return false;
}


void
DirectListener::SocketExit (SymmetricSocket *sock)
{
  emit SocketClosed (sock);
}


} // namespace
