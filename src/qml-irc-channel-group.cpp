
#include "qml-irc-channel-group.h"

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
#include <QIcon>
#include <QCloseEvent>
#include <QMessageBox>
#include <QMetaObject>
#include <QDeclarativeItem>
#include "irc-abstract-channel.h"

namespace egalite
{

QmlIrcChannelGroup::QmlIrcChannelGroup (QWidget *parent)
  :QWidget (parent)
{
  ui.setupUi (this);
  activeIcon = QIcon (":/ircicons/active.png");
  quietIcon = QIcon (":/ircicons/inactive.png");
  move (250,250);
}

void
QmlIrcChannelGroup::Start ()
{
  ui.qmlChannelView->setSource (
         QUrl::fromLocalFile("qml/IrcChannelGroup.qml"));

  qmlRoot = ui.qmlChannelView->rootObject();
  if (qmlRoot == 0) {
    QMessageBox::critical (this, "Fatal", "QML Load Failure");
    return;
  }
}

void
QmlIrcChannelGroup::AddChannel (IrcAbstractChannel * newchan)
{
  qDebug () << " QmlIrcChannelGroup:: AddChannel " << newchan;
  qDebug () << "                      name     " << newchan->Name();
  QVariant chanObjVar;
  QMetaObject::invokeMethod (qmlRoot, "addChannel",
              Qt::DirectConnection,
              Q_RETURN_ARG (QVariant, chanObjVar));
  qDebug () << "     addChannel returns " << chanObjVar;
  QObject *chanObj = chanObjVar.value<QObject*>();
  QDeclarativeComponent * chanItem = 
             qobject_cast<QDeclarativeComponent*> (chanObj);
  qDebug () << "  added qml item " << chanItem;
  if (chanItem) {
    qDebug () << "   item color / width " << chanItem->property("color")
              << chanItem->property ("width");
  }
}

void
QmlIrcChannelGroup::RemoveChannel (IrcAbstractChannel * chan)
{
}

void
QmlIrcChannelGroup::MarkActive (IrcAbstractChannel * chan, bool active)
{
}

bool
QmlIrcChannelGroup::HaveChannel (IrcAbstractChannel * chan)
{
  return false;
}

void
QmlIrcChannelGroup::Close ()
{
}

void
QmlIrcChannelGroup::Show ()
{
  show ();
}

void
QmlIrcChannelGroup::ShowChannel (IrcAbstractChannel *chan)
{
}


void
QmlIrcChannelGroup::Hide ()
{
  hide ();
}

void
QmlIrcChannelGroup::closeEvent (QCloseEvent *event)
{
  Hide ();
  event->ignore ();
}

} // namespace
