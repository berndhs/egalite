
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
#include "irc-channel-box.h"

namespace egalite
{

IrcChannelGroup::IrcChannelGroup (QWidget *parent)
  :QDialog (parent)
{
  ui.setupUi (this);
  activeIcon = QIcon (":/icons/active.png");
  quiteIcon = QIcon (":/icons/inactive.png");
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
IrcChannelGroup::DropChannel (IrcChannelBox * deadchan)
{
  if (deadchan) {
    int index = ui.tabWidget->indexOf (deadchan);
    if (index >= 0) {
      ui.tabWidget->removeTab (index);
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

} // namespace
