#ifndef ACCOUNT_EDIT_H
#define ACCOUNT_EDIT_H

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
#include "ui_server-account.h"
#include <QDialog>

namespace egalite
{

class AccountEdit : public QDialog 
{
Q_OBJECT

public:

  AccountEdit (QWidget *parent = 0);

  void Exec ();

public slots:

  void Cancel ();
  void SaveAccount ();
  void DeleteAccount ();

private:

  void Connect ();
  void ClearForm ();
  bool Lookup (QString jid);
  
  bool haveAccount;

  Ui_ServerAccountEdit    ui;

};

} // namespace

#endif
