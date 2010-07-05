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

using namespace deliberate;

namespace egalite
{

void
ShowConfig (const QSslConfiguration & conf)
{
  qDebug () << " SSL configuration: ";
  qDebug () << " Certificates: " << conf.caCertificates();
// qDebug () << " Ciphers: " << conf.ciphers();
  qDebug () << " NULL: " << conf.isNull();
  qDebug () << " local Cert: " << conf.localCertificate();
  qDebug () << " peer Cert: " << conf.peerCertificate ();
  qDebug () << " peer Cert Chain: " << conf.peerCertificateChain ();
  qDebug () << " peer verify depnth: " << conf.peerVerifyDepth ();
  qDebug () << " peer verfiy mode: " << conf.peerVerifyMode ();
  qDebug () << " private key: " << conf.privateKey ();
  qDebug () << " session cipher: " << conf.sessionCipher ();
}

DirectCaller::DirectCaller (QWidget *parent)
  :QDialog (parent),
   pickCert (0)
{
}


void
DirectCaller::Setup (CertRecord & certRec)
{
  ui.setupUi (this);
  QString clientName ("barbados");
  clientName = Settings().value ("direct/client",clientName).toString();
  Settings().setValue ("direct/client",clientName);
 
  QString pass ("enkhuizen");
  key = QSslKey (certRec.Key().toAscii(),QSsl::Rsa,
                QSsl::Pem, QSsl::PrivateKey, pass.toUtf8());
  cert = QSslCertificate (certRec.Cert().toAscii());
  clientSock = new SymmetricSocket (key, cert);
  clientSock->Init ();
qDebug () << " did sock Init() for " << clientSock;
  clientSock->setPeerVerifyMode (QSslSocket::VerifyPeer);
  connect (clientSock, SIGNAL (Ready (SymmetricSocket*)),
           this, SLOT (EncryptDone (SymmetricSocket*)));
  connect (ui.quitButton, SIGNAL (clicked()), this, SLOT (Quit()));
#if 0
  connect (clientSock, SIGNAL (connected()), this, SLOT (Connected()));
  connect (clientSock, SIGNAL (hostFound()), this, SLOT (HostFound()));
  connect (clientSock, SIGNAL (disconnected ()), this, SLOT (Disconnected()));
  connect (clientSock, SIGNAL (readyRead()), this, SLOT (SockDataReady()));
  connect (clientSock, SIGNAL (encrypted()), this, SLOT (EncryptDone()));
  connect (clientSock, SIGNAL (sslErrors( const QList<QSslError>&)),
           this, SLOT (Errors (const QList<QSslError>&)));
  connect (clientSock, SIGNAL (peerVerifyError (const QSslError&)),
           this, SLOT (VerifyProblem (const QSslError&)));
#endif
  connect (ui.sendButton, SIGNAL (clicked()), this, SLOT (DoSend()));
  show ();
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

void
DirectCaller::KeyInit (QString certHost, QString pass)
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

void
DirectCaller::Connect (QString otherHost, int callid)
{
  myCallid = callid;
  party = otherHost;
  QHostInfo hinfo = QHostInfo::fromName (otherHost);
  if (hinfo.addresses().isEmpty()) {
    exit (1);
  }
  QHostAddress hostAddress = hinfo.addresses().first ();
  setWindowTitle (tr("DirectCaller Client"));
  ui.otherHost->setText (hostAddress.toString());
//  qDebug () << " before connection mirror sock config " ;
//  ShowConfig (clientSock->sslConfiguration());
qDebug () << " before connectToHost, have local cert " << clientSock->Socket()->localCertificate();
  clientSock->connectToHostEncrypted (otherHost, 29999,
                                      hinfo.hostName(),
                                      QSslSocket::ReadWrite);

}

void
DirectCaller::EncryptDone (SymmetricSocket *sock)
{
  qDebug () << " Caller emit ConnectionReady " << sock;
  emit ConnectionReady (sock);
}

#if 0
void
DirectCaller::Errors (const QList<QSslError>& errList)
{
  qDebug () << objectName() <<  " CALLER DirectCaller ssl error list: ";
  QList<QSslError>::const_iterator  erit;
  for (erit=errList.begin(); erit != errList.end(); erit++) {
    qDebug () << "ssl error "<< *erit;
  }
}

void
DirectCaller::VerifyProblem ( const QSslError & error)
{
  qDebug() << objectName() << " CALLER DirectCaller ssl verify error " << error;
  qDebug () << " CALLER client socket " << clientSock;
  if (clientSock) {
    bool isok (false);
    QList <QSslCertificate>  clist = clientSock->peerCertificateChain();
    if (clist.size() > 0) {
      isok = PickOneCert (clist);
    }
    if (isok) {
      clientSock->ignoreSslErrors ();
    }
  }
}

#endif

bool
DirectCaller::PickOneCert (const QList <QSslCertificate> & clist)
{
  if (pickCert == 0) {
    pickCert = new PickCert (this, QString ("Outgoing"));
  }
  if (pickCert) {
    return pickCert->Pick (clist);
  } else {
    return false;
  }
}

void
DirectCaller::Hangup ()
{
  if (clientSock) {
    clientSock->disconnectFromHost ();
    hide (); 
  }
}

void
DirectCaller::Send (const QByteArray & data)
{
qDebug () << " direct writing " << data;
  clientSock->write (data);
}

} // namespace

