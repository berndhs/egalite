
#ifndef CHAT_CONTENT_H
#define CHAT_CONTENT_H

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

#include "audio-message.h"
#include "ui_chat-content.h"
#include "direct-parser.h"
#include <QDialog>
#include <QByteArray>
#include <QUrl>
#include <QFile>

class QXmppMessage;
class QDomElement;
class QDomDocument;
class QTimer;
class QLabel;
class QSslSocket;

namespace egalite
{

/** \brief The dialog window that deals with text of a chat connection.
 * 
 * This class knows about displaying text, and grabbing text from 
 * the input area. It doesn't know about how text is sent or received.
 */

class XferInfo {
public:

  enum XferKind {
    Xfer_None = 0,
    Xfer_File = 1,
    Xfer_Audio = 2
  };

  enum XferDirection {
    Xfer_In = 0,
    Xfer_Out = 1
  };
  
  XferInfo ()
    :removeOnComplete (false),
     pipeline (1)
    {}
  ~XferInfo() {}
  
  QString         id;
  XferKind        kind;
  XferDirection   inout;
  quint64         fileSize;
  quint64         lastChunk;
  quint64         lastChunkAck;
  bool            removeOnComplete;
  int             pipeline;
};

class ChatContent : public QDialog 
{
Q_OBJECT

public:

  /** \brief ChatContent::Mode - None: don't send anything
                                  Raw: send bytes
                                  Xmpp: send xmpp
                                  Embed: send Egalite protocol
   */

  enum Mode { ChatModeNone = 0, 
               ChatModeRaw  = 1,
               ChatModeXmpp = 2,
               ChatModeEmbed = 3
             };

  ChatContent (QWidget * parent=0);
  ~ChatContent ();

  void SetMode (Mode mode);

  void SetRemoteName (const QString & name);
  void SetLocalName  (const QString & name);

  void Start (Mode mode,
              const QString & remoteName,
              const QString & localName);
  void Start ();

  QString RemoteName () { return remoteName; }
  QString LocalName () { return localName; }
  QString ProtoVersion () { return protoVersion; }
  void    SetProtoVersion (QString newProto);
  void    SetHeartbeat (int secs);

  void    SetInput (QSslSocket * dev);

public slots:

  void IncomingXmpp (const QXmppMessage &msg, bool isLocal=false);
  void HandleAnchor (const QUrl & url);
  void Stop ();
  void InputAvailable ();
  void IncomingDirect (DirectMessage msg);
  void AudioStarted ();
  void AudioStopped ();

  bool close ();

private slots:

  void Send ();
  void EndChat ();
  void SaveContent ();
  void Heartbeat ();
  void SendAudioRequest (qint64 usecs);
  void StartFileSend ();
  void StartAudioSend ();
  void UpdateXferDisplay ();
  void StartXferDisplay ();
  void DebugCheck ();

signals:

  void Outgoing (const QByteArray &data);
  void Outgoing (const QXmppMessage &msg);
  void Disconnect (QString remote);
  void Activity (QWidget * activeWidget);
  void ChangeProto (QWidget *, QString newproto);
  void AckArrived (QString xferId, quint64 chunkNum);

private:

  typedef std::map <QString, XferInfo>   XferStateMap;
  typedef std::map <QString, QFile*>     XferFileMap;

  void EmbedDirectMessage (QByteArray & raw);
  void ReadCtl (DirectMessage & msg);
  void ReadSendfile (DirectMessage & msg);
  void SendMessage (const QString & content, bool isControl=false);
  void ModeUpdate ();
  void SendFirstPart (const QString & id);
  void SendNextPart (const QString & id);
  void SendFinished (const QString & id);
  void SendChunk (XferInfo & info, const QByteArray & data);
  void SendfileDeny (DirectMessage & msg);
  void SendfileChunkAck (DirectMessage & msg);
  void SendfileChunkData (DirectMessage & msg);
  void SendfileSamReq (DirectMessage & msg);
  void SendfileSendReq (DirectMessage & msg);
  void SendfileRcvDone (DirectMessage & msg);
  void SendfileAbort (DirectMessage & msg);
  bool OpenSaveFile (const QString & id, const QString & filename);
  void AbortTransfer (const QString & id, QString msg = QString());
  void AckChunk (const QString & id, quint64 num);
  void CloseTransfer (const QString & id, bool good=false);
  void SendDomDoc (QDomDocument & doc);
  void ReadDomDoc (QDomDocument & doc, bool isLocal = false);

  void DumpAttributes (DirectMessage & elt, QString msg = QString("debug:"));
  void ListActiveTransfers (bool showBox = false);


  Ui_ChatContent   ui;

  QSslSocket      *ioDev;
  DirectParser     directParser;
  QString          remoteName;
  QString          localName;
  Mode             chatMode;
  quint64          rcvCount;
  quint64          sendCount;
  QString          protoVersion;
  int              sendSinceBeat;
  int              maxBurst;
  QTimer          *heartBeat;
  QTimer          *stateUpdate;
  QString          plainSendMessage;
  QString          extraSendMessage;
  bool             extraSendHighlight;
  QString          extraSendStyle;
  QString          normalStyle;
  QString          plainAudioMessage;
  QString          activeAudioMessage;
  bool             qtAudioOk;
  

  int              sendFileWindow;
  int              sendChunkSize;
  XferStateMap     xferState;
  XferFileMap      xferFile;
  AudioMessage     audio;

  QString          dateMask;
  QString          chatLine;
  QString          localLine;
  QString          remoteLine;
  QString          localHtmlColor;
  QString          remoteHtmlColor;

};

} // namespace

#endif
