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
#include <QXmppConfiguration.h>
#include <QStandardItemModel>

#include "config-edit.h"
#include "cert-store.h"

#include <map>

class QTimer;
class QApplication;
class QModelIndex;

namespace egalite 
{

class DirectListener;
class DirectCaller;
class SymmetricSocket;
class ChatBox;
class ServerContact;

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
  void Send (const QXmppMessage & msg);

private slots:

  void PassOK ();
  void PassCancel ();
  void Send ();
  void Login ();
  void CallDirect ();
  void ClearCall (int callid);
  void ConnectDirect (SymmetricSocket *direct, QString localNick);
  void StartServerChat (QString remoteName);
  void ClearDirect (SymmetricSocket *direct);
  void XmppPoll ();
  void DebugCheck ();
  void PickedItem (const QModelIndex & index );

private:

  void Connect ();
  bool GetPass ();
  void CallDirectFrom (QString nick);

  Ui_DChatMain    ui;
  QApplication   *pApp;

  QStandardItemModel  contactModel;

  ConfigEdit     configEdit;
  CertStore      certStore;

  QXmppClient   *xclient;
  QXmppConfiguration  xmppConfig;
  int           publicPort;
  QString       user;
  QString       xmppUser;
  QString       server;
  QString       password;
  QDialog      *passdial;
  int           callnum;
  QTimer       *debugTimer;
  QTimer       *xmppTimer;


  std::map <QString, DirectListener*> inDirect;
  std::map <int, DirectCaller*>   outDirect;

  std::map <QString, ChatBox *> directChats;
  std::map <QString, ChatBox *> serverChats;

  std::map <QString, ServerContact*> serverContacts;
};

} // namespace

#endif
