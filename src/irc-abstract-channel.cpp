
#include "irc-abstract-channel.h"

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
   qmlItem (0),
   topmost (false),
   active (false)
{
  Connect ();
  SetTopic (tr("Channel %1").arg (chanName));
}

IrcAbstractChannel::~IrcAbstractChannel ()
{
  if (qmlItem) {
    QMetaObject::invokeMethod (qmlItem, "deallocateSelf");
  }
}

void
IrcAbstractChannel::SetQmlItem (QDeclarativeItem * item)
{
  if (qmlItem) {
    disconnect (qmlItem, 0,0,0);
  }
  qmlItem = item;
  connect (qmlItem, SIGNAL (userSend ()), this, SLOT (UserSend()));
  connect (qmlItem, SIGNAL (userUp()), this, SLOT (UserUp()));
  connect (qmlItem, SIGNAL (userDown()), this, SLOT (UserDown ()));
  connect (qmlItem, SIGNAL (activatedLink(const QString &)),
           this, SLOT (ActivatedCookedLink(const QString &)));
}

QRectF
IrcAbstractChannel::cookedBoundingRect () const
{
  if (qmlItem) {
    QVariant rectVar;
    QMetaObject::invokeMethod (qmlItem, "cookedBoundingRect",
             Q_RETURN_ARG (QVariant, rectVar));
    QRectF br = rectVar.toRectF();
    return br;
  }
  return QRectF ();
}
 
void
IrcAbstractChannel::SetTopmost (bool top)
{
  if (qmlItem) {
    topmost = top;
    qmlItem->setProperty ("z", top ? 1 : -1);
    if (topmost) {
      active = false;
    }
  }
}

void
IrcAbstractChannel::HeadHeightChanged (int newHeight)
{
  qDebug () << "IrcAbstractChannel :: HeadHeightChanged " << newHeight;
  if (qmlItem) {
    qmlItem->setProperty ("parentHeightReserve", newHeight);
  }
}

bool
IrcAbstractChannel::Topmost ()
{
  return topmost;
}

bool
IrcAbstractChannel::IsActive ()
{
  return active;
}

void
IrcAbstractChannel::SetActive (bool a)
{
  active = a;
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
  cookedLog.append (smalldate
                          .arg (now.toString ("hh:mm:ss"))
                          .arg (cooked));
  cookedLog.append ("<br>\n");
  UpdateCooked ();
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
  if (qmlItem) {
    qmlItem->setProperty ("channelTopic", cooked);
  }
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
  AppendSmall (cookedLog, tr(" Enter: -&gt; %1").arg(name));
  UpdateCooked ();
}

void
IrcAbstractChannel::DropName (const QString & name, const QString & msg)
{
  if (!oldNames.contains (name)) {  // not mine - don't care
    return;
  }
  oldNames.removeAll (name);
  qSort (oldNames.begin(), oldNames.end(), IrcAbstractChannel::Less);
  namesModel.setStringList (oldNames);
  if (qmlItem) {
    qmlItem->setProperty ("userListCounter",
      tr("%1 Users").arg (oldNames.size()));
  }
  AppendSmall (cookedLog, tr(" Exit: &lt;- %1 %2").arg(name).arg(msg));
  UpdateCooked ();
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
IrcAbstractChannel::ActivatedCookedLink (const QString & link)
{
  QUrl url (link);
  if (url.scheme () == "ircsender") {
    if (qmlItem) {
      QVariant msgVar;
      QMetaObject::invokeMethod (qmlItem, "userData",
         Q_RETURN_ARG (QVariant, msgVar));
      QString msg (msgVar.toString());
      msg.append (url.userName());
      msg.append (": ");
      QMetaObject::invokeMethod (qmlItem, "writeUserData",
         Q_ARG (QVariant, msg));
    }
  } else {
    QDesktopServices::openUrl (url);
  }
}

void
IrcAbstractChannel::AppendSmall (QString & log, const QString & line)
{
  log.append (QString ("<span style=\"font-size: small\">%1</span><br>\n")
                       .arg (line));
}

void
IrcAbstractChannel::UpdateCooked ()
{
  if (qmlItem) {
    QMetaObject::invokeMethod (qmlItem, "setCookedLog",
            Q_ARG (QVariant, cookedLog));
  }
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

void
IrcAbstractChannel::UserSend ()
{
  qDebug () << " User sending " ;
  if (qmlItem) {
    QVariant userData;
    QMetaObject::invokeMethod (qmlItem, "userData",
        Q_RETURN_ARG (QVariant, userData));
    QString data = userData.toString();
    qDebug () << "   user data " << data;
    if (data.trimmed().length() > 0) {
      QMetaObject::invokeMethod (qmlItem, "clearUserData");
      emit Outgoing (chanName, data);
      history.append (data);
      history.removeDuplicates ();
      historyIndex = history.size();
    }
  }
}

void
IrcAbstractChannel::UserUp ()
{
  qDebug () << " User Up arrow ";
}

void
IrcAbstractChannel::UserDown ()
{
  qDebug () << " User Down arrow ";
}

} // namespace
