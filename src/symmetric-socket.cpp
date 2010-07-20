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
#include <QTimer>

using namespace deliberate;

namespace egalite
{

SymmetricSocket::SymmetricSocket (int socketDescriptor,
                            QSslKey argKey, QSslCertificate argCert)
  :sockDescript (socketDescriptor),
   haveDescriptor (true),
   sock (0),
   pickCert (0),
   dialog (0),
   key (argKey),
   cert (argCert),
   checkTimer (0)
{
  dialog = new QDialog;
  ui.setupUi (dialog);
  dialog->setWindowTitle (tr("Symmetric Socket"));
  checkTimer = new QTimer (this);
  connect (checkTimer, SIGNAL (timeout()), this, SLOT (TimerReport()));
  checkTimer->start (100);
  localName = cert.subjectInfo (QSslCertificate::CommonName);
  dialog->show();
}

SymmetricSocket::SymmetricSocket (QSslKey argKey, QSslCertificate argCert)
  :sockDescript (-1),
   haveDescriptor (false),
   sock (0),
   pickCert (0),
   dialog (0),
   key (argKey),
   cert (argCert),
   checkTimer (0)
{
  dialog = new QDialog;
  ui.setupUi (dialog);
  dialog->setWindowTitle (tr("Symmetric Socket"));
  checkTimer = new QTimer (this);
  connect (checkTimer, SIGNAL (timeout()), this, SLOT (TimerReport()));
  checkTimer->start (100);
  localName = cert.subjectInfo (QSslCertificate::CommonName);
#if 0
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
  if (checkTimer) {
    checkTimer->stop ();
    delete checkTimer;
    checkTimer = 0;
  }
}

void
SymmetricSocket::close ()
{
  if (checkTimer) {
    checkTimer->stop ();
  }
}

void
SymmetricSocket::Show ()
{
  if (dialog) {
    dialog->show ();
  }
}

void
SymmetricSocket::Hide ()
{
  if (dialog) {
    dialog->hide ();
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
qDebug () << " connect encrypted with " << sock;
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
qDebug () << " symmetric write " << byteArray;
  if (sock) {
    return sock->write (byteArray);
  } else {
    return -1;
  }
}

void
SymmetricSocket::Init ()
{
  qDebug () << " starting socket  " << this;

  ui.dataLine->setText ("initializing");
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
  sock->setPrivateKey (key);
  sock->setLocalCertificate (cert);
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
  connect (ui.closeButton, SIGNAL (clicked()), this, SLOT (Close ()));
}

void
SymmetricSocket::Done ()
{
  qDebug () << " server socket Done";
  if (sock) {
    sock->close ();
  }
  dialog->done(0);
  emit Exiting (this);
}

void
SymmetricSocket::Close ()
{
  if (sock) {
    sock->close ();
  }
}

void
SymmetricSocket::Disconnected ()
{
  qDebug () << " SYMMETRIC socket disconnected ";
  Done();
}

void
SymmetricSocket::EncryptDone ()
{
  qDebug () << " SYMMETRIC encrypt done " << this;
  ui.dataLine->setText (tr("Ready"));
  checkTimer->stop ();
  checkTimer->start (1000);
  emit Ready (this);
}

void
SymmetricSocket::SendData (const QByteArray &data)
{
qDebug () << "SYMMETRIC Send Data " << data;
  if (sock) {
    qint64 bytesSent = sock->write (data);
    Q_UNUSED (bytesSent);
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
  qDebug () << objectName() << " SYMMETRIC ssl BIG ERROR list: ";
  QList<QSslError>::const_iterator  erit;
  for (erit=errList.begin(); erit != errList.end(); erit++) {
    qDebug () << "ssl ERROR "<< *erit;
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
  qDebug () << "SYMMETRIC ABSTRACT socket error " << socketError;
  qDebug () << " ssl errors " << sock->sslErrors ();
qDebug () << " ABSTRACT sees peer cert " << sock->peerCertificate ();
  Disconnected ();
}

void
SymmetricSocket::SockModeChange (QSslSocket::SslMode newmode)
{
  qDebug () << " socket " << sock << " NEW MODE is now " << newmode;
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
  qDebug() << objectName() << " SERVER SymmetricSocket ssl VERIFY error " << error;
}

bool
SymmetricSocket::PickOneCert (const QList <QSslCertificate> & clist)
{
  if (pickCert == 0) {
    pickCert = new PickCert (0,QString("Incoming"));
  }
  if (pickCert) {
    bool accepted (false);
    QSslCertificate goodCert;
    pickCert->Pick (clist, accepted, goodCert);
    if (accepted) {
      remoteName = goodCert.subjectInfo (QSslCertificate::CommonName);
    } else {
      remoteName = tr("Connection Refused");
    }
    return accepted;
  } else {
    return false;
  }
}

void
SymmetricSocket::TimerReport ()
{
  ui.addressLine->setText (QString("0x")+QString::number(qulonglong (sock),16));
  if (sock) {
    ui.stateLine->setText (QString::number (int(sock->state())));
    ui.validBar->setValue (sock->isValid() ? 1 : 0);
    ui.encryptedBar->setValue (sock->isEncrypted() ? 1 : 0);
    ui.remoteAddress->setText (sock->peerAddress().toString());
    ui.localAddress->setText (sock->localAddress().toString());
  }
}


} // namespace

