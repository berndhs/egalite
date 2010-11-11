
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
#include <QDateTime>
#include "link-mangle.h"

namespace egalite
{

IrcChannelBox::IrcChannelBox (const QString & name,
                              const QString & sock,
                              QWidget *parent)
  :QWidget (parent),
   chanName (name),
   sockName (sock)
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
  SetTopic (tr("Channel %1").arg (chanName));
  show ();
  BalanceSplitter ();
}

void
IrcChannelBox::BalanceSplitter ()
{
  QList <int> widList = ui.horizonSplitter->sizes();
  int numParts = widList.size();
  int totWid (0);
  for (int i=0; i<numParts; i++) {
    totWid += widList[i];
  }
  int firstWid = (totWid * 80) / 100;
  widList[0] = firstWid;
  widList[1] = totWid - firstWid;
  for (int i=2; i< numParts; i++) {
    widList[i] = 0;
  }
qDebug () << " resized width list " << widList;
  ui.horizonSplitter->setSizes (widList);
  update ();
qDebug () << " resulting width list " << ui.horizonSplitter->sizes();
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
  emit Outgoing (chanName, QString ("/part %1").arg (chanName));
}

void
IrcChannelBox::Incoming (const QString & message)
{
  QString cooked = LinkMangle::Anchorize (message, 
                         LinkMangle::HttpExp(),
                         LinkMangle::HttpAnchor);
qDebug () << " cooked message " << cooked;
  QDateTime now = QDateTime::currentDateTime ();
  QString smalldate ("<span style=\"font-size:small\">"
                     "%1</span> %2");
  ui.chanHistory->append (smalldate
                          .arg (now.toString ("hh:mm:ss"))
                          .arg (cooked));
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
  QStringList::iterator sit;
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
  ui.rawLog->append (tr("Enter: %1").arg(name));
  AppendSmall (ui.chanHistory, tr(" Enter: -&gt; %1").arg(name));
}

void
IrcChannelBox::DropName (const QString & name)
{
  oldNames.removeAll (name);
  ui.chanUsers->clear();
  ui.chanUsers->addItems (oldNames);
  ui.chanUsers->sortItems();
  ui.rawLog->append (tr("Exit: %1").arg(name));
  AppendSmall (ui.chanHistory, tr(" Exit: &lt;- %1").arg(name));
}

void
IrcChannelBox::TypingFinished ()
{
  QString msg = ui.textEnter->text();
  if (msg.trimmed().length() > 0) {
    emit Outgoing (chanName, msg);
  }
  ui.rawLog->append (msg);
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
IrcChannelBox::AppendSmall (QTextBrowser * log, const QString & line)
{
  log->append (QString ("<span style=\"font-size: small\">%1</span>")
                       .arg (line));
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
