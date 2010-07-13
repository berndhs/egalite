
/** \brief ChatBox class contains a direct chat and the status for it.
*/

#include "chat-box.h"


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

#include <QDebug>
#include <QXmppMessage.h>

namespace egalite
{

ChatBox::ChatBox (QWidget *parent)
  :QDialog (parent)
{
  ui.setupUi (this);
}

ChatBox::~ChatBox ()
{
  qDebug () << " deallocated chat box " << this;
}

void
ChatBox::Add (QWidget *widget, QString title)
{
  ui.tabWidget->addTab (widget, title);
}

void
ChatBox::Run ()
{
qDebug () << " chat box " << this << "running ";
  show ();
}

void
ChatBox::Close ()
{
  qDebug () << " close chat box called";
  int howmany = ui.tabWidget->count();
  for (int w = 0; w < howmany; w++) {
    ui.tabWidget->widget(w)->close ();
  }
  close ();
}

bool
ChatBox::HaveWidget (QWidget *widget)
{
  int howmany = ui.tabWidget->count ();
  for (int w=0; w < howmany; w++) {
    if (ui.tabWidget->widget (w) == widget) {
      return true;
    }
  }
  return false;
}

void
ChatBox::SetTitle (QString boxtitle)
{
  this->setWindowTitle (boxtitle);
}

void
ChatBox::Incoming (const QXmppMessage & msg)
{
  emit HandoffIncoming (msg);
}

} // namespace
