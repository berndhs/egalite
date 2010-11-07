
#include "enter-string.h"

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

namespace egalite
{

EnterString::EnterString (QWidget *parent)
  :QDialog (parent)
{
  ui.setupUi (this);
  connect (ui.chooseButton, SIGNAL (clicked()),
           this, SLOT (DoChoose ()));
  connect (ui.cancelButton, SIGNAL (clicked()),
           this, SLOT (DoCancel ()));
  hide ();
}

bool
EnterString::Choose  (const QString & title,
                      const QString & label,
                      bool hideSave)
{
  setWindowTitle (title);
  ui.textLabel->setText (label);
  ui.textLine->clear ();
  ui.saveBox->setChecked (false);
  if (hideSave) {
    ui.saveBox->hide ();
  }
  int retval = exec ();
  return (retval != 0);
}

void
EnterString::DoChoose ()
{
  saveIt = ui.saveBox->isChecked ();
  value = ui.textLine->text ();
  haveValue = true;
  accept ();
}

void
EnterString::DoCancel ()
{
  saveIt = false;
  value.clear ();
  haveValue = false;
  reject ();
}


} // namespace
