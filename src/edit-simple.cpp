
#include "edit-simple.h"

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

EditSimple::EditSimple (QWidget *parent)
  :QDialog (parent),
   saver (0),
   remover (0),
   loader (0)
{
}

void
EditSimple::SetTitle (const QString & newTitle)
{
  setWindowTitle (newTitle);
}

void
EditSimple::SetAcceptText (const QString & newText)
{
  ui.saveButton->setText (newText);
}

void
EditSimple::SetDeleteText (const QString & newText)
{
  ui.deleteButton->setText (newText);
}

void
EditSimple::SetChoices (const QStringList & newList)
{
  choices = newList;
}

void
EditSimple::SetSaver (void (*saveFunc) (const QString &))
{
  saver = saveFunc;
}

void
EditSimple::SetRemover (void (*deleteFunc) (const QString &))
{
  remover = deleteFunc;
}

void
EditSimple::SetLoader (QStringList (*loadFunc) ())
{
  loader = loadFunc;
}

void
EditSimple::AddNew ()
{
  newName = tr ("--- New ---");
  choices.append (newName);
}

void
EditSimple::Save ()
{
  QListWidgetItem * item = ui.stringList->currentItem();
  if (item && saver) {
    QString choice = item->text ();
    (*saver) (choice);
  }
  done (1);
}

void
EditSimple::Remove ()
{
  QListWidgetItem * item = ui.stringList->currentItem();
  if (item && remover) {
    QString choice = item->text ();
    (*remover) (choice);
  }
  done (-1);
}

void
EditSimple::Cancel ()
{
  done (0);
}

int
EditSimple::Exec (bool allowNew)
{
  if (loader) {
    choices = (*loader) ();
  }
  if (allowNew) {
    AddNew ();
  }
  ui.stringList->clear ();
  ui.stringList->addItems (choices);
  return exec ();
}

int 
EditSimple::Exec (const QStringList & choiceList,
                  bool allowNew)
{
  choices = choiceList;
  if (allowNew) {
    AddNew ();
  }
  ui.stringList->clear ();
  ui.stringList->addItems (choices);
  return exec ();
}

int 
EditSimple::Exec (const QStringList & choiceList,
             const QString & newTitle,
             const QString & newAccept,
             const QString & newDelete,
             bool  allowNew )
{
  SetTitle (newTitle);
  SetAcceptText (newAccept);
  SetDeleteText (newDelete);
  SetChoices (choiceList);
  return Exec (choiceList, allowNew);
}

int 
EditSimple::Exec (const QString & newTitle,
             const QString & newAccept,
             const QString & newDelete,
             bool  allowNew )
{
  SetTitle (newTitle);
  SetAcceptText (newAccept);
  SetDeleteText (newDelete);
  return Exec (allowNew);
}


} // namespace

