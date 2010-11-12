
#include "irc-float.h"

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

#include <QCloseEvent>

namespace egalite
{

IrcFloat::IrcFloat (QWidget *parent)
  :QDialog (parent),
   chanBox (0)
{
  ui.setupUi (this);
}

void
IrcFloat::Hide ()
{
  hide ();
}

void
IrcFloat::Show ()
{
  show ();
}

void
IrcFloat::AddChannel (IrcChannelBox *chan)
{
  if (chanBox != 0) {
    return;
  }
  chanBox = chan;
  ui.mainLayout->addWidget (chanBox, 0,0,1,1);
  setWindowTitle (chan->Name());
  connect (chanBox, SIGNAL (HideMe()), this, SLOT (Hide()));
  chan->show ();
}

void
IrcFloat::RemoveChannel (IrcChannelBox *chan)
{
  if (chanBox == chan) {
    disconnect (chanBox, 0, this, 0);
    chanBox = 0;
  }
}

void
IrcFloat::closeEvent (QCloseEvent *event)
{
  if (chanBox) {
    chanBox->Part ();
  }
  QDialog::closeEvent (event);
}

} // namespace
