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

#include <QSslSocket>
#include <QSslError>
#include <QDialog>
#include <QTimer>

#include "ui_mirror.h"

class QApplication;

namespace egalite
{

class PickCert;

class DirectCaller : public QDialog
{

  Q_OBJECT

public:

  DirectCaller (QWidget *parent = 0);

  void Connect (QString otherHost, int callid);
  void Hangup ();

private slots:

  void Quit ();

  void EncryptDone ();
  void Errors (const QList<QSslError>& errList);
  void VerifyProblem ( const QSslError & error);
  void Connected ();
  void HostFound ();
  void Disconnected ();
  void SockDataReady ();
  void DoSend ();

signals:

  void Finished (int myid);


private:

  void GetPeerMessage (QSslSocket *sock);
  void ShowCertInfo (const QSslCertificate & cert);
  bool PickOneCert (const QList <QSslCertificate> & clist);

  Ui_MirrorDisplay  ui;

  QSslSocket  *clientSock;
  PickCert    *pickCert;
  int         myCallid;

};

} // namespace

#endif
