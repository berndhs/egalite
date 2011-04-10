
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
#include "irc-text-browser.h"

namespace egalite
{

QmlIrcChannelGroup::QmlIrcChannelGroup (QWidget *parent)
  :QWidget (parent),
   qmlRoot (0),
   chanLinkPrefix ("chanlink://channel_"),
   debugTimer (this)
{
  ui.setupUi (this);
  activeIcon = QIcon (":/ircicons/active.png");
  quietIcon = QIcon (":/ircicons/inactive.png");
  move (250,250);
  connect (&debugTimer, SIGNAL (timeout()),
           this, SLOT (DebugCheck()));
  debugTimer.start (30*1000);
  QTimer::singleShot (10000, this, SLOT (DebugCheck()));
}

void
QmlIrcChannelGroup::Start ()
{
  ui.qmlChannelView->setSource (
         QUrl("qrc:///qml/IrcChannelGroup.qml"));

  qmlRoot = ui.qmlChannelView->rootObject();
  if (qmlRoot == 0) {
    QMessageBox::critical (this, "Fatal", "QML Load Failure");
    return;
  }
  QDeclarativeEngine * engine = ui.qmlChannelView->engine ();
  if (engine == 0) {
    QMessageBox::critical (this, "Fatal", "No QML Engine");
    return;
  }
  int handle = qmlRegisterType<IrcTextBrowser>
              ("net.sf.egalite",1,0,"IrcTextBrowser");
  qDebug () << " ----------------------- "
             "QmlIrcChannelGroup registered type as " << handle;
  connect (qmlRoot, SIGNAL (selectedChannel (QString)),
           this, SLOT (ClickedChannel (QString)));
  connect (qmlRoot, SIGNAL (changedHeadHeight (int)),
           this, SLOT (HeadHeightChanged (int)));
}

void
QmlIrcChannelGroup::SetChannelList ()
{
  if (qmlRoot) {
    QString chanAnchList;
    int nc = channelList.count ();
    for (int i=0; i<nc; i++) {
       IrcAbstractChannel * chan = channelList.at(i);
       if (chan) {
         chanAnchList.append (
             (chan->IsActive() ? QString ("%1<b>-!</b> ") : QString ("%1 "))
               .arg(ChannelAnchor (chan->Name()))
             );
       }
    }
    QMetaObject::invokeMethod (qmlRoot, "setChannelList",
             Q_ARG (QVariant, chanAnchList));
  }
}

void
QmlIrcChannelGroup::AddChannel (IrcAbstractChannel * newchan)
{
  QVariant chanObjVar;
  QMetaObject::invokeMethod (qmlRoot, "addChannel",
              Qt::DirectConnection,
              Q_RETURN_ARG (QVariant, chanObjVar));
  QObject *chanObj = chanObjVar.value<QObject*>();
  if (chanObj) {
    chanObj->setProperty ("boxLabel",newchan->Name());
    QObject * model = qobject_cast<QObject*>(newchan->userNamesModel());
    QMetaObject::invokeMethod (chanObj, "setModel",
        Q_ARG (QVariant, qVariantFromValue (model)));
    newchan->SetQmlItem (qobject_cast<QDeclarativeItem*>(chanObj));
    channelList << newchan;
    SetTopmostChannel (newchan);
    SetChannelList ();
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
    channelList.removeAll (chan);
    if (chan->Topmost() && !channelList.isEmpty()) {
      SetTopmostChannel (channelList.last());
    }
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
    SetChannelList ();
  }
}

void
QmlIrcChannelGroup::HeadHeightChanged (int newHeight)
{
  int nc = channelList.count();
  for (int i=0; i<nc; i++) {
    channelList.at(i)->HeadHeightChanged (newHeight);
  }
}

void
QmlIrcChannelGroup::MarkActive (IrcAbstractChannel * chan, bool active)
{
  if (chan) {
    chan->SetActive (active);
    SetChannelList ();
  }
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

void
QmlIrcChannelGroup::DebugCheck ()
{
  qDebug () << "QmlIrcChannelGroup :: DebugCheck ";
  int nc = channelList.count();
  for (int i=0; i<nc; i++) {
    IrcAbstractChannel * chan = channelList.at(i);
    qDebug () << " channel " << chan->Name() << " bounds "
              << chan->cookedBoundingRect ();
  }
}

} // namespace
