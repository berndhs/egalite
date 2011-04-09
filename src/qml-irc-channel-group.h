#ifndef IRC_CHANNEL_GROUP_H
#define ITC_CHANNEL_GROUP_H

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
#include "ui_irc-qml-channel-group.h"
#include <QIcon>
#include <QWidget>
#include <QGraphicsObject>
#include <QDeclarativeItem>
#include <QStringList>
#include <QList>

class QCloseEvent;

namespace egalite
{

class IrcAbstractChannel;

class QmlIrcChannelGroup : public QWidget
{
Q_OBJECT

public:

  QmlIrcChannelGroup (QWidget *parent=0);

  void Start ();

  void AddChannel (IrcAbstractChannel * chan);
  void RemoveChannel (IrcAbstractChannel * chan);
  void MarkActive (IrcAbstractChannel * chan, bool active);
  bool HaveChannel (IrcAbstractChannel * chan);
  void ShowChannel (IrcAbstractChannel * chan);
 
  void Close ();

public slots:

  void Show ();
  void Hide ();

private slots:

  void ClickedChannel (QString link);
  void HeadHeightChanged (int newHeight);

protected:

  void closeEvent (QCloseEvent *event);

private:

  QString  ChannelAnchor (const QString & name);
  void     SetChannelList ();
  void     SetTopmostChannel (IrcAbstractChannel * topChan);
  void     SetTopmostChannel (const QString & topName);

  Ui_IrcQmlChannelGroup   ui;
  QIcon                   activeIcon;
  QIcon                   quietIcon;
  QGraphicsObject        *qmlRoot;
  QList <IrcAbstractChannel*>  channelList;
  QStringList             channelAnchorList;
  QString                 chanLinkPrefix;

};

} // namespace

#endif
