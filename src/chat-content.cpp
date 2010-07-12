
#include "chat-content.h"
#include <QXmppMessage.h>
#include <QXmlStreamWriter>
#include <QDomDocument>
#include <QDebug>

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

namespace egalite
{

ChatContent::ChatContent (QWidget *parent)
  :QDialog (parent)
{
  ui.setupUi (this);

  connect (ui.sendButton, SIGNAL (clicked()), this, SLOT (Send()));
  connect (ui.quitButton, SIGNAL (clicked()), this, SLOT (EndChat()));
  connect (ui.chatInput, SIGNAL (returnPressed()), this, SLOT (Send()));
}

void
ChatContent::SetRemoteName (const QString & name)
{
  remoteName = name;
}

void
ChatContent::SetLocalName (const QString & name)
{
  localName = name;
}

void
ChatContent::Incoming (const QByteArray & data)
{
  QDomDocument msgDoc;
  msgDoc.setContent (data);
  QXmppMessage msg;
  msg.parse (msgDoc.documentElement());
  GetMessage (msg);
}

void
ChatContent::GetMessage (const QXmppMessage & msg)
{
  QString from = msg.from ();
  QString to   = msg.to ();
  QString body = msg.body ();
  QString pattern ("%1 says to %2: %3");
  QString msgtext = pattern.arg(from).arg(to).arg(body);
  ui.textHistory->append (msgtext);
  qDebug () << " message from " << from << " to " << to << 
               " is " << body;
}

void
ChatContent::Send ()
{
  QString content = ui.chatInput->text();
  QXmppMessage msg (localName,remoteName,content);
  QByteArray outbuf ("<?xml version='1.0'>");
  QXmlStreamWriter out (&outbuf);
  msg.toXml (&out);
  emit Outgoing (outbuf);
  Incoming (outbuf);
  ui.chatInput->clear ();
}

void
ChatContent::EndChat ()
{
  emit Disconnect ();
}

} // namespace
