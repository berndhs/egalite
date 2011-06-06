
#include "deliberate.h"
#include "chat-content.h"
#include "direct-message.h"
#include "direct-parser.h"
#if DO_AUDIO
#include "audio-message.h"
#endif
#if D_MOBI_AUDIO
#include "mobi-audi-message.h"
#endif
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
#include "html-mangle.h"
#include <iostream>

/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2010, 2011 Bernd Stramm
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
   activeAudioMessage (tr("\n playing")),
   qtAudioOk (false),
   sendFileWindow (1),
   sendChunkSize (4*1024),
#if DO_AUDIO
   audio (this),
#endif
#if DO_MOBI_AUDIO
   mobiAudio (this),
#endif
   dateMask ("yy-MM-dd hh:mm:ss"),
   chatLine (tr("(%1) <b style=\"font-size:small; "
                 "color:@color@;\">%2</b>: %3")),
   localHtmlColor ("blue"),
   remoteHtmlColor ("red")
{
  ui.setupUi (this);
  #if DO_AUDIO
  #if DELIBERATE_QT_AUDIO_OK
  qtAudioOk = true;
  #else
  qtAudioOk = false;
  #endif
  #else
  qtAudioOk = false;
  #endif
  
  #if DO_MOBI_AUDIO
  #if DELIBERATE_QT_AUDIO_OK
  qtAudioOk = true;
  #else
  qtAudioOk = false;
  #endif
  #else
  qtAudioOk = false;
  #endif

  plainSendMessage = ui.sendFileButton->text ();
  plainAudioMessage = ui.samButton->text ();
  connect (ui.quitButton, SIGNAL (clicked()), this, SLOT (EndChat()));
  connect (ui.saveButton, SIGNAL (clicked()), this, SLOT (SaveContent()));
  connect (ui.sendButton, SIGNAL (clicked()), this, SLOT (Send()));
  connect (ui.sendFileButton, SIGNAL (clicked()), 
          this, SLOT (StartFileSend ()));
  connect (ui.samButton, SIGNAL (clicked()),
          this, SLOT (StartAudioSend ()));
  connect (ui.textHistory, SIGNAL (anchorClicked (const QUrl&)),
          this, SLOT (HandleAnchor (const QUrl&)));
#if DO_AUDIO
  connect (&audio, SIGNAL (HaveAudio(qint64)), 
          this, SLOT (SendAudioRequest(qint64)));
  connect (&audio, SIGNAL (PlayStarting()), this, SLOT (AudioStarted()));
  connect (&audio, SIGNAL (PlayFinished()), this, SLOT (AudioStopped()));
#endif
#if DO_MOBI_AUDIO
  connect (&mobiAudio, SIGNAL (HaveAudio(qint64)), 
          this, SLOT (SendAudioRequest(qint64)));
  connect (&mobiAudio, SIGNAL (PlayStarting()), this, SLOT (AudioStarted()));
  connect (&mobiAudio, SIGNAL (PlayFinished()), this, SLOT (AudioStopped()));
#endif
  ui.quitButton->setDefault (false);
  ui.saveButton->setDefault (false);
  ui.sendFileButton->setDefault (false);
  ui.samButton->setDefault (false);
  ui.quitButton->setAutoDefault (false);
  ui.saveButton->setAutoDefault (false);
  ui.samButton->setAutoDefault (false);
  ui.sendFileButton->setAutoDefault (false);
  ui.sendFileButton->hide ();
  ui.samButton->hide ();
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
    ui.samButton->show ();
    ui.samButton->setEnabled (qtAudioOk);
  } else {
    ui.sendFileButton->hide ();
    ui.samButton->hide ();
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
  } else if (subop == "samreq") {
    SendfileSamReq (msg);
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
qDebug () << " sending Dom Doc op " << doc.toByteArray().left(256);
  if (ioDev) {
    ioDev->write (msgbytes);
    ioDev->write (spaces);
    ioDev->flush ();
  } else {
    emit Outgoing (msgbytes);
  }
}

void
ChatContent::SendDirectMessage (DirectMessage & msg)
{
  qDebug () << __PRETTY_FUNCTION__ ;
  DumpAttributes (msg, " <------- Outgoing Direct");
  // prepare message in XML format
  QBuffer outbuf;
  outbuf.open (QBuffer::WriteOnly);
  QXmlStreamWriter xmlout;
  xmlout.setDevice (&outbuf);
  xmlout.setAutoFormatting (true);
  xmlout.setAutoFormattingIndent (1);
  xmlout.writeStartDocument ();
  xmlout.writeStartElement ("egalite");
  xmlout.writeAttribute ("version",protoVersion);
  xmlout.writeStartElement ("cmd");
  DirectMessage::AttributeMap atts = msg.Attributes();
  DirectMessage::AttributeMap::iterator ait;
  for (ait=atts.begin(); ait != atts.end(); ait++) {
    xmlout.writeAttribute (ait->first, ait->second);
  }
  QByteArray & data = msg.Data();
  if (data.size() > 0) {
    xmlout.writeCharacters (data);
  }
  xmlout.writeEndElement (); // cmd
  xmlout.writeEndElement (); // egalite
  xmlout.writeEndDocument ();
  // send bytes
  if (ioDev) {
    ioDev->write (outbuf.buffer());
    ioDev->flush ();
qDebug () << " op " << msg.Op ();
qDebug () << " message sent: " << outbuf.buffer().left (512);
  } else {
    emit Outgoing (outbuf.buffer());
  }
  sendSinceBeat ++;
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
                           .arg (HtmlMangle::Sanitize(body))
                            ;
  QString cookedText = HtmlMangle::Anchorize (msgtext,
                                   HtmlMangle::HttpExp (),
                                   HtmlMangle::HtmlAnchor);
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
      DirectMessage dmsg;
      dmsg.SetOp ("ctl");
      dmsg.SetSubop ("heartbeat");
      SendDirectMessage (dmsg);
    }
    directParser.TryRead (maxBurst);
    sendSinceBeat = 0;
  }
}

void
ChatContent::EmbedDirectMessage (QByteArray & raw)
{
  qDebug () << " sending " << raw;
  DirectMessage dmsg;
  dmsg.SetOp ("xmpp");
  dmsg.SetSubop ("msg");
  dmsg.SetAttribute ("num",QString::number (sendCount));
  dmsg.SetData (raw);
qDebug () << " embed op " << dmsg.Op () << " raw " << dmsg.Data();
  SendDirectMessage (dmsg);
  sendCount++;
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
ChatContent::StartAudioSend ()
{
#if DO_AUDIO
  ui.samButton->setEnabled  (false);
  audio.Record (ui.textHistory->pos(), ui.chatInput->size());
#else
  #if DO_MOBI_AUDIO
  ui.samButton->setEnabled (false);
  mobiAudio.Record (ui.textHistory->pos(), ui.chatInput->size());
  #endif
#endif
}

void
ChatContent::SendAudioRequest (qint64 usecs)
{
#if DO_AUDIO || DO_MOBI_AUDIO
  XferInfo  info;
  info.id = QUuid::createUuid ().toString();
  info.lastChunk = 0;
  info.kind = XferInfo::Xfer_Audio;
  info.inout = XferInfo::Xfer_Out;
  info.removeOnComplete = true;
#if DO_AUDIO
  QFile * fp =  new QFile (audio.Filename());
#else 
  #if DO_MOBI_AUDIO
  QFile * fp =  new QFile (mobiAudio.Filename());
  #endif
#endif
  bool isopen = fp ->open (QFile::ReadOnly);
  info.fileSize = fp->size ();
qDebug () << " file open " << isopen << " size " << fp->size();
  if (isopen) {
    xferFile [info.id] = fp;
    xferState [info.id] = info;  

    DirectMessage req;
    req.SetOp ("sendfile");
    req.SetSubop ("samreq");
    req.SetAttribute ("xferid",info.id);
    req.SetAttribute ("size",QString::number (info.fileSize));
    req.SetAttribute ("usecs",QString::number (usecs));
#if DO_AUDIO
    req.SetAttribute ("name",audio.OutFormat().codec());
    req.SetAttribute ("rate",
                       QString::number (audio.OutFormat().frequency()));
    req.SetAttribute ("channels",
                       QString::number (audio.OutFormat().channels()));
    req.SetAttribute ("samplesize",
                       QString::number (audio.OutFormat().sampleSize()));
    req.SetAttribute ("codec",audio.OutFormat().codec());
    req.SetAttribute ("byteorder",
                       QString::number (audio.OutFormat().byteOrder()));
    req.SetAttribute ("sampletype",
                       QString::number (audio.OutFormat().sampleType()));
#else
  #if DO_MOBI_AUDIO
    req.SetAttribute ("name",mobiAudio.OutFormat().codec());
    req.SetAttribute ("rate",
                       QString::number (mobiAudio.OutFormat().bitRate()));
    req.SetAttribute ("channels",
                       QString::number (mobiAudio.OutFormat().channelCount()));
    req.SetAttribute ("codec",mobiAudio.OutFormat().codec());
  #endif
#endif
    SendDirectMessage (req);
    StartXferDisplay ();
  }
#endif
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
    info.kind = XferInfo::Xfer_File;
    info.inout = XferInfo::Xfer_Out;
    info.pipeline = 5;
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
  DirectMessage req;
  req.SetOp ("sendfile");
  req.SetSubop ("sendreq");
  req.SetAttribute ("xferid",id);
  req.SetAttribute ("name",finfo.fileName());
  req.SetAttribute ("size",QString::number (info.fileSize));
  SendDirectMessage (req);
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
      for (int ch=0; ch< info.pipeline; ch++) {
        QByteArray data = fp->read (sendChunkSize);
        if (data.size() < 1) {
          SendFinished (id);
          break;
        } else {
          SendChunk (info, data);
        }
        qDebug () << " send chunk " << ch << " size " << sendChunkSize;
      }
      info.pipeline = 1;
    }
  }
}

void
ChatContent::SendFinished (const QString & id)
{
  DirectMessage msg;
  msg.SetOp ("sendfile");
  msg.SetSubop ("snd-done");
  msg.SetAttribute ("xferid",id);
  SendDirectMessage (msg);
  CloseTransfer (id, true);
}

void
ChatContent::SendChunk (XferInfo & info ,
                        const QByteArray & data)
{
  DirectMessage msg;
  msg.SetOp ("sendfile");
  msg.SetSubop ("chunk-data");
  msg.SetAttribute ("xferid",info.id);
  info.lastChunk += 1;
  msg.SetAttribute ("chunk",QString::number (info.lastChunk));
  msg.SetData (data.toBase64());
  SendDirectMessage (msg);
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
  bool removeFile (false);
  XferInfo::XferKind kind (XferInfo::Xfer_None);
  XferInfo::XferDirection inout (XferInfo::Xfer_Out);
  XferStateMap::iterator  stateIt = xferState.find(id);
  if (stateIt != xferState.end()) {
    kind = stateIt->second.kind;
    inout = stateIt->second.inout;
    removeFile = stateIt->second.removeOnComplete;
  }
  xferState.erase (id);
  QFile *fp = xferFile[id];
  QString filename (tr("unknown file"));
  if (fp) {
    filename = fp->fileName();
    fp->close();
    if (removeFile) {
      fp->remove ();
    }
    if (kind == XferInfo::Xfer_File) {
      delete fp;
    }
  }
  xferFile.erase (id);
  UpdateXferDisplay ();
  QMessageBox finished (this);
  switch (kind) {
  case XferInfo::Xfer_File:
    finished.setText (tr("Transfer of file \"%1\" ended %2")
                    .arg (filename)
                    .arg (good ? QString ("with Success")
                               : QString ("with Errors")));
    QTimer::singleShot (30000, &finished, SLOT (accept()));
    finished.exec ();
    break;
  case XferInfo::Xfer_Audio:
#if DO_AUDIO || DO_MOBI_AUDIO
    if (inout == XferInfo::Xfer_In) {
#if DO_AUDIO
      audio.FinishReceive ();
#else
  #if DO_MOBI_AUDIO
      mobiAudio.FinishReceive ();
  #endif
#endif
    } else if (inout == XferInfo::Xfer_Out) {
      ui.samButton->setEnabled (qtAudioOk);
    }
#endif
    break;
  default:
    qDebug () << " invalid transfer type " << kind << " finished"; 
    break;
  }
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
  DirectMessage msg;
  msg.SetOp ("sendfile");
  msg.SetSubop ("chunk-ack");
  msg.SetAttribute ("xferid",id);
  msg.SetAttribute ("chunk", QString::number (num));
  SendDirectMessage (msg);
}

void
ChatContent::AbortTransfer (const QString & id, QString msg)
{
  qDebug () << " Abort Transfer: " << msg;
  CloseTransfer (id);
  DirectMessage nack;
  nack.SetOp ("sendfile");
  nack.SetSubop ("abort");
  nack.SetAttribute ("xferid",id);
  SendDirectMessage (nack);
  QMessageBox box;
  box.setText (tr("Failure: ") + msg);
  QTimer::singleShot (15000, &box, SLOT (reject()));
  box.exec ();
}

void
ChatContent::SendfileSamReq (DirectMessage & msg)
{
  qDebug () << " received SAM request";
  QString subop ("deny");
  QString xferId = msg.Attribute ("xferid");
#if DO_AUDIO || DO_MOBI_AUDIO
#if DO_AUDIO
  if (qtAudioOk && (!audio.BusyReceive())) {   
#else
  #if DO_MOBI_AUDIO
  if (qtAudioOk && (!mobiAudio.BusyReceive())) {   
  #endif
#endif
    XferInfo info;
    info.id = xferId;
    info.fileSize = 0;
    info.lastChunk = 0;
    info.lastChunkAck = 0;
    info.kind = XferInfo::Xfer_Audio;
    info.inout = XferInfo::Xfer_In;
#if DO_AUDIO
    audio.StartReceive ();
    QFile * fp = audio.InFile ();
#endif
#if DO_MOBI_AUDIO
    mobiAudio.StartReceive ();
    QFile * fp = mobiAudio.InFile ();
#endif
    bool isopen = fp->open (QFile::WriteOnly);
qDebug () << __PRETTY_FUNCTION__ << " open " << fp->fileName() <<  isopen;
    if (isopen) {
      xferFile [xferId] = fp;
      xferState [xferId] = info;
      subop = "goahead";
#if DO_AUDIO
      QAudioFormat & fmt (audio.InFormat());  // NOTE the reference, not const!
      fmt.setFrequency (msg.Attribute("rate").toInt());
      fmt.setChannels (msg.Attribute("channels").toInt());
      fmt.setSampleSize (msg.Attribute("samplesize").toInt());
      fmt.setByteOrder (QAudioFormat::Endian 
                          (msg.Attribute("byteorder").toInt()));
      fmt.setSampleType (QAudioFormat::SampleType 
                          (msg.Attribute("sampletype").toInt()));
      audio.SetInLength (msg.Attribute("usecs").toLongLong());
#else
  #if DO_MOBI_AUDIO
  #endif
#endif
      StartXferDisplay ();
    }
  }
#endif
  DirectMessage resp;
  resp.SetOp ("sendfile");
  resp.SetSubop (subop);
  resp.SetAttribute ("xferid",xferId);
  SendDirectMessage (resp);
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
  DirectMessage resp;
  resp.SetOp ("sendfile");
  resp.SetSubop (subop);
  resp.SetAttribute ("xferid",xferId);
  SendDirectMessage (resp);
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
    info.kind = XferInfo::Xfer_File;
    info.inout =XferInfo::Xfer_In;
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
ChatContent::AudioStarted ()
{
  QString msgPat ("%1 %2");
  ui.samButton->setText (msgPat.arg(plainAudioMessage)
                               .arg(activeAudioMessage));
}

void
ChatContent::AudioStopped ()
{
  ui.samButton->setText (plainAudioMessage);
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
