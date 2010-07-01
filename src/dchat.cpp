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
#include "deliberate.h"
#include <QDebug>
#include "ui_getpassword.h"
#include <QXmppConfiguration.h>
#include <QXmppMessage.h>
#include <QString>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>

#include "direct-listener.h"

using namespace deliberate;

namespace egalite {

DChatMain::DChatMain (QWidget *parent)
  :QMainWindow (parent),
   pApp (0),
   configEdit (this),
   xclient (this),
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
  if (Settings().contains("sizes/main")) {
    QSize defaultSize = size();
    QSize newsize = Settings().value ("sizes/main", defaultSize).toSize();
    resize (newsize);
  }
  user = Settings().value ("network/user", user).toString();
  Settings().setValue ("network/user",user);
  server = Settings().value ("network/server",server).toString();
  Settings().setValue ("network/server",server);
  
  QString directHost ("reflect");
  DirectListener * listen = new DirectListener (this);
  inDirect[directHost] = listen;
  listen->Init (directHost, QString("enkhuizen"));
  listen->Listen (QHostAddress ("2001:4830:1135:1::1"),29999);
  show ();
}

void
DChatMain::Quit ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/main",currentSize);
  Settings().sync();
  if (pApp) {
    pApp->quit ();
  }
}

void
DChatMain::Connect ()
{
  connect (ui.quitButton, SIGNAL (clicked()), this, SLOT (Quit()));
  connect (ui.sendButton, SIGNAL (clicked()), this, SLOT (Send()));

  connect (ui.actionQuit, SIGNAL (triggered()), this, SLOT (Quit()));
  connect (ui.actionPreferences, SIGNAL (triggered()),
           &configEdit, SLOT (Exec()));
  connect (ui.actionLog_In, SIGNAL (triggered()),
           this, SLOT (Login()));
}

void
DChatMain::Login ()
{
  if (GetPass()) {
    xclient.connectToServer (server,user,
                           password);
  }
}

bool
DChatMain::GetPass ()
{
  if (passdial == 0) {
    passdial = new QDialog (this);
  }
  Ui_GetString  passui;
  passui.setupUi (passdial);
  passui.userText->setText (user);
  passui.serverText->setText (server);
  passui.passwordText->setText ("");
  connect (passui.okButton, SIGNAL (clicked()), this, SLOT (PassOK()));
  connect (passui.cancelButton, SIGNAL (clicked()), this, SLOT (PassCancel()));
  int haveit = passdial->exec ();
  if (haveit == 1) {
    password = passui.passwordText->text ();
    user = passui.userText->text ();
    server = passui.serverText->text ();
    Settings().setValue ("network/user",user);
    Settings().setValue ("network/server",server);
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
  QString body = ui.inputLine->text();
  QString to ("roteva@jtalk.berndnet");
  xclient.sendMessage (to,body);
  QXmppMessage msg (user,to,body);
  QByteArray outbuf("<?xml version='1.0'>");
  QXmlStreamWriter out (&outbuf);
  msg.toXml (&out);
  std::cout << " message 1:"  << std::endl;
  std::cout << outbuf.data() << std::endl;

  QDomDocument msgdoc;
  msgdoc.setContent (outbuf);
  std::cout << " message doc string " << msgdoc.toString().toStdString() << std::endl;
  QByteArray outb2;
  QXmlStreamWriter out2 (&outb2);
  QXmppMessage msg2;
  msg2.parse (msgdoc.documentElement());
  msg2.toXml (&out2);
  std::cout << " message 2: " << std::endl;
  std::cout << outb2.data() << std::endl;
  
}


} // namespace

