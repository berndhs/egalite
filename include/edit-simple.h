#ifndef EDIT_SIMPLE_H
#define EDIT_SIMPLE_H

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

#include "ui_edit-simple.h"
#include <QDialog>
#include <QString>
#include <QStringList>

namespace egalite
{
class EditSimple : public QDialog
{
Q_OBJECT

public:

  EditSimple (QWidget *parent=0);
  EditSimple (const QString & title, QWidget *parent=0);

  void SetTitle (const QString & newTitle);
  void SetAcceptText (const QString & newText);
  void SetDeleteText (const QString & newText);
  void SetSaver (void (*saveFunc) (const QString&));
  void SetRemover (void (*deleteFunc) (const QString&));
  void SetLoader (QStringList (*loadFunc) ());
  void SetFuncs (void (*saveFunc) (const QString &),
                 void (*deleteFunc) (const QString &),
                 QStringList (*loadFunc) ());

  int  Exec (bool allowNew = true);
  int  Exec (const QStringList & choiceList,
             bool allowNew = true);
  int  Exec (const QStringList & choiceList,
             const QString & newTitle,
             const QString & newAccept,
             const QString & newDelete,
             bool  allowNew = true);
  int  Exec (const QString & newTitle,
             const QString & newAccept,
             const QString & newDelete,
             bool  allowNew = true);
  QString Choice ();

private slots:

  void  Save ();
  void  Remove ();
  void  Cancel ();

private:

  void AddNew ();
  void SetChoices (const QStringList & newList);
  void Connect ();

  Ui_EditSimple   ui;

  QStringList   choices;
  QString       newName;
  bool          newAllowed;
  void         (*saver) (const QString &);
  void         (*remover) (const QString &);
  QStringList  (*loader) ();

};

} // namespace

#endif
