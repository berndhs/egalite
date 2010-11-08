#ifndef IRC_CHANNEL_BOX_H
#define IRC_CHANNEL_BOX_H

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
#include "ui_irc-channel-box.h"
#include <QStringListModel>

class QMenuBar;
class QMenu;
class QAction;
class QFocusEvent;
class QShowEvent;
class QEvent;
class QUrl;
class QListWidgetItem;

namespace egalite
{

class IrcChannelBox : public QWidget
{
Q_OBJECT

public:

  IrcChannelBox (const QString & name, QWidget *parent=0);

  void Close ();

  void SetTopic (const QString & newTopic);

  void AddNames (const QString & names);
  void AddName (const QString & name);
  void DropName (const QString & name);

  QString Topic () { return topic; }
  QString Name ()  { return name; }

public slots:

  void Incoming (const QString & message);
  void Part ();
  void Float ();
  void Dock ();

private slots:

  void TypingFinished ();
  void Link (const QUrl & url);
  void ClickedUser (QListWidgetItem * item);

protected:

  bool event (QEvent * evt);
 

signals:

  void Outgoing (QString channel, QString message);
  void Active (IrcChannelBox * box);
  void InUse (IrcChannelBox * box);
  void WantFloat (IrcChannelBox * box);
  void WantDock (IrcChannelBox * box);

private:

   void   Connect ();

   Ui_IrcChannelBox    ui;
   QString             name;
   QString             topic;
   QStringList         oldNames;


};

} // namespace

#endif