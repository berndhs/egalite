
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

#include "add-listener.h"

namespace egalite
{

AddListener::AddListener (QWidget *parent)
  :QDialog (parent),
   portPicked (0)
{
  ui.setupUi (this);
  connect (ui.startButton, SIGNAL (clicked()), this, SLOT (Start()));
  connect (ui.cancelButton, SIGNAL (clicked()), this, SLOT (Cancel()));
}

int
AddListener::SelectStart (const QString & identity)
{
  ui.identityEdit->setText (identity);
  ui.addressEdit->clear ();
  addressPicked.clear();
  ui.portEdit->clear ();
  portPicked = 0;
  return exec ();
}

void
AddListener::Start ()
{
  addressPicked = ui.addressEdit->text();
  portPicked = ui.portEdit->text().toInt();
  done (1);
}

void
AddListener::Cancel ()
{
  addressPicked.clear ();
  portPicked = 0;
  done (0);
}

QString
AddListener::Address ()
{
  return addressPicked;
}

int
AddListener::Port ()
{
  return portPicked;
}

} // namespace

