#ifndef IRC_CHANNEL_BOX_H
#define IRC_CHANNEL_BOX_H

/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2011, Bernd Stramm
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
#include <QList>
#include <QRegExp>
#include <QObject>
#include <QUrl>
#include <QString>
#include <QStringList>
#include <QDeclarativeItem>

#include "user-list-model.h"


namespace egalite
{

class IrcAbstractChannel : public QObject
{
Q_OBJECT

public:

  IrcAbstractChannel (const QString & name, 
                 const QString & sockName,
                 QObject *parent=0);

  ~IrcAbstractChannel ();

  void Close ();

  UserListModel * userNamesModel ();

  void SetTopic (const QString & newTopic);
  void SetHost (const QString & hostName);
  void SetPartMsg (const QString & part)
               { partMsg = part; }

  void AddNames (const QString & names);
  void AddName (const QString & name);
  void DropName (const QString & name, const QString & msg = QString());

  QString Topic () { return topic; }
  QString Name ()  { return chanName; }
  QString Sock () { return sockName; }
  QDeclarativeItem * QmlItem () { return qmlItem; }

  void SetQmlItem (QDeclarativeItem * item);

  void StartWatching (const QRegExp & watch);
  void StopWatching  (const QRegExp & watch);

  bool Topmost ();
  void SetTopmost (bool top);
  void HeadHeightChanged (int newHeight);

public slots:

  void Incoming (const QString & message, const QString & raw = QString());
  void Part ();
  void Float ();
  void Dock ();
  void WhoisData (const QString & otherUser, 
                  const QString & numeric,
                  const QString & data);

private slots:

  void Link (const QUrl & url);
  void ClickedUser (const QString & userName);
  void HideMe ();
  void HideGroup ();
  void HideAll ();
  void CopyClip ();
  void Whois ();
  void UserSend ();
  void UserUp ();
  void UserDown ();

protected:

  bool event (QEvent * evt);
  bool eventFilter (QObject * obj, QEvent *evt);
 

signals:

  void Outgoing (QString channel, QString message);
  void OutRaw (QString sockName, QString data);
  void WantWhois (QString channel, QString otherUser, bool want);
  void Active (IrcAbstractChannel * box);
  void InUse (IrcAbstractChannel * box);
  void WantFloat (IrcAbstractChannel * box);
  void WantDock (IrcAbstractChannel * box);
  void HideAllChannels ();
  void HideDock ();
  void HideChannel (IrcAbstractChannel * box);
  void WatchAlert (QString pattern, QString line);

private:

  void   Connect ();
  void   AppendSmall (QString & log, const QString & line);
  void   UpdateCooked ();
  void   CheckWatch (const QString & data);

  static bool Less (const QString & left, const QString & right);

  QString             chanName;
  QString             sockName;
  QString             partMsg;
  QString             topic;
  QStringList         oldNames;
  QString             queryUser;
  QString             clipSave;
  QString             cookedLog;

  QList <QRegExp>     watchList;

  QStringList         history;
  QString             historyBottom;
  int                 historyIndex;
  UserListModel       namesModel;
  QDeclarativeItem   *qmlItem;
  bool                topmost;

};


} // namespace

#endif