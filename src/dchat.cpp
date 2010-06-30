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

#include <iostream>
#include "dchat.h"
#include "delib-debug.h"
#include <QDebug>
#include "ui_getpassword.h"
#include <QXmppConfiguration.h>
#include <QXmppMessage.h>
#include <QString>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>


namespace dchat {

DChatMain::DChatMain (QWidget *parent)
  :QDialog (parent),
   pApp (0),
   xclient (this),
   user ("bernd@jtalk.berndnet"),
   server ("jtalk.berndnet"),
   password (QString("password")),
   passdial (0)
{
  ui.setupUi (this);
  Connect ();
}

void
DChatMain::Init (QApplication *pap)
{
  pApp = pap;
}

void
DChatMain::Run ()
{
  show ();
  qDebug () << "client error: " << xclient.getSocketError();
  if (GetPass()) {
    xclient.connectToServer (server,user,
                           password);
  }
}

void
DChatMain::Quit ()
{
  if (pApp) {
    pApp->quit ();
  }
}

void
DChatMain::Connect ()
{
  connect (ui.quitButton, SIGNAL (clicked()), SLOT (Quit()));
  connect (ui.sendButton, SIGNAL (clicked()), SLOT (Send()));
}

bool
DChatMain::GetPass ()
{
  if (passdial == 0) {
    passdial = new QDialog (this);
  }
  Ui_GetString  passui;
  passui.setupUi (passdial);
  passui.textEnter->setText ("");
  connect (passui.okButton, SIGNAL (clicked()), this, SLOT (PassOK()));
  connect (passui.cancelButton, SIGNAL (clicked()), this, SLOT (PassCancel()));
  int haveit = passdial->exec ();
  if (haveit == 1) {
    password = passui.textEnter->text ();
    return true;
  } else {
    return false;
  }
}

void
DChatMain::PassOK ()
{
  if (passdial) {
    passdial->done (1);
  }
}

void
DChatMain::PassCancel ()
{
  if (passdial) {
    passdial->done (0);
  }
}

void
DChatMain::Send ()
{
  QString body ("message body goes here");
  QString to ("roteva@jtalk.berndnet");
  xclient.sendMessage (to,body);
  QXmppMessage msg (user,to,body);
  QByteArray outbuf;
  QXmlStreamWriter out (&outbuf);
  msg.toXml (&out);
  std::cout << " message 1:"  << std::endl;
  std::cout << outbuf.data() << std::endl;
}


} // namespace

