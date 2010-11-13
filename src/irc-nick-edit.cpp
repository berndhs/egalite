
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

#include "irc-nick-edit.h"
#include "cert-store.h"
#include "pick-string.h"
#include <QStringList>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>

namespace egalite
{

IrcNickEdit::IrcNickEdit (QWidget *parent)
  :QDialog (parent),
   haveNick (false)
{
  ui.setupUi (this);
  Connect ();
}

void
IrcNickEdit::Connect ()
{
  bool ok (true);
  ok &= connect (ui.deleteButton, SIGNAL (clicked()),
           this, SLOT (DeleteNick ()));
  ok &= connect (ui.saveButton, SIGNAL (clicked()),
           this, SLOT (SaveNick ()));
  ok &= connect (ui.cancelButton, SIGNAL (clicked()),
           this, SLOT (Cancel ()));
}

void
IrcNickEdit::ClearForm ()
{
  ui.userNameEdit->clear ();
  ui.nickEdit->clear ();
  ui.passwordEdit->clear ();
}

void
IrcNickEdit::Exec ()
{
  PickString  picker (this);
  picker.move (this->pos());
  picker.SetTitle (tr("Pick IRC Nick "));
  QStringList list = CertStore::IF().IrcNicks ();
  QString newNick (tr ("--- New Nick ---"));
  list << newNick;
  int picked = picker.Pick (list);
  if (picked) {
    QString ident = picker.Choice();
    ui.userNameEdit->setText (tr("New Name"));
    if (ident != newNick) {
      Lookup (ident);
    }
    show ();
  }
}

void
IrcNickEdit::Cancel ()
{
  ClearForm ();
  haveNick = false;
  hide ();
}

void
IrcNickEdit::SaveNick ()
{
  QString ircUser (ui.userNameEdit->text());
  QString nick (ui.nickEdit->text());
  QString pass (ui.passwordEdit->text());
  CertStore::IF().SaveIrcNick (nick, ircUser, pass);
  QString pmsg (ui.partEdit->text());
  QString qmsg (ui.quitEdit->text());
  CertStore::IF().SaveIrcMessages (nick, pmsg, qmsg);
  hide ();
  haveNick = true;
}

void
IrcNickEdit::DeleteNick ()
{
  bool gone (false);
  if (haveNick) {
    QString nick (ui.nickEdit->text());
    gone = CertStore::IF().RemoveIrcNick (nick);
    if (!gone) {
      QMessageBox  nodelete;
      nodelete.setText (tr("Cannot Delete Nick"));
      QTimer::singleShot (15000, &nodelete, SLOT (reject()));
      nodelete.exec ();
    }
  }
}

bool
IrcNickEdit::Lookup (QString nick)
{
  QString ircUser;
  QString pass;
  QString pmsg;
  QString qmsg;
  haveNick = CertStore::IF().GetIrcIdent (nick, ircUser, pass);
  CertStore::IF().GetIrcMessages (nick, pmsg, qmsg);
  if (haveNick) {
    ui.userNameEdit->setText (ircUser);
    ui.nickEdit->setText (nick);
    ui.passwordEdit->setText (pass);
    ui.partEdit->setText (pmsg);
    ui.quitEdit->setText (qmsg);
  } else {
    ui.userNameEdit->clear ();
    ui.nickEdit->clear ();
    ui.passwordEdit->clear();
  }
  return haveNick;
}


} // namespace

