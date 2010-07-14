#ifndef CHAT_BOX_H
#define CHAT_BOX_H

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

#include "ui_chat-box.h"


class QXmppMessage;

namespace egalite
{

/** \brief ChatBox contains widgets relating to a chat, organized in tabs.
  * These widgets can be chat contents (i.e. the text), or socket widgets,
  * or anything else that is related to a single chat connection.
  */

class ChatBox : public QDialog
{
Q_OBJECT

public:

  ChatBox (QWidget *parent = 0);
  ~ChatBox ();

  void Run ();
  void SetTitle (QString boxtitle);
  void Add (QWidget *widget, QString title);
  int WidgetIndex (QWidget *widget);

public slots:

  void Close ();
  void Incoming (const QXmppMessage &msg);
  void WidgetActivity (QWidget *activeWidget);

signals:

  void HandoffIncoming (const QXmppMessage &msg);

private:

  Ui_ChatBox     ui;

};

} // namespace

#endif
