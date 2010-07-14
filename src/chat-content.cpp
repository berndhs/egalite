
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
  :QDialog (parent),
   chatMode (ChatModeRaw)
{
  ui.setupUi (this);

  ui.quitButton->setDefault (false);
  ui.sendButton->setDefault (true);  /// send when Return pressed
  connect (ui.sendButton, SIGNAL (clicked()), this, SLOT (Send()));
  connect (ui.quitButton, SIGNAL (clicked()), this, SLOT (EndChat()));
  ui.quitButton->setDefault (false);
}

void
ChatContent::SetMode (Mode mode)
{
  chatMode = mode;
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
  Incoming (msg);
}

void
ChatContent::Incoming (const QXmppMessage & msg)
{
  QString from = msg.from ();
  QString to   = msg.to ();
  QString body = msg.body ();
  QString pattern (tr("<b>%1</b> says to <b>%2</b>: %3"));
  QString msgtext = pattern.arg(from).arg(to).arg(body);
  ui.textHistory->append (msgtext);
qDebug () << " emit activity " << this;
  emit Activity (this);
}

void
ChatContent::Send ()
{
  QString content = ui.chatInput->text().trimmed ();
  ui.chatInput->clear ();
  if (content.length() < 1) {            /// dont send empty messages
    return;
  }
  QXmppMessage msg (localName,remoteName,content);
  if (chatMode == ChatModeRaw) {
    QByteArray outbuf ("<?xml version='1.0'>");
    QXmlStreamWriter out (&outbuf);
    msg.toXml (&out);
    emit Outgoing (outbuf);
  } else if (chatMode == ChatModeXmpp) {
    emit Outgoing (msg);
  }
  /// be optimistic and report what we just sent as being echoed back
  Incoming (msg);
}

void
ChatContent::EndChat ()
{
qDebug () << " EndChat called";
  emit Disconnect ();
}

} // namespace
