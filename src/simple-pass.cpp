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

#include "simple-pass.h"

namespace egalite
{

SimplePass::SimplePass (QWidget *parent)
  :QDialog (parent)
{
  setupUi (this);
  connect (okButton, SIGNAL (clicked()), this, SLOT (Ok()));
  connect (cancelButton, SIGNAL (clicked()), this, SLOT (Cancel()));
}

QString
SimplePass::GetPassword (QString purpose)
{
  purposeLabel->setText (purpose);
  passwordEdit->setEchoMode (QLineEdit::Password);
  passwordEdit->clear ();
  int yes = exec ();
  if (yes == 1) {
    return passwordEdit->text();
  } else {
    return QString ();
  }
}

QString
SimplePass::GetPlainString (QString title, QString purpose)
{
  setWindowTitle (title);
  purposeLabel->setText (purpose);
  passwordEdit->setEchoMode (QLineEdit::Normal);
  int yes = exec ();
  if (yes == 1) {
    return passwordEdit->text();
  } else {
    return QString ();
  }
}

bool
SimplePass::GotPassword ()
{
  return gotPassword;
}

bool
SimplePass::GotPlainString ()
{
  return gotPlain;
}

void
SimplePass::Ok ()
{
  QLineEdit::EchoMode mode = passwordEdit->echoMode ();
  gotPassword = (mode == QLineEdit::Password);
  gotPlain =    (mode == QLineEdit::Normal);
  done (1);
}

void
SimplePass::Cancel ()
{
  gotPassword = false;
  gotPlain = false;
  passwordEdit->clear ();
  done (0);
}

} // namespace

