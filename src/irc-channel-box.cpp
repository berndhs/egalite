
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
#include <QUrl>
#include <QDesktopServices>
#include <QListWidgetItem>
#include "link-mangle.h"

namespace egalite
{

IrcChannelBox::IrcChannelBox (const QString & name, QWidget *parent)
  :QWidget (parent),
   name (name)
{
  ui.setupUi (this);
  ui.chanTopic->setOpenLinks (false);
  ui.chanHistory->setOpenLinks (false);
  ui.partButton->setAutoDefault (false);
  ui.partButton->setDefault (false);
  ui.floatButton->setAutoDefault (false);
  ui.floatButton->setDefault (false);
  ui.dockButton->setAutoDefault (false);
  ui.dockButton->setDefault (false);
  Connect ();
  SetTopic (tr("Channel %1").arg (name));
  show ();
}


void
IrcChannelBox::Connect ()
{
  connect (ui.textEnter, SIGNAL (returnPressed()),
           this, SLOT (TypingFinished()));
  connect (ui.sendButton, SIGNAL (clicked ()),
           this, SLOT (TypingFinished ()));
  connect (ui.partButton, SIGNAL (clicked ()),
           this, SLOT (Part()));
  connect (ui.floatButton, SIGNAL (clicked ()),
           this, SLOT (Float()));
  connect (ui.dockButton, SIGNAL (clicked ()),
           this, SLOT (Dock()));
  connect (ui.chanTopic, SIGNAL (anchorClicked (const QUrl &)),
           this, SLOT (Link (const QUrl &)));
  connect (ui.chanHistory, SIGNAL (anchorClicked (const QUrl &)),
           this, SLOT (Link (const QUrl &)));
  connect (ui.chanUsers, SIGNAL (itemClicked (QListWidgetItem *)),
           this, SLOT (ClickedUser (QListWidgetItem *)));
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
  QString cooked = LinkMangle::Anchorize (message, 
                         LinkMangle::HttpExp(),
                         LinkMangle::HttpAnchor);
qDebug () << " cooked message " << cooked;
  ui.chanHistory->append (cooked);
  emit Active (this);
}

void
IrcChannelBox::SetTopic (const QString & newTopic)
{
  topic = newTopic;
  QString cooked = LinkMangle::Anchorize (topic, 
                         LinkMangle::HttpExp(),
                         LinkMangle::HttpAnchor);
  ui.chanTopic->setHtml (cooked);
}

void
IrcChannelBox::AddNames (const QString & names)
{
  QStringList newNames = names.split (QRegExp ("(\\s+)"));
  oldNames.append (newNames);
  oldNames.removeDuplicates ();
  ui.chanUsers->clear();
  ui.chanUsers->addItems (oldNames);
  ui.chanUsers->sortItems();
}

void
IrcChannelBox::AddName (const QString & name)
{
  if (!oldNames.contains (name)) {
    oldNames.append (name);
  }
  ui.chanUsers->clear();
  ui.chanUsers->addItems (oldNames);
  ui.chanUsers->sortItems();
}

void
IrcChannelBox::DropName (const QString & name)
{
  oldNames.removeAll (name);
  ui.chanUsers->clear();
  ui.chanUsers->addItems (oldNames);
  ui.chanUsers->sortItems();
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

void
IrcChannelBox::Link (const QUrl & url)
{
  if (url.scheme () == "ircsender") {
    QString msg = ui.textEnter->text ();
    msg.append (url.authority());
    msg.append (":");
    ui.textEnter->setText (msg);
  } else {
    QDesktopServices::openUrl (url);
  }
}

void
IrcChannelBox::ClickedUser (QListWidgetItem *item)
{
  qDebug () << " clicked on user " << item->text ();
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
