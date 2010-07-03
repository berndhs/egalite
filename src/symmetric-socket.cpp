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
#include "symmetric-socket.h"
#include "pick-cert.h"
#include <QSslSocket>
#include <QAbstractSocket>
#include <QSslKey>
#include <QFile>
#include <QHostAddress>

using namespace deliberate;

namespace egalite
{

SymmetricSocket::SymmetricSocket (int socketDescriptor,
                            QSslKey argKey, QSslCertificate argCert)
  :sockDescript (socketDescriptor),
   haveDescriptor (true),
   sock (0),
   pickCert (0),
   started (false),
   key (argKey),
   cert (argCert)
{
  dialog = new QDialog;
  ui.setupUi (dialog);
  dialog->setWindowTitle (tr("Symmetric Socket"));
#if 0
  connect (ui.sendButton, SIGNAL (clicked()), this, SLOT (Send()));
  connect (ui.quitButton, SIGNAL (clicked()), this, SLOT (Done()));
#endif
  dialog->show();
}

SymmetricSocket::SymmetricSocket (QSslKey argKey, QSslCertificate argCert)
  :sockDescript (-1),
   haveDescriptor (false),
   sock (0),
   pickCert (0),
   started (false),
   key (argKey),
   cert (argCert)
{
  dialog = new QDialog;
  ui.setupUi (dialog);
  dialog->setWindowTitle (tr("Symmetric Socket"));
#if 0
  connect (ui.sendButton, SIGNAL (clicked()), this, SLOT (Send()));
  connect (ui.quitButton, SIGNAL (clicked()), this, SLOT (Done()));
#endif
  dialog->show();
}

SymmetricSocket::~SymmetricSocket ()
{
  if (sock) {
    delete sock;
    sock = 0;
  }
  if (pickCert) {
    delete pickCert;
    pickCert = 0;
  }
}

void
SymmetricSocket::setPeerVerifyMode (QSslSocket::PeerVerifyMode mode)
{
  if (sock) {
qDebug () << " set peer verify mode for ssl sock " << sock << " to " << mode;
    sock->setPeerVerifyMode (mode);
  }
}
  
void 
SymmetricSocket::connectToHostEncrypted ( const QString & hostName, 
                                quint16 port, 
                                const QString & sslPeerName,
                                QSslSocket::OpenMode mode)
{
  if (sock) {
    sock->connectToHostEncrypted (hostName, port, sslPeerName, mode);
  }
}

void
SymmetricSocket::disconnectFromHost ()
{
  if (sock) {
    sock->disconnectFromHost ();
  }
}

qint64
SymmetricSocket::write (const QByteArray & byteArray)
{
  if (sock) {
    return sock->write (byteArray);
  } else {
    return -1;
  }
}

void
SymmetricSocket::Init ()
{
  if (started) {
    qDebug () << " already started" ;
    return;
  }
  qDebug () << " starting socket  " << this;

  ui.dataLine->setText ("initialize socket");
  sock = new QSslSocket;
  sock->setProtocol (QSsl::AnyProtocol);
  QList <QSslCertificate> emptylist;
  sock->setCaCertificates (emptylist);
  if (sock == 0) {
    qDebug () << " cannot allocate socket ";
    emit ConnectError (QAbstractSocket::UnknownSocketError);
    return;
  }
  bool ok (true);
  if (haveDescriptor) {
    ok = sock->setSocketDescriptor (sockDescript);
  }
  if (!ok) {
    qDebug () << " error setting socket " << sock->error();
    emit ConnectError (sock->error());
    return;
  }
  qDebug () << " socket " << sock << " in thread " << thread();
  connect (sock, SIGNAL (encrypted()), this, SLOT (EncryptDone()));
  connect (sock, SIGNAL (readyRead()), this, SLOT (Receive()));
  connect (sock, SIGNAL (disconnected()), this, SLOT (Disconnected()));
  connect (sock, SIGNAL (sslErrors( const QList<QSslError>&)),
           this, SLOT (Errors (const QList<QSslError>&)));
  connect (sock, SIGNAL (peerVerifyError (const QSslError&)),
           this, SLOT (VerifyProblem (const QSslError&)));
  connect (sock, SIGNAL (modeChanged (QSslSocket::SslMode )),
           this, SLOT (SockModeChange (QSslSocket::SslMode )));
  connect (sock, SIGNAL (error ( QAbstractSocket::SocketError  )),
           this, SLOT (SockError ( QAbstractSocket::SocketError )));
}

void
SymmetricSocket::Start ()
{
  ui.otherHost->setText (sock->peerName());
  QString peerIp = sock->peerAddress().toString();
  quint16 peerPort = sock->peerPort();
  ui.otherIP->setText (peerIp + " : " + QString::number(peerPort));
  ui.dataLine->clear ();

  sock->setPrivateKey (key);
  sock->setLocalCertificate (cert);
  qDebug () << " starting server sock " << sock << " encryption while in mode " << sock->mode();
  sock->startServerEncryption ();
  qDebug () << " started  server sock " << sock << "encryption now in mode   " << sock->mode();
  started = true;
}

void
SymmetricSocket::Done ()
{
  qDebug () << " server socket Done";
  sock->close ();
  dialog->done(0);
  emit Exiting (this);
}

void
SymmetricSocket::Disconnected ()
{
  qDebug () << " server socket disconnected ";
  Done();
}

void
SymmetricSocket::EncryptDone ()
{
  qDebug () << " symmetric encrypt done " << this;
  emit Ready (this);
}

void
SymmetricSocket::SendData (const QByteArray &data)
{
  if (sock) {
    sock->write (data);
  }
}

QString
SymmetricSocket::PeerName ()
{
  if (sock) {
    return sock->peerName();
  } else {
    return (tr("anonymous"));
  }
}

void
SymmetricSocket::Receive ()
{
  QByteArray data;
  if (sock) {
    data = sock->readAll ();
  }
  emit ReceiveData (data);
}

void
SymmetricSocket::Errors (const QList<QSslError>& errList)
{
  qDebug () << objectName() << " SymmetricSocket ssl error list: ";
  QList<QSslError>::const_iterator  erit;
  for (erit=errList.begin(); erit != errList.end(); erit++) {
    qDebug () << "ssl error "<< *erit;
  }
  qDebug () << objectName()<< " SERVER sock " << sock;
  if (sock) {
    bool isok (false);
    QList <QSslCertificate>  clist = sock->peerCertificateChain();
qDebug () << " SERVER: peer is called " << sock->peerName();
qDebug () << " SERVER: peer IP " << sock->peerAddress();
qDebug () << " SERVER: peer PORT " << sock->peerPort ();
qDebug () << " SERVER: other side chain " << clist;
    QSslCertificate callerCert = sock->peerCertificate ();
qDebug () << " SERVER: other side cert " << callerCert;
    clist.append (callerCert);
qDebug () << " SERVER SymmetricSocket num certs " << clist.size();
    if (clist.size() > 0) {
      isok = PickOneCert (clist);
    }
    if (isok) {
      sock->ignoreSslErrors ();
    }
  }
}

void
SymmetricSocket::SockError ( QAbstractSocket::SocketError socketError )
{
  qDebug () << " abstract socket error " << socketError;
  qDebug () << " ssl errors " << sock->sslErrors ();
  if (socketError == QAbstractSocket::UnknownSocketError) {
    sock->ignoreSslErrors ();
  }
}

void
SymmetricSocket::SockModeChange (QSslSocket::SslMode newmode)
{
  qDebug () << " socket " << sock << " mode is now " << newmode;
}

QList<QSslCertificate>
SymmetricSocket::caCertificates () const
{
  if (sock) {
    return sock->caCertificates ();
  } else {
    QList <QSslCertificate> emptylist;
    return  emptylist;
  }
}

void
SymmetricSocket::VerifyProblem ( const QSslError & error)
{
  qDebug() << objectName() << " SERVER SymmetricSocket ssl verify error " << error;
}

bool
SymmetricSocket::PickOneCert (const QList <QSslCertificate> & clist)
{
  if (pickCert == 0) {
    pickCert = new PickCert (0,QString("Incoming"));
  }
  if (pickCert) {
    return pickCert->Pick (clist);
  } else {
    return false;
  }
}


} // namespace

