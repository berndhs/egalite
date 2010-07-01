
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
#include <QThread>
#include <QSslSocket>
#include <QSslCertificate>
#include <QSslKey>
#include "ui_mirror.h"
#include <QDialog>


namespace egalite
{

class ServerSocket : public QThread
{
  Q_OBJECT

public:

  ServerSocket (int socketDescriptor,
                QSslKey argKey, QSslCertificate argCert);
  ~ServerSocket ();

  void run ();
  void Start ();

  QList<QSslCertificate> caCertificates () const;
  QSslSocket* Socket () {
    return sock;
  }

public slots:

  void Done ();
  void Errors (const QList<QSslError>& errList);
  void VerifyProblem ( const QSslError & error);
  void SockModeChange (QSslSocket::SslMode newmode);
  void SockError ( QAbstractSocket::SocketError socketError );

private slots:

  void Send ();
  void Receive ();
  void Disconnected ();
  void EncryptDone ();

signals:

  void ConnectError (QSslSocket::SocketError socketError);
  void Exiting (ServerSocket * self);

private:

  int           sockDescript;
  QSslSocket    *sock;
  QDialog       *dialog;
  Ui_MirrorDisplay  ui;
  bool          started;
  QSslKey       key;
  QSslCertificate cert;


} ;


} // namespace

#endif
