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
#include "direct-caller.h"
#include "symmetric-socket.h"

using namespace deliberate;

namespace egalite {

DChatMain::DChatMain (QWidget *parent)
  :QMainWindow (parent),
   pApp (0),
   configEdit (this),
   xclient (0),
   passdial (0),
   callnum (0)
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
  directHost = Settings().value ("direct/host",directHost).toString();
  Settings().setValue ("direct/host",directHost);
  DirectListener * listen = new DirectListener (this);
  inDirect[directHost] = listen;
  listen->Init (directHost, QString("enkhuizen"));
  listen->Listen (QHostAddress ("2001:4830:1135:1::1"),29999);
  connect (listen, SIGNAL (Receive (const QByteArray &)),
           this, SLOT (GetRaw (const QByteArray&)));
  connect (listen, SIGNAL (SocketReady (SymmetricSocket *)),
           this, SLOT (IncomingDirect (SymmetricSocket *)));
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
  connect (ui.directButton, SIGNAL (clicked()), this, SLOT (CallDirect()));
  connect (ui.sendDirectButton, SIGNAL (clicked()),
           this, SLOT (SendDirect()));
  connect (ui.replyDirectButton, SIGNAL (clicked()),
           this, SLOT (ReplyDirect()));
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
    if (!xclient) {
      xclient = new QXmppClient (this);
    }
    xclient->connectToServer (server,user, password);
    connect (xclient, SIGNAL (messageReceived  (const QXmppMessage  &)),
             this, SLOT (GetMessage (const QXmppMessage &)));
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
DChatMain::GetMessage (const QXmppMessage & msg)
{
  QString from = msg.from ();
  QString to   = msg.to ();
  QString body = msg.body ();
  QString pattern ("%1 says to %2: %3");
  QString msgtext = pattern.arg(from).arg(to).arg(body);
  ui.textDisplay->append (msgtext);
  qDebug () << " message from " << from << " to " << to << 
               " is " << body;
}


void
DChatMain::GetRaw (const QByteArray &data)
{
  QDomDocument msgDoc;
  msgDoc.setContent (data);
  QXmppMessage msg;
  msg.parse (msgDoc.documentElement());
  GetMessage (msg);
qDebug () << " received raw message " << data;
}

void
DChatMain::Send ()
{
  QString body = ui.inputLine->text();
  QString to ("roteva@jtalk.berndnet");
  if (xclient) {
    xclient->sendMessage (to,body);
  }
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

void
DChatMain::CallDirect ()
{
  QString dest ("bernd.reflective-computing.com");
  callnum++;
  DirectCaller * newcall = new DirectCaller (this);
  if (newcall) {
    newcall->Setup ();
    outDirect[callnum] = newcall;
qDebug () << " start direct connect " << callnum << " call " << newcall;
    newcall->Connect (dest, callnum);
    connect (newcall, SIGNAL (Finished (int)), this, SLOT (ClearCall (int)));
    connect (newcall, SIGNAL (ConnectionReady (SymmetricSocket*)),
             this, SLOT (ConnectDirect (SymmetricSocket*)));
  }
}

void
DChatMain::ConnectDirect (SymmetricSocket * sock)
{
  if (sock) {
    QString other = sock->PeerName();
    directChats [other] = sock;
    connect (sock, SIGNAL (ReceiveData (const QByteArray &)),
             this, SLOT (GetRaw (const QByteArray &)));
  }
}

void
DChatMain::SendDirect ()
{
  SymmetricSocket *call = directChats.begin ()->second;
  QString data = ui.inputLine->text();
  QXmppMessage msg (user,call->PeerName(), data);
  
  QByteArray outbuf("<?xml version='1.0'>");
  QXmlStreamWriter out (&outbuf);
  msg.toXml (&out);
  call->SendData (outbuf);
qDebug () << " sending direct: " << outbuf;
}

void
DChatMain::ReplyDirect ()
{
  SymmetricSocket *call = directChats.begin ()->second;
  QString data = ui.inputLine->text();
  QXmppMessage msg (user,call->PeerName(), data);
  
  QByteArray outbuf("<?xml version='1.0'>");
  QXmlStreamWriter out (&outbuf);
  msg.toXml (&out);
  call->SendData (outbuf);
qDebug () << " reply direct : " << outbuf;
}

void
DChatMain::HangupDirect (int callid)
{
  std::map <int,DirectCaller*>::iterator callit;
  callit = outDirect.find (callid);
  if (callit != outDirect.end()) {
    DirectCaller * call = callit->second;
    call->Hangup ();
qDebug () << " end direct connect " << callid << " call " << call;
  }
}

void
DChatMain::ClearCall (int callid)
{
  std::map <int,DirectCaller*>::iterator callit;
  callit = outDirect.find (callid);
  if (callit != outDirect.end()) {
    outDirect.erase (callit);
  }
}


} // namespace

