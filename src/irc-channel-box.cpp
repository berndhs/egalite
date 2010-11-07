
#include "irc-channel-box.h"

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

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDebug>
#include <QFocusEvent>
#include <QShowEvent>
#include <QEvent>

namespace egalite
{

IrcChannelBox::IrcChannelBox (const QString & name, QWidget *parent)
  :QWidget (parent),
   name (name)
{
  ui.setupUi (this);
  SetupMenus ();
  Connect ();
  topic = name;
}

void
IrcChannelBox::SetupMenus ()
{
  menuBar = new QMenuBar (this);
  chanMenu = new QMenu ("Channel", this);
  viewMenu = new QMenu ("View",this);
  menuBar->addAction (chanMenu->menuAction());
  menuBar->addAction (viewMenu->menuAction());
  actionPart = new QAction ("Leave", this);
  chanMenu->addAction (actionPart);
  actionDock = new QAction (QIcon (":/ircicons/dock.png"),"Dock", this);
  actionDock->setIconVisibleInMenu (true);
  viewMenu->addAction (actionDock);
  actionFloat = new QAction (QIcon(":/ircicons/float.png"),"Float",this);
  actionFloat->setIconVisibleInMenu (true);
  viewMenu->addAction (actionFloat);
  
}

void
IrcChannelBox::Connect ()
{
  connect (ui.textEnter, SIGNAL (returnPressed()),
           this, SLOT (TypingFinished()));
  connect (ui.sendButton, SIGNAL (clicked ()),
           this, SLOT (TypingFinished ()));
  connect (actionPart, SIGNAL (triggered ()),
           this, SLOT (Part()));
  connect (actionFloat, SIGNAL (triggered ()),
           this, SLOT (Float()));
  connect (actionDock, SIGNAL (triggered ()),
           this, SLOT (Dock()));
}

void
IrcChannelBox::Run ()
{
  show ();
}

void
IrcChannelBox::Close ()
{
  hide ();
}

void
IrcChannelBox::Float ()
{
  emit WantFloat (this);
}

void
IrcChannelBox::Dock ()
{
  emit WantDock (this);
}

void
IrcChannelBox::Part ()
{
  emit Outgoing (name, QString ("/part %1").arg (name));
}

void
IrcChannelBox::Incoming (const QString & message)
{
  ui.chanHistory->append (message);
  emit Active (this);
}

void
IrcChannelBox::TypingFinished ()
{
  QString msg = ui.textEnter->text();
  if (msg.trimmed().length() > 0) {
    emit Outgoing (name, ui.textEnter->text());
  }
  ui.textEnter->clear ();
}


bool
IrcChannelBox::event (QEvent *evt)
{
  QEvent::Type  tipo = evt->type();
  switch (tipo) {
  case QEvent::WindowActivate:
    emit InUse (this);
    break;
  default:
    break;
  }
  return QWidget::event (evt);
}

} // namespace
