#ifndef DCHAT_H
#define DCHAT_H

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
#include "ui_dchat.h"
#include <QMainWindow>
#include <QXmppClient.h>

#include "config-edit.h"
#include "cert-store.h"

#include <map>


class QApplication;

namespace egalite 
{

class DirectListener;
class DirectCaller;
class SymmetricSocket;

class DChatMain : public QMainWindow 
{
Q_OBJECT

public:

  DChatMain (QWidget * parent = 0);

  void Init (QApplication *pap);

  void Run ();

public slots:

  void Quit ();
  void GetMessage (const QXmppMessage  & msg);
  void GetRaw (const QByteArray &data);
  void SendDirect ();
  void ReplyDirect ();

private slots:

  void PassOK ();
  void PassCancel ();
  void Send ();
  void Login ();
  void CallDirect ();
  void ClearCall (int callid);
  void ConnectDirect (SymmetricSocket *direct);

private:

  void Connect ();
  bool GetPass ();
  void CallDirectFrom (QString nick);

  Ui_DChatMain    ui;
  QApplication   *pApp;
  ConfigEdit     configEdit;
  CertStore      certStore;

  QXmppClient   *xclient;
  QString       user;
  QString       server;
  QString       password;
  QDialog      *passdial;
  int           callnum;


  std::map <QString, DirectListener*> inDirect;
  std::map <int, DirectCaller*>   outDirect;

  std::map <QString, SymmetricSocket *> directChats;

};

} // namespace

#endif
