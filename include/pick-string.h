#ifndef PICK_DIRECT_ORIGIN_H
#define PICK_DIRECT_ORIGIN_H

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

#include "ui_pick-string.h"
#include <QDialog>
#include <QString>
#include <QStringList>
#include <QStandardItemModel>

namespace egalite
{

class PickString : public QDialog
{
 Q_OBJECT

public:

  PickString (QWidget *parent=0);

  void SetTitle (QString title) { setWindowTitle (title); }

  int Pick (const QStringList & choiceList);
  
  QString Choice();

private slots:

  void PickCurrent ();
  void PickNothing ();
  void PickOne (const QModelIndex & index);

private:

  Ui_PickString    ui;

  QStandardItemModel   stringModel;
  QString              choice;

};

} // namespace


#endif

