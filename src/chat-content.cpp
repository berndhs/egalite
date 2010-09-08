
#include "deliberate.h"
#include "chat-content.h"
#include "direct-message.h"
#include "direct-parser.h"
#include <QXmppMessage.h>
#include <QXmlStreamWriter>
#include <QDomDocument>
#include <QDomElement>
#include <QDomText>
#include <QDomAttr>
#include <QRegExp>
#include <QDesktopServices>
#include <QTimer>
#include <QMessageBox>
#include <QUuid>
#include <QLabel>
#include <QSslSocket>
#include <QDebug>
#include "link-mangle.h"
#include <iostream>

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
   ioDev (0),
   directParser (this),
   chatMode (ChatModeRaw),
   rcvCount (0),
   sendCount (0),
   protoVersion (QString()),
   sendSinceBeat (0),
   maxBurst (0),
   heartBeat (0),
   stateUpdate (0),
   extraSendMessage (tr("\n%1 active")),
   extraSendHighlight (false),
   extraSendStyle ("QPushButton { font-style:italic;}"),
   sendFileWindow (1),
   sendChunkSize (1*1024),
   dateMask ("yy-MM-dd hh:mm:ss"),
   chatLine (tr("(%1) <b style=\"font-size:small; "
                 "color:@color@;\">%2</b>: %3")),
   localHtmlColor ("blue"),
   remoteHtmlColor ("red")
{
  ui.setupUi (this);
  
  plainSendMessage = ui.sendFileButton->text ();
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
  heartBeat = new QTimer (this);
  stateUpdate = new QTimer (this);
  heartBeat->stop ();
  stateUpdate->stop ();
  connect (heartBeat, SIGNAL (timeout()), this, SLOT (Heartbeat ()));
  connect (stateUpdate, SIGNAL (timeout()), this, SLOT (UpdateXferDisplay ()));
  QTimer * debugTimer = new QTimer (this);
  connect (debugTimer, SIGNAL (timeout()), this, SLOT (DebugCheck()));
  debugTimer->start (60*1000);
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
  extraSendMessage = deliberate::Settings().value("style/xfermessage",
                                            extraSendMessage)
                                            .toString();
  deliberate::Settings().setValue ("style/xfermessage",extraSendMessage);
  extraSendStyle = deliberate::Settings().value("style/xferstyle",
                                            extraSendStyle)
                                            .toString();
  deliberate::Settings().setValue ("style/xferstyle",extraSendStyle);
                          
  rcvCount = 0;
  sendCount = 0;
  qDebug () << " style sheet for sendFile button "
            << ui.sendFileButton->styleSheet();
  normalStyle = ui.sendFileButton->styleSheet ();
  connect (&directParser, SIGNAL (Message (DirectMessage)),
           this, SLOT (IncomingDirect (DirectMessage)));
  directParser.Start ();
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
ChatContent::SetInput (QSslSocket * dev)
{
  ioDev = dev;
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
ChatContent::InputAvailable ()
{
  if (ioDev && ioDev->isReadable()) { 
    /** QDomDocument.setContent is not reentrannt so we cannot read
        directly from the socket
    */
    QByteArray bytes = ioDev->readAll ();
    directParser.AddData (bytes);
    directParser.TryRead ();
  }
}

void
ChatContent::IncomingDirect (DirectMessage msg)
{
qDebug () << " Incoming Direct op " << msg.Op () << "/" << msg.Subop();
  QString op = msg.Op();
  if (op == "xmpp") {
    QDomDocument xmppDoc;
    QByteArray data = msg.Data();
    xmppDoc.setContent (data);
    ReadDomDoc (xmppDoc);
  } else if (op == "ctl") {
    ReadCtl (msg);
  } else if (op == "sendfile") {
    ReadSendfile (msg);
  }
}

void
ChatContent::ReadCtl (DirectMessage & msg)
{
  QString subop = msg.Subop ();
  if (subop == "heartbeat") {
    qDebug () << " heartbead received, thank you";
  } else {
    qDebug () << " Unknown Control Message subop " << subop;
  }
}

void
ChatContent::ReadSendfile (DirectMessage & msg)
{
  DumpAttributes (msg, " ---->> Incoming V 0.3 sendfile message ");
  QString subop = msg.Subop();
  QString id = msg.Attribute ("xferid");
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
ChatContent::ReadDomDoc (QDomDocument & doc, bool isLocal)
{
  QDomElement root = doc.documentElement();
qDebug () << "INcoming Tag " << root.tagName();
  if (root.tagName() == "message") { 
    QXmppMessage msg;
    msg.parse (root);
    IncomingXmpp (msg, isLocal);
  } else {
    qDebug () << " invalid tag " << root.tagName();
  }
}

void
ChatContent::SendDomDoc (QDomDocument & doc)
{
  static QByteArray spaces (8,' ');
  QByteArray msgbytes = doc.toString().toUtf8();
  sendSinceBeat ++;
  if (ioDev) {
    ioDev->write (msgbytes);
    ioDev->write (spaces);
    ioDev->flush ();
  } else {
    emit Outgoing (msgbytes);
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
      ChangeProto (this, "0.3");
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
      SetProtoVersion ("0.3");
      EmbedDirectMessage (outbuf);
    } else {
      sendCount++;
      emit Outgoing (outbuf);
    }
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
    if (sendSinceBeat < 2) {
      QDomDocument heartDoc ("egalite");
      QDomElement root = heartDoc.createElement ("egalite");
      root.setAttribute ("version",protoVersion); 
      heartDoc.appendChild (root);
      QDomElement msg = heartDoc.createElement ("cmd");
      msg.setAttribute ("op","ctl");
      msg.setAttribute ("subop","heartbeat");
      root.appendChild (msg);
      SendDomDoc (heartDoc);
    }
    directParser.TryRead (maxBurst);
    sendSinceBeat = 0;
  }
}

void
ChatContent::EmbedDirectMessage (QByteArray & raw)
{
  qDebug () << " sending " << raw;
  QDomDocument directDoc ("egalite");
  QDomElement root = directDoc.createElement ("egalite");
  root.setAttribute ("version",protoVersion);
  QDomElement msg = directDoc.createElement ("cmd");
  msg.setAttribute ("op","xmpp");
  msg.setAttribute ("subop","msg");
  msg.setAttribute ("num",QString::number(sendCount));
  root.appendChild (msg);
  QDomText txt = directDoc.createTextNode (raw);
  msg.appendChild (txt);
  directDoc.appendChild (root);
  sendCount++;
  SendDomDoc (directDoc);
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
  qDebug () << " supposed to send first part for " << id;
  QFile * fp = xferFile[id];
  if (!fp) {
    return;
  }
  XferInfo & info = xferState[id];
  QFileInfo finfo (fp->fileName());
  QDomDocument requestDoc ("egalite");
  QDomElement  request = requestDoc.createElement ("egalite");
  request.setAttribute ("version", protoVersion);
  requestDoc.appendChild (request);
  QDomElement cmd = requestDoc.createElement ("cmd");
  cmd.setAttribute ("op","sendfile");
  cmd.setAttribute ("subop","sendreq");
  cmd.setAttribute ("xferid",id);
  cmd.setAttribute ("name",finfo.fileName());
  cmd.setAttribute ("size",info.fileSize);
  request.appendChild (cmd);
  SendDomDoc (requestDoc);
  StartXferDisplay ();
  ListActiveTransfers (false);
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
  QDomDocument chunkDoc ("egalite");
  QDomElement  chunkRoot = chunkDoc.createElement ("egalite");
  chunkRoot.setAttribute ("version", protoVersion);
  chunkDoc.appendChild (chunkRoot);
  QDomElement cmd = chunkDoc.createElement ("cmd");
  cmd.setAttribute ("op","sendfile");
  cmd.setAttribute ("subop","snd-done");
  cmd.setAttribute ("xferid",id);
  chunkRoot.appendChild (cmd);
  SendDomDoc (chunkDoc);
  CloseTransfer (id, true);
}

void
ChatContent::SendChunk (XferInfo & info ,
                        const QByteArray & data)
{
  QDomDocument chunkDoc ("egalite");
  QDomElement  chunkRoot = chunkDoc.createElement ("egalite");
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
  SendDomDoc (chunkDoc);
}

void 
ChatContent::SendfileDeny (DirectMessage & msg)
{
  QString id = msg.Attribute ("xferid");
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
  UpdateXferDisplay ();
  QMessageBox finished (this);
  finished.setText (tr("Transfer of file \"%1\" ended %2")
                    .arg (filename)
                    .arg (good ? QString ("with Success")
                               : QString ("with Errors")));
  QTimer::singleShot (30000, &finished, SLOT (accept()));
  finished.exec ();
  ListActiveTransfers (false);
}

void 
ChatContent::SendfileChunkAck (DirectMessage & msg)
{
  QString id = msg.Attribute ("xferid");
  SendNextPart (id);
}

void 
ChatContent::SendfileChunkData (DirectMessage & msg)
{
  QString id = msg.Attribute ("xferid");
  QFile * fp = xferFile[id];
  if (fp) {
    XferStateMap::iterator stateIt = xferState.find (id);
    if (stateIt == xferState.end()) {
      return;
    }
    XferInfo & info = stateIt->second;
    QByteArray data = msg.Data ();
    data = QByteArray::fromBase64 (data);
    fp->write (data);
    qulonglong num = msg.Attribute ("chunk").toULongLong ();
    if (num > info.lastChunk) {
      info.lastChunk = num;
      AckChunk (id, num);
    } else {
      AbortTransfer (id, "File transfer: out of order chunks");
    }
  }
}

void
ChatContent::AckChunk (const QString & id, quint64 num)
{
  QDomDocument  ackDoc ("egalite");
  QDomElement   ack = ackDoc.createElement ("egalite");
  ack.setAttribute ("version",protoVersion);
  ackDoc.appendChild (ack);
  QDomElement cmd = ackDoc.createElement ("cmd");
  cmd.setAttribute ("op","sendfile");
  cmd.setAttribute ("subop","chunk-ack");
  cmd.setAttribute ("xferid",id);
  cmd.setAttribute ("chunk",QString::number (num));
  ack.appendChild (cmd);
  SendDomDoc (ackDoc);
}

void
ChatContent::AbortTransfer (const QString & id, QString msg)
{
  qDebug () << " Abort Transfer: " << msg;
  CloseTransfer (id);
  QDomDocument  abortDoc ("egalite");
  QDomElement   nack = abortDoc.createElement ("egalite");
  nack.setAttribute ("version",protoVersion);
  abortDoc.appendChild (nack);
  QDomElement cmd = abortDoc.createElement ("cmd");
  cmd.setAttribute ("op","sendfile");
  cmd.setAttribute ("subop","abort");
  cmd.setAttribute ("xferid",id);
  nack.appendChild (cmd);
  SendDomDoc (abortDoc);
  QMessageBox box;
  box.setText (tr("Failure: ") + msg);
  QTimer::singleShot (15000, &box, SLOT (reject()));
  box.exec ();
}

void 
ChatContent::SendfileSendReq (DirectMessage & msg)
{
  QString filename = msg.Attribute ("name");
  quint64 filesize = msg.Attribute ("size").toULongLong();
  QString xferId   = msg.Attribute ("xferid");
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
      StartXferDisplay ();
    }
    break;
  case QMessageBox::Discard:
  default:
    break;
  }
  QDomDocument  responseDoc ("egalite");
  QDomElement response = responseDoc.createElement ("egalite");
  response.setAttribute ("version",protoVersion);
  responseDoc.appendChild (response);
  QDomElement cmd = responseDoc.createElement ("cmd");
  cmd.setAttribute ("op","sendfile");
  cmd.setAttribute ("subop",subop);
  cmd.setAttribute ("xferid",xferId);
  response.appendChild (cmd);
  SendDomDoc (responseDoc);
}

void 
ChatContent::SendfileRcvDone (DirectMessage & msg)
{
  QString id = msg.Attribute ("xferid");
  CloseTransfer (id, true);
}

void 
ChatContent::SendfileAbort (DirectMessage & msg)
{
  QString id = msg.Attribute ("xferid");
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

void
ChatContent::StartXferDisplay ()
{
  if (stateUpdate == 0) {
    stateUpdate = new QTimer (this);
    connect (stateUpdate, SIGNAL (timeout ()),
             this, SLOT (UpdateXferDisplay ()));
  }
  if (!stateUpdate->isActive() || (stateUpdate->interval() < 1)) {
    stateUpdate->start (1000);
  }
  UpdateXferDisplay ();
}

void
ChatContent::UpdateXferDisplay ()
{
  int howmany = xferState.size();
  if (howmany == 0) {
    ui.sendFileButton->setText (plainSendMessage);
    ui.sendFileButton->setStyleSheet (normalStyle);
    stateUpdate->stop ();
  } else {
    QString msgPat ("%1 %2");
    QString extra;
    extraSendHighlight = ! extraSendHighlight;  
    extra = extraSendMessage.arg(howmany);
    ui.sendFileButton->setText (msgPat.arg(plainSendMessage)
                                      .arg (extra));
    if (extraSendHighlight) {
      ui.sendFileButton->setStyleSheet (extraSendStyle);
    } else {
      ui.sendFileButton->setStyleSheet (normalStyle);
    }
  }
}

void
ChatContent::DumpAttributes (DirectMessage & elt, QString msg)
{
  DirectMessage::AttributeMap atts = elt.Attributes ();
  qDebug () << msg;
  qDebug () << " message " << atts.size() << " attributes: ";
  DirectMessage::AttributeMap::iterator it;
  for (it = atts.begin(); it != atts.end(); it++) {
    qDebug () << it->first << " = " << it->second;
  }
}

void
ChatContent::ListActiveTransfers (bool showBox)
{
  XferStateMap::iterator  stateIt;
  QStringList report;
  for (stateIt = xferState.begin (); stateIt != xferState.end(); stateIt++) {
    QString line;
    XferInfo  & info = stateIt->second;
    line.append ("Id: ");
    line.append (info.id);
    line.append (" size ");
    line.append (QString::number (info.fileSize));
    line.append (" lastChunk ");
    line.append (QString::number (info.lastChunk));
    line.append (" lastAck ");
    line.append (QString::number (info.lastChunkAck));
    report << line;
  }
  qDebug () << " Active Transfers:";
  qDebug () << report;
  if (showBox) {
    QMessageBox  box;
    box.setWindowTitle ("Active File Transfers");
    box.setText (report.join("\n"));
    box.exec ();
  }
}

void
ChatContent::DebugCheck ()
{
  if (ioDev) {
    qDebug() << " Debug Check Chat COntent " << this;
    qDebug() << " socket " << ioDev << " ready " << ioDev->isReadable();
    qDebug() << " socket bytes avail " << ioDev->bytesAvailable();
  }
}

} // namespace
