#ifndef IRC_CHANNEL_GROUP_H
#define ITC_CHANNEL_GROUP_H

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
#include "ui_irc-channel-group.h"
#include <QIcon>
#include <QSize>
#include <QPoint>

class QCloseEvent;

namespace egalite
{

class IrcChannelBox;

class IrcChannelGroup : public QDialog
{
Q_OBJECT

public:

  IrcChannelGroup (QWidget *parent=0);

  void AddChannel (IrcChannelBox * chan);
  void RemoveChannel (IrcChannelBox * chan);
  void MarkActive (IrcChannelBox * chan, bool active);
  bool HaveChannel (IrcChannelBox * chan);
  void ShowChannel (IrcChannelBox * chan);
 
  void Close ();

public slots:

  void Show ();
  void Hide ();

protected:

  void closeEvent (QCloseEvent *event);

private:

  Ui_IrcChannelGroup    ui;
  QIcon                 activeIcon;
  QIcon                 quietIcon;
  QSize                 oldSize;
  QPoint                oldPos;
  bool                  hidSelf;

};

} // namespace

#endif
