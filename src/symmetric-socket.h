
#ifndef SERVER_SOCKET_H
#define SERVER_SOCKET_H

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

#include <QSslSocket>
#include <QSslCertificate>
#include <QSslKey>
#include "ui_mirror.h"
#include <QDialog>
#include <QObject>


class QTimer;

namespace egalite
{

class PickCert;

class SymmetricSocket : public QObject
{
  Q_OBJECT

public:

  SymmetricSocket (int socketDescriptor,
                   QSslKey argKey, 
                   QSslCertificate argCert);
  SymmetricSocket (QSslKey argKey, QSslCertificate argCert);
  ~SymmetricSocket ();

  void Start ();
  void Init ();
  QString PeerName ();

  void setPeerVerifyMode ( QSslSocket::PeerVerifyMode mode );
  void connectToHostEncrypted ( const QString & hostName, 
                                quint16 port, 
                                const QString & sslPeerName,
                                QSslSocket::OpenMode mode = QSslSocket::ReadWrite );
  void disconnectFromHost ();

  qint64 write ( const QByteArray & byteArray );

  QList<QSslCertificate> caCertificates () const;
  QSslSocket* Socket () {
    return sock;
  }

public slots:

  void Done ();
  void Close ();
  void Errors (const QList<QSslError>& errList);
  void VerifyProblem ( const QSslError & error);
  void SockModeChange (QSslSocket::SslMode newmode);
  void SockError ( QAbstractSocket::SocketError socketError );

  void SendData (const QByteArray &data);
  void TimerReport ();

private slots:

  void Receive ();
  void Disconnected ();
  void EncryptDone ();

signals:

  void ConnectError (QSslSocket::SocketError socketError);
  void Exiting (SymmetricSocket * self);
  void ReceiveData (const QByteArray &data);
  void Ready (SymmetricSocket * self);

private:

  bool PickOneCert (const QList <QSslCertificate> & clist);

  int           sockDescript;
  bool          haveDescriptor;
  QSslSocket    *sock;
  PickCert      *pickCert;
  QDialog       *dialog;
  Ui_MirrorDisplay  ui;
  QSslKey       key;
  QSslCertificate cert;
  QTimer       *checkTimer;


} ;


} // namespace

#endif
