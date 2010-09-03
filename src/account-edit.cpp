
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
 *****************************************************************/

#include "account-edit.h"
#include "cert-store.h"
#include "pick-string.h"
#include <QStringList>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>

namespace egalite
{

AccountEdit::AccountEdit (QWidget *parent)
  :QDialog (parent),
   haveAccount (false)
{
  ui.setupUi (this);
  Connect ();
}

void
AccountEdit::Connect ()
{
  bool ok (true);
  ok &= connect (ui.deleteButton, SIGNAL (clicked()),
           this, SLOT (DeleteAccount ()));
  ok &= connect (ui.saveButton, SIGNAL (clicked()),
           this, SLOT (SaveAccount ()));
  ok &= connect (ui.cancelButton, SIGNAL (clicked()),
           this, SLOT (Cancel ()));
}

void
AccountEdit::ClearForm ()
{
  ui.userNameEdit->clear ();
  ui.serverEdit->clear ();
  ui.passwordEdit->clear ();
}

void
AccountEdit::Exec ()
{
  PickString  picker (this);
  picker.move (this->pos());
  picker.SetTitle (tr("Pick Server Account "));
  QStringList list = CertStore::IF().AccountList ();
  QString newAccount (tr ("--- New Account ---"));
  list << newAccount;
  int picked = picker.Pick (list);
  if (picked) {
    QString ident = picker.Choice();
    ui.userNameEdit->setText (tr("New Name"));
    if (ident != newAccount) {
      Lookup (ident);
    }
    show ();
  }
}

void
AccountEdit::Cancel ()
{
  ClearForm ();
  haveAccount = false;
  hide ();
}

void
AccountEdit::SaveAccount ()
{
  QString jid (ui.userNameEdit->text());
  QString server (ui.serverEdit->text());
  QString pass (ui.passwordEdit->text());
  bool savedit = CertStore::IF().SaveAccount (jid, server, pass);
  if (savedit) {
    hide ();
    haveAccount = true;
  } else {
    haveAccount = false;
    QMessageBox  nosave;
    nosave.setText (tr("Cannot Save Account"));
    QTimer::singleShot (15000, &nosave, SLOT (reject()));
    nosave.exec ();
  }
}

void
AccountEdit::DeleteAccount ()
{
  bool gone (false);
  if (haveAccount) {
    QString jid (ui.userNameEdit->text());
    gone = CertStore::IF().DeleteAccount (jid);
    if (!gone) {
      QMessageBox  nodelete;
      nodelete.setText (tr("Cannot Delete Account"));
      QTimer::singleShot (15000, &nodelete, SLOT (reject()));
      nodelete.exec ();
    }
  }
}

bool
AccountEdit::Lookup (QString jid)
{
  QString server;
  QString pass;
  haveAccount = CertStore::IF().RecallAccount (jid, server, pass);
  if (haveAccount) {
    ui.userNameEdit->setText (jid);
    ui.serverEdit->setText (server);
    ui.passwordEdit->setText (pass);
  } else {
    ui.userNameEdit->clear ();
    ui.serverEdit->clear ();
    ui.passwordEdit->clear();
  }
  return haveAccount;
}


} // namespace

