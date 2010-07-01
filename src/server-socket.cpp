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

#include "server-socket.h"
//#include "egal-server.h"
//#include "egalite-global.h"
#include <QSslSocket>
#include <QAbstractSocket>
#include <QSslKey>
#include <QFile>
#include <QHostAddress>

namespace egalite
{

ServerSocket::ServerSocket (int socketDescriptor,
                            QSslKey argKey, QSslCertificate argCert)
  :QThread (0),
   sockDescript (socketDescriptor),
   started (false),
   key (argKey),
   cert (argCert)
{
  dialog = new QDialog;
  ui.setupUi (dialog);
  dialog->setWindowTitle (tr("Server Socket"));
  connect (ui.sendButton, SIGNAL (clicked()), this, SLOT (Send()));
  connect (ui.quitButton, SIGNAL (clicked()), this, SLOT (Done()));
  dialog->show();
}

ServerSocket::~ServerSocket ()
{
  if (sock) {
    delete sock;
  }
}

void
ServerSocket::run ()
{
  qDebug () << " socket run called " << thread();
  qDebug () << " started: " << started;
  if (!started) {
    Start ();
  }
  QByteArray message ("Hello from Server ");
  if (sock->isEncrypted()) {
    message.append (" encrypted");
  } else {
    message.append (" clear");
  }
  int nbytes(0);
  nbytes = sock->write (message);
  qDebug () << " wrote " << nbytes << " bytes";
  qDebug () << " sock isEncrypted () " << sock->isEncrypted();
  qDebug () << " sock proto " << sock->protocol();
  QThread::run ();
}

void
ServerSocket::Start ()
{
  if (started) {
    qDebug () << " already started" ;
    return;
  }
  qDebug () << " starting socket thread " << this;

  ui.dataLine->setText ("initialize socket");
  sock = new QSslSocket;
  sock->setProtocol (QSsl::AnyProtocol);
  QList <QSslCertificate> emptylist;
  sock->setCaCertificates (emptylist);
  bool certok = sock->addCaCertificates ("/home/bernd/ssl-cert/reflect/cert.pem");
  qDebug () << " adding certs ok is " << certok;
  if (sock == 0) {
    qDebug () << " cannot allocate socket ";
    emit ConnectError (QAbstractSocket::UnknownSocketError);
    return;
  }
  bool ok = sock->setSocketDescriptor (sockDescript);
  if (!ok) {
    qDebug () << " error setting socket " << sock->error();
    emit ConnectError (sock->error());
    return;
  }
  sock->setCaCertificates (QSslSocket::defaultCaCertificates());
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
  start();
}

void
ServerSocket::Done ()
{
  qDebug () << " server socket Done";
  sock->close ();
  dialog->done(0);
  emit Exiting (this);
  QThread::quit ();
}

void
ServerSocket::Disconnected ()
{
  qDebug () << " server socket disconnected ";
  Done();
}

void
ServerSocket::EncryptDone ()
{
  qDebug () << " server side encrypt done";
}

void
ServerSocket::Send ()
{
  QString data = ui.dataLine->text ();
  sock->write (data.toUtf8());
}

void
ServerSocket::Receive ()
{
  QByteArray data = sock->readAll ();
  bool enc = sock->isEncrypted();
  QString encstr (enc ? " [encrypted] " : " [clear] ");
  ui.dataLine->setText (data + encstr);
}

void
ServerSocket::Errors (const QList<QSslError>& errList)
{
  qDebug () << objectName() << " ServerSocket ssl error list: ";
  QList<QSslError>::const_iterator  erit;
  for (erit=errList.begin(); erit != errList.end(); erit++) {
    qDebug () << "ssl error "<< *erit;
  }
}

void
ServerSocket::SockError ( QAbstractSocket::SocketError socketError )
{
  qDebug () << " abstract socket error " << socketError;
  qDebug () << " ssl errors " << sock->sslErrors ();
  if (socketError == QAbstractSocket::UnknownSocketError) {
    sock->ignoreSslErrors ();
  }
}

void
ServerSocket::VerifyProblem ( const QSslError & error)
{
  qDebug() << objectName() << "ServerSocket ssl verify error " << error;
}

void
ServerSocket::SockModeChange (QSslSocket::SslMode newmode)
{
  qDebug () << " socket " << sock << " mode is now " << newmode;
}

QList<QSslCertificate>
ServerSocket::caCertificates () const
{
  if (sock) {
    return sock->caCertificates ();
  } else {
    QList <QSslCertificate> emptylist;
    return  emptylist;
  }
}


} // namespace

