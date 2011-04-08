
#include "irc-abstract-channel.h"

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
#include <QtAlgorithms>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QApplication>
#include <QClipboard>
#include "link-mangle.h"

namespace egalite
{

IrcAbstractChannel::IrcAbstractChannel (const QString & name,
                              const QString & sock,
                              QObject *parent)
  :QObject (parent),
   chanName (name),
   sockName (sock),
   historyIndex (-1),
   namesModel (this),
   qmlItem (0)
{
  Connect ();
  SetTopic (tr("Channel %1").arg (chanName));
}

UserListModel *
IrcAbstractChannel::userNamesModel ()
{
  return & namesModel;
}

void
IrcAbstractChannel::Connect ()
{
}
void
IrcAbstractChannel::CopyClip ()
{
  QClipboard * clip = QApplication::clipboard ();
  if (clip) {
    clip->setText (clipSave);
  }
}

void
IrcAbstractChannel::SetHost (const QString & hostName)
{
  //ui.serverLabel->setText (hostName);
}

void
IrcAbstractChannel::StartWatching (const QRegExp & watch)
{
  if (!watchList.contains (watch)) {
    watchList.append (watch);
  }
}

void
IrcAbstractChannel::StopWatching (const QRegExp & watch)
{
  watchList.removeAll (watch);
}

void
IrcAbstractChannel::CheckWatch (const QString & data)
{
  QList<QRegExp>::iterator lit;
  bool notSeen (true);
  for (lit=watchList.begin(); notSeen && lit != watchList.end(); lit++) {
    QRegExp rX = *lit;
    if (rX.indexIn (data,0) >= 0) {
      emit WatchAlert (chanName, data);
      notSeen = false;
    }
  }
}

void
IrcAbstractChannel::Close ()
{
}

void
IrcAbstractChannel::Float ()
{
  emit WantFloat (this);
}

void
IrcAbstractChannel::Dock ()
{
  emit WantDock (this);
}

void
IrcAbstractChannel::Part ()
{
  emit Outgoing (chanName, QString ("/part %1 :%2")
                                    .arg (chanName)
                                    .arg (partMsg));
}

void
IrcAbstractChannel::Incoming (const QString & message,
                         const QString & raw)
{
  QString cooked = LinkMangle::Anchorize (message, 
                         LinkMangle::HttpExp(),
                         LinkMangle::HttpAnchor);
qDebug () << " cooked message " << cooked;
  QDateTime now = QDateTime::currentDateTime ();
  QString smalldate ("<span style=\"font-size:small\">"
                     "%1</span> %2");
  #if 0
  ui.chanHistory->append (smalldate
                          .arg (now.toString ("hh:mm:ss"))
                          .arg (cooked));
  #endif
  CheckWatch (raw.length() > 0 ? raw : message);
  emit Active (this);
}

void
IrcAbstractChannel::SetTopic (const QString & newTopic)
{
  topic = newTopic;
  QString cooked = LinkMangle::Anchorize (topic, 
                         LinkMangle::HttpExp(),
                         LinkMangle::HttpAnchor);
  #if 0
  ui.chanTopic->setHtml (cooked);
  #endif
}

void
IrcAbstractChannel::AddNames (const QString & names)
{
  QStringList newNames = names.split (QRegExp ("(\\s+)"));
qDebug () << " IrcAbstractChannel :: AddNames " << newNames;
  oldNames.append (newNames);
  oldNames.removeDuplicates ();
  qSort (oldNames.begin(), oldNames.end(), IrcAbstractChannel::Less);
  namesModel.setStringList (oldNames);
  if (qmlItem) {
    qmlItem->setProperty ("userListCounter",
      tr("%1 Users").arg (oldNames.size()));
  }
}

void
IrcAbstractChannel::AddName (const QString & name)
{
qDebug () << " IrcAbstractChannel :: AddName " << name;
  if (oldNames.contains (name)) {
    return;
  }
  oldNames.append (name);
  qSort (oldNames.begin(), oldNames.end(), IrcAbstractChannel::Less);
  namesModel.setStringList (oldNames);
  if (qmlItem) {
    qmlItem->setProperty ("userListCounter",
      tr("%1 Users").arg (oldNames.size()));
  }
#if 0
  AppendSmall (ui.chanHistory, tr(" Enter: -&gt; %1").arg(name));
#endif
}

void
IrcAbstractChannel::DropName (const QString & name, const QString & msg)
{
#if 0
  if (!oldNames.contains (name)) {  // not mine - don't care
    return;
  }
  oldNames.removeAll (name);
  ui.chanUsers->clear();
  qSort (oldNames.begin(), oldNames.end(), IrcAbstractChannel::Less);
  ui.chanUsers->addItems (oldNames);
  ui.rawLog->append (tr("Exit: %1 %2").arg(name). arg (msg));
  ui.usersLabel->setText (tr("%1 Users").arg (oldNames.size()));
  AppendSmall (ui.chanHistory, tr(" Exit: &lt;- %1 %2").arg(name).arg(msg));
#endif
}

void
IrcAbstractChannel::TypingFinished ()
{
#if 0
  QString msg = ui.textEnter->text();
  if (msg.trimmed().length() > 0) {
    emit Outgoing (chanName, msg);
    ui.rawLog->append (msg);
    ui.textEnter->clear ();
    history.append (msg);
    history.removeDuplicates ();
    historyIndex = history.size();
  }
#endif
}

void
IrcAbstractChannel::Link (const QUrl & url)
{
#if 0
  if (url.scheme () == "ircsender") {
    QString msg = ui.textEnter->text ();
    msg.append (url.userName());
    msg.append (": ");
    ui.textEnter->setText (msg);
  } else {
    QDesktopServices::openUrl (url);
  }
#endif
}

void
IrcAbstractChannel::AppendSmall (const QString & line)
{
#if 0
  log->append (QString ("<span style=\"font-size: small\">%1</span>")
                       .arg (line));
#endif
}

void
IrcAbstractChannel::ClickedUser (const QString & userName)
{
#if 0
  qDebug () << " clicked on user " << item->text ();
  if (item) {
    queryUser = item->text ();
    if (queryUser.startsWith (QChar ('@'))
        || queryUser.startsWith (QChar ('+'))) {
      queryUser.remove (0,1);
    }
    QPoint here = QCursor::pos();
    userMenu->exec (here);
  }
#endif
}

void
IrcAbstractChannel::Whois ()
{
#if 0
  QStringList head;
  head << tr ("Whois info on");
  head << queryUser;
  ui.userinfoWidget->clear ();
  QTreeWidgetItem * item = new QTreeWidgetItem (head);
  ui.userinfoWidget->addTopLevelItem (item);
  ui.userinfoWidget->show ();
  QString query ("WHOIS %1");
  emit WantWhois (chanName, queryUser, true);
  emit OutRaw (sockName, query.arg (queryUser));
#endif
}

void
IrcAbstractChannel::WhoisData (const QString &otherUser,
                          const QString &numeric,
                          const QString &data)
{
#if 0
  QStringList newinfo;
  QString theData (data.trimmed());
  if (numeric == "318") {
    emit WantWhois (chanName, otherUser, false);
    return;
  } else if (numeric == "311") { 
    newinfo << tr("User Name ") << theData;
  } else if (numeric == "312") { 
    newinfo << tr("Server") << theData;
  } else if (numeric == "313") {
    newinfo << tr("Op") << theData;
  } else if (numeric == "319") {
    if (theData.startsWith (QChar (':'))) {
      theData.remove (0,1);
    }
    newinfo << tr("Channels") << theData;
  }
  QTreeWidgetItem * item = new QTreeWidgetItem (newinfo);
  ui.userinfoWidget->addTopLevelItem (item);
  ui.rawLog->append (QString ("WHOIS %1 %2").arg (otherUser).arg (data));
#endif
}

void
IrcAbstractChannel::HideMe ()
{
  emit HideChannel (this);
}

void
IrcAbstractChannel::HideGroup ()
{
  emit HideDock ();
}

void
IrcAbstractChannel::HideAll ()
{
  emit HideAllChannels ();
}
bool
IrcAbstractChannel::eventFilter (QObject * obj, QEvent * evt)
{
#if 0
  if (obj == ui.textEnter) {
    if (evt->type() == QEvent::KeyPress) {
      return DoHistory (ui.textEnter, history, evt, 
                        historyIndex, historyBottom);
    }
  }
#endif
  return QObject::eventFilter (obj, evt);
}

bool
IrcAbstractChannel::event (QEvent *evt)
{
  QEvent::Type  tipo = evt->type();
  switch (tipo) {
  case QEvent::WindowActivate:
    emit InUse (this);
    break;
  default:
    break;
  }
  return QObject::event (evt);
}

bool
IrcAbstractChannel::Less (const  QString & left, const QString & right)
{
  return left.toLower() < right.toLower ();
}

} // namespace
