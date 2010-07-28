
#include "deliberate.h"
#include "chat-content.h"
#include <QXmppMessage.h>
#include <QXmlStreamWriter>
#include <QDomDocument>
#include <QRegExp>
#include <QDesktopServices>
#include <QDebug>
#include "link-mangle.h"

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

#include <QFileDialog>

namespace egalite
{

ChatContent::ChatContent (QWidget *parent)
  :QDialog (parent),
   chatMode (ChatModeRaw),
   dateMask ("yy-MM-dd hh:mm:ss")
{
  ui.setupUi (this);

  connect (ui.quitButton, SIGNAL (clicked()), this, SLOT (EndChat()));
  connect (ui.saveButton, SIGNAL (clicked()), this, SLOT (SaveContent()));
  connect (ui.sendButton, SIGNAL (clicked()), this, SLOT (Send()));
  connect (ui.textHistory, SIGNAL (anchorClicked (const QUrl&)),
          this, SLOT (HandleAnchor (const QUrl&)));
  ui.quitButton->setDefault (false);
  ui.saveButton->setDefault (false);
  ui.quitButton->setAutoDefault (false);
  ui.saveButton->setAutoDefault (false);
  ui.sendButton->setDefault (true);  /// send when Return pressed
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
ChatContent::Start ()
{
  dateMask = deliberate::Settings().value ("style/dateformat",dateMask)
                                   .toString();
  deliberate::Settings().setValue ("style/dateformat",dateMask);
}

void
ChatContent::Start (Mode mode,
              const QString & remoteName,
              const QString & localName)
{
  SetMode (mode);
  SetRemoteName (remoteName);
  SetLocalName (localName);
  Start ();
}


void
ChatContent::SaveContent ()
{
  QString filename = QFileDialog::getSaveFileName (this, 
                      tr ("Save Chat Content"));
  if (filename.length () > 0) {
    QFile file (filename);
    file.open (QFile::WriteOnly);
    file.write (ui.textHistory->toPlainText ().toUtf8());
    file.close ();
  }
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
  QString body = msg.body ().trimmed();
  if (body.length() < 1) {
    return;
  }
  QDateTime  now = QDateTime::currentDateTime();
  QString pattern (tr("(%3) <b>%1</b>: %2"));
  QString msgtext = pattern.arg(from).arg(body)
                           .arg (now.toString (dateMask));
  QString cookedText = LinkMangle::Anchorize (msgtext,
                                   LinkMangle::HttpExp (),
                                   LinkMangle::HttpAnchor);
  ui.textHistory->append (cookedText);
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
  emit Disconnect (remoteName);
}

void
ChatContent::HandleAnchor (const QUrl & url)
{
  QDesktopServices::openUrl (url);
}


} // namespace
