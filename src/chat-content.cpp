
#include "deliberate.h"
#include "chat-content.h"
#include <QXmppMessage.h>
#include <QXmlStreamWriter>
#include <QDomDocument>
#include <QDomElement>
#include <QDomText>
#include <QRegExp>
#include <QDesktopServices>
#include <QTimer>
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
   rcvCount (0),
   sendCount (0),
   protoVersion (QString()),
   heartPeriod (0),
   heartBeat (0),
   dateMask ("yy-MM-dd hh:mm:ss"),
   chatLine (tr("(%1) <b style=\"font-size:small; "
                 "color:@color@;\">%2</b>: %3")),
   localHtmlColor ("blue"),
   remoteHtmlColor ("red")
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
  Qt::WindowFlags flags = windowFlags ();
  flags |= (Qt::WindowMinimizeButtonHint | Qt::WindowSystemMenuHint);
  setWindowFlags (flags);
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
                                   .toString ();
  deliberate::Settings().setValue ("style/dateformat",dateMask);
  chatLine = deliberate::Settings().value ("style/chatline",chatLine)
                                   .toString ();
  deliberate::Settings().setValue ("style/chatline",chatLine);
  localHtmlColor = deliberate::Settings().value ("style/localColor",
                                    localHtmlColor)
                                   .toString ();
  deliberate::Settings().setValue ("style/localColor",localHtmlColor);
  remoteHtmlColor = deliberate::Settings().value ("style/remoteColor",
                                    remoteHtmlColor)
                                   .toString ();
  deliberate::Settings().setValue ("style/remoteColor",remoteHtmlColor);
  localLine = chatLine;
  localLine.replace (QString("@color@"),localHtmlColor);
  remoteLine = chatLine;
  remoteLine.replace (QString("@color@"),remoteHtmlColor);
  rcvCount = 0;
  sendCount = 0;
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
  if (chatMode == ChatModeRaw) {
    ModeUpdate ();
  }
}

void
ChatContent::SetHeartbeat (int secs)
{
  if (heartBeat == 0) {
    heartBeat = new QTimer (this);
    connect (heartBeat, SIGNAL (timeout()), this, SLOT (Heartbeat()));
  }
  if (secs <= 0) {
    heartBeat->stop ();
  } else {
    heartBeat->start (secs * 1000);
  }
}


void
ChatContent::SaveContent ()
{
  QString defaultFilePat ("%1-%2.log");
  QStringList  parts = remoteName.split (QRegExp("[ @/]"));
  QDateTime  now = QDateTime::currentDateTime();
  QString defaultName = defaultFilePat
                          .arg (parts.at(0))
                          .arg (now.toString(tr("yy-MM-dd-hhmmss")));
  QString filename = QFileDialog::getSaveFileName (this, 
                      tr ("Save Chat Content"),
                      defaultName);
  if (filename.length () > 0) {
    QFile file (filename);
    file.open (QFile::WriteOnly);
    file.write (ui.textHistory->toPlainText ().toUtf8());
    file.close ();
  }
}

void
ChatContent::IncomingDirect (const QByteArray & data, bool isLocal)
{
  QDomDocument doc;
  doc.setContent (data);
qDebug () << "Incoming Raw " << data;
  QDomElement root = doc.documentElement();
  if (root.tagName() == "message") { 
    QXmppMessage msg;
    msg.parse (root);
    SetProtoVersion ("0.1");
    IncomingXmpp (msg, isLocal);
  } else if (root.tagName() == "Egalite") {
    SetProtoVersion ("0.2");
    chatMode = ChatModeEmbed;
    ExtractXmpp (root, isLocal); 
  } else {
    qDebug () << " invalid tag " << root.tagName();
  }
}

void
ChatContent::SetProtoVersion (QString newProto)
{
  if (protoVersion != newProto) {
    protoVersion = newProto;
    emit ChangeProto (this, newProto);
  }
}

void
ChatContent::IncomingXmpp (const QXmppMessage & msg, bool isLocal)
{
  rcvCount ++;
qDebug () << "Receive Count == " << rcvCount << " mode " << chatMode;
  QString from = msg.from ();
  QString to   = msg.to ();
  QString body = msg.body ().trimmed();
  if (body.length() < 1) {
    if (rcvCount == 1 && chatMode == ChatModeRaw) {
      /// empty first message -- other end wants Embed mode
      SendMessage (QString (), true);
      chatMode = ChatModeEmbed;
      qDebug () << " Switch to Mode " << chatMode;
      ChangeProto (this, "0.2");
    }
    return;
  }
  QDateTime  now = QDateTime::currentDateTime();
  QString msgtext = (isLocal ? localLine : remoteLine)
                           .arg (now.toString (dateMask))
                           .arg (from)
                           .arg (body)
                            ;
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
  SendMessage (content, false);
}

void
ChatContent::ModeUpdate ()
{
  SendMessage (QString(),true);
}

void
ChatContent::SendMessage (const QString & content, bool isControl)
{
  if (chatMode == ChatModeRaw &&
      sendCount == 0 &&
      !isControl) { // try to go to Embed mode
    ModeUpdate ();
  }
  QXmppMessage msg (localName,remoteName,content);
  if (chatMode == ChatModeRaw || chatMode == ChatModeEmbed) {
    QByteArray outbuf ("<?xml version='1.0'>");
    QXmlStreamWriter out (&outbuf);
    msg.toXml (&out);
    if (chatMode == ChatModeEmbed) {
      EmbedDirectMessage (outbuf);
      SetProtoVersion ("0.2");
    }
    sendCount++;
    emit Outgoing (outbuf);
  } else if (chatMode == ChatModeXmpp) {
    sendCount++;
    emit Outgoing (msg);
  }
  /// be optimistic and report what we just sent as being echoed back
  if (!isControl) {
    IncomingXmpp (msg, true);
  }
}

void
ChatContent::Heartbeat ()
{
  if (chatMode == ChatModeEmbed) {
    QDomDocument heartDoc ("Egalite");
    QDomElement root = heartDoc.createElement ("Egalite");
    root.setAttribute ("version",protoVersion); 
    heartDoc.appendChild (root);
    QDomElement msg = heartDoc.createElement ("cmd");
    msg.setAttribute ("op","ctl");
    msg.setAttribute ("subop","heartbeat");
    root.appendChild (msg);
    QByteArray outbuf = heartDoc.toString().toUtf8();
    emit Outgoing (outbuf);
qDebug () << "SEND Heartbeat " << outbuf;
  }
}

void
ChatContent::EmbedDirectMessage (QByteArray & raw)
{
  qDebug () << " sending " << raw;
  QDomDocument directDoc ("Egalite");
  QDomElement root = directDoc.createElement ("Egalite");
  root.setAttribute ("version",protoVersion);
  directDoc.appendChild (root);
  QDomElement msg = directDoc.createElement ("cmd");
  msg.setAttribute ("op","xmpp");
  msg.setAttribute ("subop","msg");
  msg.setAttribute ("num",QString::number(sendCount));
  root.appendChild (msg);
  QDomText txt = directDoc.createTextNode (raw);
  msg.appendChild (txt);
  raw = directDoc.toString().toUtf8();
}

void
ChatContent::ExtractXmpp (QDomElement & msg, bool isLocal)
{
  QDomElement body = msg.firstChildElement ();
  QString opcode = body.attribute ("op",QString("badop"));
qDebug () << " incoming message op " << opcode;
  if (opcode == "xmpp") {
    QByteArray msgtext = body.text ().toUtf8();
qDebug () << " encapsulated xmpp message is " << msgtext;
    QDomDocument qxmppDoc;
    qxmppDoc.setContent (msgtext);
    QDomElement qxmppRoot = qxmppDoc.documentElement();
    QXmppMessage msg;
    msg.parse (qxmppRoot);
    IncomingXmpp (msg, isLocal);
  } else if (opcode == "ctl") {
    qDebug () << "egalite ctl message received: " 
              << msg.attribute ("op") 
              << msg.attribute("subop");
  }
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
