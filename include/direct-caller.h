#ifndef MIRROR_H
#define MIRROR_H


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

#include <QSslError>
#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QSslCertificate>
#include <QSslKey>
#include "cert-store.h"

class QApplication;

namespace egalite
{

class PickCert;
class SymmetricSocket;

/** \brief Establish a new SSL connection to someone listening.
  *
  * The caller presents an SSL certificate to the remote side, and
  * talks to the symmetric socket.
  */

class DirectCaller : public QObject
{

  Q_OBJECT

public:

  DirectCaller (QWidget *parent = 0);

  void Setup (CertRecord & certRec, int remotePort, QString localNick);
  void Connect (QString otherHost, QString name, int callid);
  void ConnectAddress (QString addr, QString name, int callid);
  void Hangup ();
  QString Party (); /// who is on the other side
  QString Local (); /// who is on this side

public slots:


private slots:

  void Quit ();

  void EncryptDone (SymmetricSocket *sock);

signals:

  void Finished (int myid);
  void ConnectionReady (SymmetricSocket * sock, QString localNick);


private:

  void ShowCertInfo (const QSslCertificate & cert);

  QWidget          *parentWidget;

  SymmetricSocket  *clientSock;
  PickCert         *pickCert;
  int               myCallid;
  QString           party;
  QString           localName;
  QSslCertificate   cert;
  QSslKey           key;
  int               publicPort;


};

} // namespace

#endif
