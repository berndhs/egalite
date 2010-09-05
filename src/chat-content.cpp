
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
#include <QMessageBox>
#include <QUuid>
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
   sendFileWindow (1),
   sendChunkSize (2048),
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
  connect (ui.sendFileButton, SIGNAL (clicked()), 
          this, SLOT (StartFileSend ()));
  connect (ui.textHistory, SIGNAL (anchorClicked (const QUrl&)),
          this, SLOT (HandleAnchor (const QUrl&)));
  ui.quitButton->setDefault (false);
  ui.saveButton->setDefault (false);
  ui.sendFileButton->setDefault (false);
  ui.quitButton->setAutoDefault (false);
  ui.saveButton->setAutoDefault (false);
  ui.sendFileButton->setAutoDefault (false);
  ui.sendFileButton->hide();
  ui.sendButton->setDefault (true);  /// send when Return pressed
  Qt::WindowFlags flags = windowFlags ();
  flags |= (Qt::WindowMinimizeButtonHint | Qt::WindowSystemMenuHint);
  setWindowFlags (flags);
}


ChatContent::~ChatContent ()
{
  Stop ();
}

void
ChatContent::Stop ()
{
  qDebug () << " Chat Content close " << this;
  if (heartBeat) {
    heartBeat->stop ();
  }
}

bool
ChatContent::close ()
{
  Stop ();
  return QDialog::close ();
}

void
ChatContent::SetMode (Mode mode)
{
  chatMode = mode;
  if (chatMode == ChatModeEmbed) {
    ui.sendFileButton->show ();
  } else {
    ui.sendFileButton->hide ();
  }
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
  QDomElement root = doc.documentElement();
qDebug () << "INcoming Tag " << root.tagName();
  if (root.tagName() == "message") { 
    QXmppMessage msg;
    msg.parse (root);
    SetProtoVersion ("0.1");
    IncomingXmpp (msg, isLocal);
  } else if (root.tagName() == "Egalite") {
    SetProtoVersion ("0.2");
    SetMode (ChatModeEmbed);
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
      SetMode (ChatModeEmbed);
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
  } else if (opcode == "sendfile") {
    qDebug () << " send file protocol message ";
    ReceiveSendfileProto (body);
  } else if (opcode == "ctl") {
    qDebug () << "egalite ctl message received: " 
              << msg.attribute ("op") 
              << msg.attribute("subop");
  }
}

void
ChatContent::ReceiveSendfileProto (QDomElement & msg)
{
  QString subop;
  subop = msg.attribute ("subop");
  QString id;
  id = msg.attribute ("xferid");
qDebug () << " ReceiveSendfileProto subop " << subop << "  xferid " << id;
  if (subop == "goahead") { 
    SendNextPart (id);
  } else if (subop == "deny") {
    SendfileDeny (msg);
  } else if (subop == "chunk-ack") {
    SendfileChunkAck (msg);
  } else if (subop == "chunk-data") {
    SendfileChunkData (msg);
  } else if (subop == "sendreq") {
    SendfileSendReq (msg);
  } else if (subop == "rcv-done") {
    CloseTransfer (id, true);
  } else if (subop == "snd-done") {
    CloseTransfer (id, true);
  } else if (subop == "abort") {
    SendfileAbort (msg);
  } else {
    qDebug () << " Unknown subop-code" << subop;
  }
}

void
ChatContent::EndChat ()
{
qDebug () << " EndChat called";
  emit Disconnect (remoteName);
  Stop ();
}

void
ChatContent::HandleAnchor (const QUrl & url)
{
  QDesktopServices::openUrl (url);
}

void
ChatContent::StartFileSend ()
{
  if (chatMode != ChatModeEmbed) {
    return;
  }
  QString defaultName;
  QString filename = QFileDialog::getOpenFileName (this, 
                      tr ("Select File to Save"),
                      defaultName);
  if (filename.length () > 0) {
    qDebug () << " selected file name " << filename;
    XferInfo  info;
    info.id = QUuid::createUuid ().toString();
    info.fileSize = 0;
    info.lastChunk = 0;
    info.lastChunkAck  = 0;
    QFile * fp =  new QFile (filename);
    bool isopen = fp ->open (QFile::ReadOnly);
    info.fileSize = fp->size ();
    if (isopen) {
      xferFile [info.id] = fp;
      xferState [info.id] = info;
      SendFirstPart (info.id);
    } else {
      delete fp;
      QMessageBox cantOpen (this);
      cantOpen.setText (tr("Cannot open file %1 to send").arg(filename));
      QTimer::singleShot (15000, &cantOpen, SLOT (reject()));
      cantOpen.exec ();
    }
  }
}

void
ChatContent::SendFirstPart (const QString & id)
{
  qDebug () << " supposed to send next part for " << id;
  QFile * fp = xferFile[id];
  if (!fp) {
    return;
  }
  XferInfo & info = xferState[id];
  QDomDocument requestDoc ("Egalite");
  QDomElement  request = requestDoc.createElement ("Egalite");
  request.setAttribute ("version", protoVersion);
  requestDoc.appendChild (request);
  QDomElement cmd = requestDoc.createElement ("cmd");
  cmd.setAttribute ("op","sendfile");
  cmd.setAttribute ("subop","sendreq");
  cmd.setAttribute ("xferid",id);
  cmd.setAttribute ("name",fp->fileName());
  cmd.setAttribute ("size",info.fileSize);
  request.appendChild (cmd);
  QByteArray msgbytes = requestDoc.toString().toUtf8 ();
  emit Outgoing (msgbytes);
}
  
void
ChatContent::SendNextPart (const QString & id)
{
  XferStateMap::iterator  stateIt = xferState.find(id);
  XferFileMap::iterator fileIt    = xferFile.find(id);
  if (stateIt != xferState.end() && fileIt != xferFile.end()) {
    QFile * fp =  fileIt->second;
    XferInfo & info = stateIt->second;
    if (fp) {
      for (int ch=0; ch<sendFileWindow; ch++) {
        QByteArray data = fp->read (sendChunkSize);
        if (data.size() < 1) {
          SendFinished (id);
        } else {
          SendChunk (info, data);
        }
      }
    }
  }
}

void
ChatContent::SendFinished (const QString & id)
{
  CloseTransfer (id, true);
  QDomDocument chunkDoc ("Egalite");
  QDomElement  chunkRoot = chunkDoc.createElement ("Egalite");
  chunkRoot.setAttribute ("version", protoVersion);
  chunkDoc.appendChild (chunkRoot);
  QDomElement cmd = chunkDoc.createElement ("cmd");
  cmd.setAttribute ("op","sendfile");
  cmd.setAttribute ("subop","snd-done");
  cmd.setAttribute ("xferid",id);
  chunkRoot.appendChild (cmd);
  QByteArray msgtext = chunkDoc.toString().toUtf8 ();
  emit Outgoing (msgtext);
}

void
ChatContent::SendChunk (XferInfo & info ,
                        const QByteArray & data)
{
  QDomDocument chunkDoc ("Egalite");
  QDomElement  chunkRoot = chunkDoc.createElement ("Egalite");
  chunkRoot.setAttribute ("version", protoVersion);
  chunkDoc.appendChild (chunkRoot);
  QDomElement chunk = chunkDoc.createElement ("cmd");
  chunk.setAttribute ("op","sendfile");
  chunk.setAttribute ("subop","chunk-data");
  chunk.setAttribute ("xferid",info.id);
  info.lastChunk += 1;
  chunk.setAttribute ("chunk",QString::number(info.lastChunk));
  QDomText txt = chunkDoc.createTextNode (QString (data.toBase64()));
  chunk.appendChild (txt);
  chunkRoot.appendChild (chunk);
  QByteArray msgbytes = chunkDoc.toString().toUtf8();
  emit Outgoing (msgbytes);
}

void 
ChatContent::SendfileDeny (QDomElement & msg)
{
  QString id = msg.attribute ("xferid");
  CloseTransfer (id);
}

void
ChatContent::CloseTransfer (const QString & id, bool good)
{
  xferState.erase (id);
  QFile *fp = xferFile[id];
  QString filename (tr("unknown file"));
  if (fp) {
    filename = fp->fileName();
    fp->close();
    delete fp;
  }
  xferFile.erase (id);
  QMessageBox finished (this);
  finished.setText (tr("Transfer of file \"%1\" ended %2")
                    .arg (filename)
                    .arg (good ? QString ("with Success")
                               : QString ("with Errors")));
  QTimer::singleShot (30000, &finished, SLOT (accept()));
  finished.exec ();
}

void 
ChatContent::SendfileChunkAck (QDomElement & msg)
{
  QString id = msg.attribute ("xferid");
  SendNextPart (id);
}

void 
ChatContent::SendfileChunkData (QDomElement & msg)
{
  QString id = msg.attribute ("xferid");
  QFile * fp = xferFile[id];
  if (fp) {
    XferStateMap::iterator stateIt = xferState.find (id);
    if (stateIt == xferState.end()) {
      return;
    }
    XferInfo & info = stateIt->second;
    QByteArray data = msg.text ().toUtf8();
    data = QByteArray::fromBase64 (data);
    fp->write (data);
    int num = msg.attribute ("chunk").toULongLong ();
    if (num > info.lastChunk) {
      info.lastChunk = num;
      AckChunk (id, num);
    } else {
      qDebug () << "WARNING: file transfer parts out of order";
      AbortTransfer (id);
    }
  }
}

void
ChatContent::AckChunk (const QString & id, quint64 num)
{
  QDomDocument  ackDoc ("Egalite");
  QDomElement   ack = ackDoc.createElement ("Egalite");
  ack.setAttribute ("version",protoVersion);
  ackDoc.appendChild (ack);
  QDomElement cmd = ackDoc.createElement ("cmd");
  cmd.setAttribute ("op","sendfile");
  cmd.setAttribute ("subop","chunk-ack");
  cmd.setAttribute ("xferid",id);
  cmd.setAttribute ("chunk",QString::number (num));
  ack.appendChild (cmd);
  QByteArray msgtext = ackDoc.toString().toUtf8();
  emit Outgoing (msgtext);
}

void
ChatContent::AbortTransfer (const QString & id)
{
  CloseTransfer (id);
  QDomDocument  abortDoc ("Egalite");
  QDomElement   nack = abortDoc.createElement ("Egalite");
  nack.setAttribute ("version",protoVersion);
  abortDoc.appendChild (nack);
  QDomElement cmd = abortDoc.createElement ("cmd");
  cmd.setAttribute ("op","sendfile");
  cmd.setAttribute ("subop","abort");
  cmd.setAttribute ("xferid",id);
  nack.appendChild (cmd);
  QByteArray msgtext = abortDoc.toString().toUtf8();
  emit Outgoing (msgtext);
}

void 
ChatContent::SendfileSendReq (QDomElement & msg)
{
  QString filename = msg.attribute ("name");
  quint64 filesize = msg.attribute ("size").toULongLong();
  QString xferId   = msg.attribute ("xferid");
  QMessageBox askReceive (this);
  askReceive.setText (tr("Accept file %1 size %2 Bytes?")
                       .arg (filename)
                        .arg(filesize));
  askReceive.setStandardButtons (QMessageBox::Save 
                               | QMessageBox::Discard);
  askReceive.setDefaultButton (QMessageBox::Discard);
  int ans = askReceive.exec ();
  QString subop ("deny");
  bool goahead (false);
  switch (ans) {
  case QMessageBox::Save:
    goahead = OpenSaveFile (xferId,filename);
    if (goahead) {
      subop = "goahead";
    }
    break;
  case QMessageBox::Discard:
  default:
    break;
  }
  QDomDocument  responseDoc ("Egalite");
  QDomElement response = responseDoc.createElement ("Egalite");
  response.setAttribute ("version",protoVersion);
  responseDoc.appendChild (response);
  QDomElement cmd = responseDoc.createElement ("cmd");
  cmd.setAttribute ("op","sendfile");
  cmd.setAttribute ("subop",subop);
  cmd.setAttribute ("xferid",xferId);
  response.appendChild (cmd);
  QByteArray msgbytes = responseDoc.toString().toUtf8();
  emit Outgoing (msgbytes);
}

void 
ChatContent::SendfileRcvDone (QDomElement & msg)
{
  QString id = msg.attribute ("xferid");
  CloseTransfer (id, true);
}

void 
ChatContent::SendfileAbort (QDomElement & msg)
{
  QString id = msg.attribute ("xferid");
  CloseTransfer (id);
}

bool
ChatContent::OpenSaveFile (const QString &id, const QString & filename)
{
  QString savename = QFileDialog::getSaveFileName (this, 
                      tr ("Store Received File"),
                      filename);
  if (savename.length () > 0) {
    XferInfo info;
    info.id = id;
    info.fileSize = 0;
    info.lastChunk = 0;
    info.lastChunkAck = 0;
    QFile * fp = new QFile (savename);
    bool isopen = fp->open (QFile::WriteOnly);
    if (isopen) {
      xferFile [id] = fp;
      xferState [id] = info;
      return true;
    }
  }
  return false;
}


} // namespace
