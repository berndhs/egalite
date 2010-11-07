#ifndef DENADA_H
#define DENADA_H

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
#include <QMainWindow>
#include "ui_irc-sock.h"
#include "config-edit.h"
#include "helpview.h"
#include <QTcpSocket>
#include <QFile>
#include <QStringList>

using namespace deliberate;

namespace egalite 
{

class IrcChannelBox;
class IrcChannelGroup;

class IrcSock : public QDialog
{
Q_OBJECT

public:

  IrcSock (QWidget *parent=0);

  void  CloseCleanup ();

  void  SetInLog (const QString & filename);
  void  SetOutLog (const QString & filename);

  void  InChanMsg (const QString & chan, 
                   const QString & form,
                   const QString & msg);
  void  InUserMsg (const QString & from, 
                   const QString & to, 
                   const QString & msg);

public slots:

  bool  Run ();

private slots:

  void Exit ();
  void Exiting ();
  void TryConnect ();
  void TryDisconnect ();
  void ConnectionReady ();
  void ConnectionGone ();
  void Receive ();
  void DidSend (qint64 bytes);
  void FakeLogin ();
  void ChanActive (IrcChannelBox * chan);
  void ChanInUse (IrcChannelBox * chan);

  void Outgoing (QString chan, QString msg);
  
  void SendScript ();
  void RollScript ();
  void SendScriptHead ();
  void Send ();
  void Send (QString data);
  void SendPing ();

  void SockError (QAbstractSocket::SocketError err);

private:

  void Connect ();
  void AddChannel (const QString & chanName);
  void SendData (const QString & data);
  void LogRaw (const QString & raw);

  static void TransformPRIVMSG (IrcSock * context,
                                QString & result, 
                                QString & first, 
                                QString & rest);
  static void TransformJOIN    (IrcSock * context,
                                QString & result, 
                                QString & first, 
                                QString & rest);
  static void TransformDefault (IrcSock * context,
                                QString & result, 
                                QString & first, 
                                QString & rest);

  static void ReceiveNumeric (IrcSock * context,
                         const QString & first,
                         const QString & num,
                         const QString & rest);
  static void ReceivePING (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest);
  static void ReceivePRIVMSG (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest);
  static void ReceiveJOIN (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest);
  static void ReceiveIgnore (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest);
  static void ReceiveDefault (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest);

  bool             initDone;
  Ui_IrcSockMain   mainUi;

  IrcChannelGroup  *channelGroup;

  QTcpSocket     *socket;

  QFile       *logIncoming;
  QFile       *logOutgoing;
  QStringList  scriptLines;
  QTimer      *pingTimer;
  QTimer      *scriptTimer;
  QString      currentChan;
  QString      currentServer;
  bool         waitFirstReceive;
  QString      noNameServer;
  QString      noNameNick;

  QMap <QString, void (*) (IrcSock*, QString &, QString &, QString &)>  
               commandXform;

  QMap <QString, void (*) (IrcSock*, const QString &,
                           const QString &, const QString &)>
               receiveHandler;
  QMap <QString, IrcChannelBox *> 
               channels;

};

} // namespace

#endif
