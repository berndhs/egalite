
/** pick string */

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

#include "pick-string.h"
#include <QStandardItem>

namespace egalite
{

PickString::PickString (QWidget *parent)
  :QDialog (parent)
{
  ui.setupUi (this);
  connect (ui.pickButton, SIGNAL (clicked()), this, SLOT (PickCurrent ()));
  connect (ui.cancelButton, SIGNAL (clicked()), this, SLOT (PickNothing ()));
  connect (ui.stringList, SIGNAL (doubleClicked(const QModelIndex &)),
           this, SLOT (PickOne (const QModelIndex &)));
  ui.stringList->setModel (&stringModel);
}

QString
PickString::Choice ()
{
  return choice;
}

int
PickString::Pick (const QStringList & choiceList)
{
  choice.clear();
  QStringList::const_iterator  slit;
  QStandardItem * item;
  stringModel.clear ();
  for (slit = choiceList.begin (); slit != choiceList.end (); slit++) {
    item = new QStandardItem (*slit);
    stringModel.appendRow (item);
  }
  return exec ();
}

void
PickString::PickNothing ()
{
  choice.clear();
  done (0);
}

void
PickString::PickOne (const QModelIndex &index)
{
  QStandardItem * item (0);
  item = stringModel.itemFromIndex (index);
  if (item) {
    choice = item->text();
    done (1);
  } else {
    choice.clear();
  }
}

void
PickString::PickCurrent ()
{
  QModelIndex index = ui.stringList->currentIndex();
  PickOne (index);
}

} // namespace

