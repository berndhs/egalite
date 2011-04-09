
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
  :QWidget (parent),
   qmlRoot (0),
   chanLinkPrefix ("chanlink://channel_")
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
  connect (qmlRoot, SIGNAL (selectedChannel (QString)),
           this, SLOT (ClickedChannel (QString)));
}

void
QmlIrcChannelGroup::SetChannelList ()
{
  if (qmlRoot) {
    QMetaObject::invokeMethod (qmlRoot, "setChannelList",
             Q_ARG (QVariant, channelAnchorList.join (" ")));
  }
}

void
QmlIrcChannelGroup::AddChannel (IrcAbstractChannel * newchan)
{
  QVariant chanObjVar;
  QMetaObject::invokeMethod (qmlRoot, "addChannel",
              Qt::DirectConnection,
              Q_RETURN_ARG (QVariant, chanObjVar));
  qDebug () << "     addChannel returns " << chanObjVar;
  QObject *chanObj = chanObjVar.value<QObject*>();
  if (chanObj) {
    chanObj->setProperty ("color",QString("green"));
    chanObj->setProperty ("boxLabel",newchan->Name());
    QObject * model = qobject_cast<QObject*>(newchan->userNamesModel());
   qDebug () << " model pointer " << model;
   qDebug () << " model pointer " <<qVariantFromValue(model);
    QMetaObject::invokeMethod (chanObj, "setModel",
        Q_ARG (QVariant, qVariantFromValue (model)));
    newchan->SetQmlItem (qobject_cast<QDeclarativeItem*>(chanObj));
    channelAnchorList << ChannelAnchor (newchan->Name());
    channelList << newchan;
    SetTopmostChannel (newchan);
    SetChannelList ();
    qDebug () << "  Channel List " << channelAnchorList;
  }
}

QString
QmlIrcChannelGroup::ChannelAnchor (const QString & name)
{
  return QString("<a href=\"%2%1\">%1</a>")
         .arg(name).arg (chanLinkPrefix);
}

void
QmlIrcChannelGroup::RemoveChannel (IrcAbstractChannel * chan)
{
  if (chan) {
    channelAnchorList.removeAll (ChannelAnchor (chan->Name()));
    channelList.removeAll (chan);
    SetChannelList ();
  }
}

void
QmlIrcChannelGroup::SetTopmostChannel (IrcAbstractChannel * topChan)
{
  int nc = channelList.count();
  for (int i=0; i<nc; i++) {
    IrcAbstractChannel *chan = channelList.at(i);
    chan->SetTopmost (chan == topChan);
  }
}

void
QmlIrcChannelGroup::SetTopmostChannel (const QString & topName)
{
  int nc = channelList.count();
  for (int i=0; i<nc; i++) {
    IrcAbstractChannel *chan = channelList.at(i);
    chan->SetTopmost (chan->Name() == topName);
  }
  
}

void
QmlIrcChannelGroup::ClickedChannel (QString link)
{
  if (link.startsWith (chanLinkPrefix)) {
    QString name (link);
    name.remove (0,chanLinkPrefix.length());
    SetTopmostChannel (name);
  }
}

void
QmlIrcChannelGroup::MarkActive (IrcAbstractChannel * chan, bool active)
{
}

bool
QmlIrcChannelGroup::HaveChannel (IrcAbstractChannel * chan)
{
  if (chan) {
    return channelList.contains (chan);
  }
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
