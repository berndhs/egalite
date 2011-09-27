#include "xcontact-model.h"

/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2011, Bernd Stramm
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

#include <QStringList>

#include <QDebug>

namespace egalite
{

XContactLoginModel::XContactLoginModel ()
{
  qDebug () << __PRETTY_FUNCTION__;
}

void
XContactLoginModel::addLogin (const XContactLoginItem &item)
{
  qDebug () << __PRETTY_FUNCTION__ << item.resource () << item.presence();
  int nr = logins.count();
  for (int r=0; r<nr; r++) {
    if (logins.at(r).resource() == item.resource()) {
      logins[r].setPresence(item.presence());
      return;
    }
  }
  logins.append (item);
}

void
XContactLoginModel::removeLogin (const QString & longId)
{
  QStringList parts = longId.split ('/',QString::SkipEmptyParts);
  QString jid = parts.at(0);
  QString resource;
  if (parts.size () > 1) {
    resource = parts.at(1);
  }
  int nr = logins.count();
  for (int r=nr-1; r>=0; r--) {
    if (logins.at(r).resource() == resource) {
      logins.removeAt(r);
      return;
    }
  }
}

void
XContactLoginModel::updateState (const QString & resource, const QString & status, 
                           QXmppPresence::Status::Type stype)
{
  int nr = logins.count();
  qDebug () << __PRETTY_FUNCTION__ << "  model-update bottom "
            << resource << status << stype ;
  for (int r=0; r<nr; r++) {
    if (logins.at(r).resource() == resource) {
      logins[r].setPresence (stype);
      logins[r].setStatus (status);
      return;
    }
  }
  logins.append (XContactLoginItem (resource,status,stype));
}

void
XContactLoginModel::updateStateAll (QXmppPresence::Status::Type stype)
{
  int nr = logins.count();
  for (int r=0; r<nr; r++) {
    logins[r].setPresence (stype);
  }
}

void
XContactLoginModel::copyLogins(const XContactLoginModel &other)
{
  logins.clear ();
  for (auto lo = other.logins.constBegin(); 
       lo != other.logins.constEnd(); lo++) {
    logins.append (*lo);
  }
}

} // namespace
