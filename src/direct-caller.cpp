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
  ui.setupUi (this);
  clientSock = new QSslSocket (this);
  //clientSock->setProtocol (egalite::SslProto);
  clientSock->setProtocol (QSsl::SslV3);
  clientSock->setPeerVerifyMode (QSslSocket::VerifyPeer);
  QList <QSslCertificate> emptylist;
  clientSock->setCaCertificates (emptylist);
  connect (ui.quitButton, SIGNAL (clicked()), this, SLOT (Quit()));
  connect (clientSock, SIGNAL (connected()), this, SLOT (Connected()));
  connect (clientSock, SIGNAL (hostFound()), this, SLOT (HostFound()));
  connect (clientSock, SIGNAL (disconnected ()), this, SLOT (Disconnected()));
  connect (clientSock, SIGNAL (readyRead()), this, SLOT (SockDataReady()));
  connect (clientSock, SIGNAL (encrypted()), this, SLOT (EncryptDone()));
  connect (clientSock, SIGNAL (sslErrors( const QList<QSslError>&)),
           this, SLOT (Errors (const QList<QSslError>&)));
  connect (clientSock, SIGNAL (peerVerifyError (const QSslError&)),
           this, SLOT (VerifyProblem (const QSslError&)));
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
  QString clientName ("barbados");
  clientName = Settings().value ("direct/client",clientName).toString();
  Settings().setValue ("direct/client",clientName);
  KeyInit (clientName, "enkhuizen");

  clientSock->setPrivateKey (key);
  clientSock->setLocalCertificate (cert);

  QHostAddress hostAddress = hinfo.addresses().first ();
  setWindowTitle (tr("DirectCaller Client"));
  ui.otherHost->setText (hostAddress.toString());
  qDebug () << " before connection mirror sock config " ;
  ShowConfig (clientSock->sslConfiguration());
qDebug () << " before conenction CALLER cert is " << clientSock->localCertificate();
  clientSock->connectToHostEncrypted (otherHost, 29999,
                                      hinfo.hostName(),
                                      QIODevice::ReadWrite);

  qDebug () << " clientSock isEncrypted () " << clientSock->isEncrypted();
  qDebug () << " clientSock proto " << clientSock->protocol ();
  qDebug () << " after connection mirror sock cert " << clientSock->caCertificates ();
  qDebug () << " after connection mirror sock config ";
  //ShowConfig ( clientSock->sslConfiguration());
}

void
DirectCaller::EncryptDone ()
{
  qDebug () << " EncryptDone";
  ShowConfig (clientSock->sslConfiguration());
}

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
DirectCaller::Connected ()
{
  QString name = clientSock->peerName();
  ui.otherHost->setText (name);
  QString ip = clientSock->peerAddress().toString();
  QString port = QString::number (clientSock->peerPort());
  ui.otherIP->setText (ip + ":" + port);
  ui.dataLine->setText (QString ());
}

void
DirectCaller::SockDataReady ()
{
  GetPeerMessage (clientSock);
}

void
DirectCaller::GetPeerMessage (QSslSocket *sock)
{
  bool enc = sock->isEncrypted();
#if 0
  // old raw stuff
  QString encstr (enc ? " [encrypted] " : " [clear] ");
  QByteArray message = sock->readAll();
  ui.dataLine->setText (QString(message) + encstr);
  QString otherName = sock->peerName();
  QString otherIp   = sock->peerAddress().toString ();
  QString otherPort = QString::number (sock->peerPort());
  ui.otherIP->setText (otherIp + " : " + otherPort + encstr);
  ui.otherHost->setText (otherName);
#endif
  QByteArray message = sock->readAll ();
  emit Received (message);
}

void
DirectCaller::HostFound ()
{
  qDebug () << " found host at "
            << clientSock->peerAddress().toString()
            << " is " << clientSock->peerName();
}

void
DirectCaller::Disconnected ()
{
  qDebug () << " disconnect ";
  qDebug () << " Error: " << clientSock->error();
  emit Finished (myCallid);
}

void
DirectCaller::DoSend ()
{
  QByteArray senddata = ui.dataLine->text().toUtf8();
  ui.dataLine->setText (tr("sending..."));
  clientSock->write (senddata);
}

void
DirectCaller::Send (const QByteArray & data)
{
  clientSock->write (data);
}

} // namespace

