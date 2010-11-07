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

class QMenuBar;
class QMenu;
class QAction;
class QFocusEvent;
class QShowEvent;
class QEvent;

namespace egalite
{

class IrcChannelBox : public QWidget
{
Q_OBJECT

public:

  IrcChannelBox (const QString & name, QWidget *parent=0);

  void Run ();
  void Close ();

  void SetTopic (const QString & newTopic);

  QString Topic () { return topic; }
  QString Name ()  { return name; }

public slots:

  void Incoming (const QString & message);
  void Part ();
  void Float ();
  void Dock ();

private slots:

  void TypingFinished ();

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
   void   SetupMenus ();

   Ui_IrcChannelBox    ui;
   QMenuBar           *menuBar;
   QMenu              *chanMenu;
   QMenu              *viewMenu;
   QAction            *actionPart;
   QAction            *actionDock;
   QAction            *actionFloat;
   QString             name;
   QString             topic;

};

} // namespace

#endif