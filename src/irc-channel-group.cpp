
#include "irc-channel-group.h"

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
#include "irc-channel-box.h"

namespace egalite
{

IrcChannelGroup::IrcChannelGroup (QWidget *parent)
  :QDialog (parent)
{
  ui.setupUi (this);
  activeIcon = QIcon (":/ircicons/active.png");
  quiteIcon = QIcon (":/ircicons/inactive.png");
}

void
IrcChannelGroup::AddChannel (IrcChannelBox * newchan)
{
  if (newchan) {
    QString tabtext = newchan->Name();
    ui.tabWidget->addTab (newchan, tabtext);
  }
}

void
IrcChannelGroup::RemoveChannel (IrcChannelBox * chan)
{
  if (chan) {
    int index = ui.tabWidget->indexOf (chan);
    if (index >= 0) {
      ui.tabWidget->removeTab (index);
    }
    if (ui.tabWidget->count() < 1) {
      hide ();
    }
  }
}

void
IrcChannelGroup::MarkActive (IrcChannelBox * chan, bool active)
{
  int ndx = ui.tabWidget->indexOf (chan);
  if (ndx >= 0) {
    ui.tabWidget->setTabIcon (ndx, (active
                                    ? activeIcon
                                    : quiteIcon));
  }
}

bool
IrcChannelGroup::HaveChannel (IrcChannelBox * chan)
{
  return (ui.tabWidget->indexOf (chan) >= 0);
}

void
IrcChannelGroup::Close ()
{
  ui.tabWidget->clear ();
  hide ();
}

void
IrcChannelGroup::closeEvent (QCloseEvent *event)
{
  int nt = ui.tabWidget->count ();
  for (int t=nt-1; t>=0; t--) {
    QWidget * wid = ui.tabWidget->widget (t);
    IrcChannelBox * chan = dynamic_cast <IrcChannelBox *> (wid);
    if (chan) {
      chan->Part ();
    }
  }
  QDialog::closeEvent (event);
}

} // namespace
