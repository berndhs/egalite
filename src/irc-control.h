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
#include "ui_irc-control.h"
#include "config-edit.h"
#include "helpview.h"
#include "irc-float.h"
#include "irc-socket.h"
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

class IrcControl : public QDialog
{
Q_OBJECT

public:

  IrcControl (QWidget *parent=0);

  int   OpenCount ();
  bool  IsRunning () { return isRunning; }

  void  CloseCleanup ();

  void  InChanMsg (IrcSocket * sock,
                   const QString & chan, 
                   const QString & form,
                   const QString & msg);
  void  InUserMsg (IrcSocket * sock,
                   const QString & from, 
                   const QString & to, 
                   const QString & msg);
  void  IncomingCtcpChan (IrcSocket * sock,
                   const QString & from, 
                   const QString & chan,
                   const QString & msg);
  void  IncomingCtcpUser (IrcSocket * sock,
                   const QString & from, 
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
  void TryJoin ();
  void TryPart ();
  void ConnectionReady (IrcSocket * sock);
  void ConnectionGone (IrcSocket * sock);
  void ReceiveLine (IrcSocket * sock, QByteArray line);
  void NickLogin ();
  void ChangedHostName (IrcSocket * sock, QString name);
  void ChanActive (IrcChannelBox * chan);
  void ChanInUse (IrcChannelBox * chan);
  void ChanWantsDock (IrcChannelBox * chan);
  void ChanWantsFloat (IrcChannelBox * chan);
  void ChannelClicked (QListWidgetItem * item);
  void ServerClicked (QTableWidgetItem * item);
  void Send ();

  void Outgoing (QString chan, QString msg);

signals:

  void StatusChange ();

private:

  enum CellType {
       Cell_None = 0,
       Cell_Action = 1,
       Cell_Name = 2,
       Cell_Addr = 3,
       Cell_Port = 4
       };
  enum DataType {
       Data_ConnName = Qt::UserRole+1
       };

  typedef QMap <QString, IrcChannelBox *>   
          ChannelMapType;
  typedef QMap <IrcChannelBox*, IrcFloat*>    
          FloatingMapType;
  typedef QMap <QString, IrcSocket *>
          SocketMapType;
  typedef QMap <QString, void (*) (IrcControl*, IrcSocket *, const QString &,
                           const QString &, const QString &)>
          ReceiveMapType;
  typedef QMap <QString, void (*) (IrcControl*, IrcSocket *, QString &, 
                                   QString &, QString &)>  
          XformMapType;

  void Connect ();
  void AddConnect (IrcSocket * sock);
  void RemoveConnect (IrcSocket * sock);
  void TransformSend (IrcSocket * sock, QString & data);
  IrcSocket * CurrentSock (QTableWidget * table);
  QTableWidgetItem* FindType (QTableWidget * table, int row, int type);
  int  FindRow (QTableWidget * table, const QString & sname);
  void AddChannel (IrcSocket * sock, const QString & chanName);
  void DropChannel (IrcSocket * sock, 
                    const QString & chanName);
  void PartAll (const QString & sockName);
  void SendData (const QString & data);
  void LogRaw (const QString & raw);
  void AddNames (const QString & chanName, 
                 const QString & names);
  void AddName (const QString & chanName, 
                const QString & name);
  void DropName (IrcSocket * sock, 
                  const QString & chanName, 
                  const QString & name,
                  const QString & msg = QString());
  void UserQuit (IrcSocket * sock, 
                 const QString & user, 
                 const QString & msg);
  void SetTopic (IrcSocket * sock, 
                 const QString & chanName, const QString & topic);


  bool                initDone;
  Ui_IrcControl       mainUi;
  bool                isRunning;

  IrcChannelGroup    *dockedChannels;

  SocketMapType       sockets;
  bool                isConnected;

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
