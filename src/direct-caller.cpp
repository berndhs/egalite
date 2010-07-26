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

#include "deliberate.h"
#include "direct-caller.h"
#include "pick-cert.h"
#include "simple-pass.h"
#include "symmetric-socket.h"
#include <QDebug>
#include <QApplication>
#include <QHostAddress>
#include <QHostInfo>
#include <QTcpServer>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslCipher>
#include <QSsl>
#include <QFile>


namespace egalite
{

DirectCaller::DirectCaller (QWidget *parent)
  :QObject (parent),
   parentWidget (parent),
   pickCert (0)
{
}


void
DirectCaller::Setup (CertRecord & certRec, int remotePort, QString localNick)
{
  localName = localNick;
  publicPort = remotePort;
  QString pass = certRec.Password ();
  if (pass.length() == 0) {
    SimplePass  getPass (parentWidget);
    pass = getPass.GetPassword (tr("Certificate Password:"));
  }
  key = QSslKey (certRec.Key().toAscii(),QSsl::Rsa,
                QSsl::Pem, QSsl::PrivateKey, pass.toUtf8());
  cert = QSslCertificate (certRec.Cert().toAscii());
  clientSock = new SymmetricSocket (key, cert);
  clientSock->Init ();
qDebug () << " did sock Init() for " << clientSock;
  clientSock->setPeerVerifyMode (QSslSocket::VerifyPeer);
  connect (clientSock, SIGNAL (Ready (SymmetricSocket*)),
           this, SLOT (EncryptDone (SymmetricSocket*)));
}

void
DirectCaller::Quit ()
{
  Hangup ();
  
}

QString
DirectCaller::Party ()
{
  return party;
}

QString
DirectCaller::Local ()
{
  return localName;
}


void
DirectCaller::Connect (QString otherHost, QString name, int callid)
{
  myCallid = callid;
  party = otherHost;
  QString addrString;
  if (deliberate::IsIp6Address (otherHost) 
     || deliberate::IsIp4Address (otherHost)) {
    addrString = otherHost;
  } else {
    QHostInfo hinfo = QHostInfo::fromName (otherHost);
    QHostAddress hostAddress = hinfo.addresses().first ();
    addrString  = hostAddress.toString();
  }
qDebug () << " before connectToHost, have local cert " << clientSock->Socket()->localCertificate();
  clientSock->connectToHostEncrypted (addrString, publicPort,
                                      name,
                                      QSslSocket::ReadWrite);

}

void
DirectCaller::ConnectAddress (QString addr, QString name, int callid)
{
  myCallid = callid;
  party = addr;
  clientSock->connectToHostEncrypted (addr, publicPort,
                                      name,
                                      QSslSocket::ReadWrite);
}

void
DirectCaller::EncryptDone (SymmetricSocket *sock)
{
  qDebug () << " Caller emit ConnectionReady " << sock;
  emit ConnectionReady (sock, localName);
}


void
DirectCaller::Hangup ()
{
  if (clientSock) {
    clientSock->disconnectFromHost ();
  }
}

} // namespace

