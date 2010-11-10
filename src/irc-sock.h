#ifndef IRC_SOCK_H
#define IRC_SOCK_H

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
#include "irc-float.h"
#include <QTcpSocket>
#include <QFile>
#include <QStringList>
#include <QMap>
#include <QList>
#include <QSize>
#include <QPoint>

using namespace deliberate;

class QListWidgetItem;

namespace egalite 
{

class IrcChannelBox;
class IrcChannelGroup;

class IrcSock : public QDialog
{
Q_OBJECT

public:

  IrcSock (QWidget *parent=0);

  int   OpenCount ();
  bool  IsRunning () { return isRunning; }

  void  CloseCleanup ();

  void  InChanMsg (const QString & chan, 
                   const QString & form,
                   const QString & msg);
  void  InUserMsg (const QString & from, 
                   const QString & to, 
                   const QString & msg);

public slots:

  bool  Run ();
  void  Show ();
  void  Hide ();
  void  ShowGroup ();
  void  HideGroup ();

private slots:

  void Exit ();
  void Exiting ();
  void TryConnect ();
  void TryDisconnect ();
  void SockDisconnect ();
  void TryJoin ();
  void TryPart ();
  void ConnectionReady ();
  void ConnectionGone ();
  void Receive ();
  void DidSend (qint64 bytes);
  void FakeLogin ();
  void ChanActive (IrcChannelBox * chan);
  void ChanInUse (IrcChannelBox * chan);
  void ChanWantsDock (IrcChannelBox * chan);
  void ChanWantsFloat (IrcChannelBox * chan);
  void ChannelClicked (QListWidgetItem * item);

  void Outgoing (QString chan, QString msg);
  
  void RollScript ();
  void SendScriptHead ();
  void Send ();
  void Send (QString data);
  void SendPing ();

  void SockError (QAbstractSocket::SocketError err);

signals:

  void StatusChange ();

private:

  typedef QMap <QString, void (*) (IrcSock*, QString &, 
                                   QString &, QString &)>  
          XformMapType;
  typedef QMap <QString, void (*) (IrcSock*, const QString &,
                           const QString &, const QString &)>
          ReceiveMapType;
  typedef QMap <QString, IrcChannelBox *>   
          ChannelMapType;
  typedef QMap <IrcChannelBox*, IrcFloat*>    
          FloatingMapType;

  void Connect ();
  void AddChannel (const QString & chanName);
  void DropChannel (const QString & chanName);
  void SendData (const QString & data);
  void LogRaw (const QString & raw);
  void ReceiveLine (const QByteArray & line);
  void AddNames (const QString & chanName, const QString & names);
  void AddName (const QString & chanName, const QString & name);
  void DropName (const QString & chanName, const QString & name);
  void SetTopic (const QString & chanName, const QString & topic);


  bool                initDone;
  Ui_IrcSockMain      mainUi;
  bool                isRunning;

  IrcChannelGroup    *dockedChannels;

  QTcpSocket         *socket;
  bool                isConnected;
  QByteArray          lineData;

  QStringList         scriptLines;
  QTimer             *pingTimer;
  QTimer             *scriptTimer;
  QString             currentChan;
  QString             currentServer;
  QString             currentUser;
  bool                waitFirstReceive;
  QString             noNameServer;
  QString             noNameNick;
  QString             noNameChannel;
  QSize               oldSize;
  QPoint              oldPos;
  bool                hidSelf;

  XformMapType        commandXform;
  ReceiveMapType      receiveHandler;

  ChannelMapType      channels;
  FloatingMapType     floatingChannels;

  QList <QString>     ignoreSources;

  friend class IrcSockStatic;

};

} // namespace

#endif
