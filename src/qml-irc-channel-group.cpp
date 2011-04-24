
#include "qml-irc-channel-group.h"

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
#include <QIcon>
#include <QCloseEvent>
#include <QMessageBox>
#include <QMetaObject>
#include <QDeclarativeItem>
#include <QRectF>
#include "irc-abstract-channel.h"
#include "irc-text-browser.h"

namespace egalite
{

QmlIrcChannelGroup::QmlIrcChannelGroup (QWidget *parent)
  :QDeclarativeView (parent),
   qmlRoot (0),
   debugTimer (this),
   chanLinkPrefix 
     ("chanlink://channel_"),
   channelMaskActive 
     ("["
         "<span "
           "style=\"color: red\"; font-weight: bold\">"
           " ! "
         "</span>"
       "%1] "),
   channelMaskIdle 
     ("[%1] ")
{
  //ui.setupUi (this);
  activeIcon = QIcon (":/ircicons/active.png");
  quietIcon = QIcon (":/ircicons/inactive.png");
  connect (&debugTimer, SIGNAL (timeout()),
           this, SLOT (DebugCheck()));
  debugTimer.start (30*1000);
  QTimer::singleShot (10000, this, SLOT (DebugCheck()));
}

void
QmlIrcChannelGroup::Start ()
{
  int handle = qmlRegisterType<IrcTextBrowser>
              ("net.sf.egalite",1,0,"IrcTextBrowser");
  setSource (QUrl("qrc:///qml/IrcChannelGroup.qml"));

  qmlRoot = rootObject();
  if (qmlRoot == 0) {
    QMessageBox::critical (0, "Fatal", "QML Load Failure");
    return;
  }
  QRectF scene = sceneRect ();
  qreal width = scene.width();
  qreal height = scene.height();
  qDebug () << __PRETTY_FUNCTION__ << " new size w " << width << " h " << height;
  qmlRoot->setProperty ("width", width);
  qmlRoot->setProperty ("height", height);
  qDebug () << " ----------------------- "
             "QmlIrcChannelGroup registered type as " << handle;
  connect (qmlRoot, SIGNAL (selectedChannel (QString)),
           this, SLOT (ClickedChannel (QString)));
  connect (qmlRoot, SIGNAL (changedHeadHeight (int)),
           this, SLOT (HeadHeightChanged (int)));
  connect (qmlRoot, SIGNAL (hideMe()),
           this, SLOT (hide ()));
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
             (chan->IsActive() ? channelMaskActive 
                               : channelMaskIdle)
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
  if (newchan == 0) {
    return;
  }
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
  newchan->UpdateCooked ();
  newchan->RefreshNames();
  newchan->RefreshTopic();
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
  qDebug () << __PRETTY_FUNCTION__ << chan;
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
QmlIrcChannelGroup::show ()
{
  QDeclarativeView::show ();
}

void
QmlIrcChannelGroup::ShowChannel (IrcAbstractChannel *chan)
{
}


void
QmlIrcChannelGroup::hide ()
{
  QDeclarativeView::hide ();
}

void
QmlIrcChannelGroup::closeEvent (QCloseEvent *event)
{
  hide ();
  event->ignore ();
}

void
QmlIrcChannelGroup::resizeEvent (QResizeEvent *event)
{
  if (event && qmlRoot) {
    QSize size = event->size();
    qreal width = size.width();
    qreal height = size.height();
    qDebug () << __PRETTY_FUNCTION__ << " new size w " << width << " h " << height;
    qmlRoot->setProperty ("width", width);
    qmlRoot->setProperty ("height", height);
  }
  QDeclarativeView::resizeEvent (event);
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
