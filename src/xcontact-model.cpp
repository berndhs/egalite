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

#include <QModelIndex>
#include <QStringList>

#include <QDebug>

namespace egalite
{

XContactModel::XContactModel (QObject *parent)
  :QAbstractListModel (parent)
{
  QHash<int, QByteArray> roles;
  roles[Data_Jid] = "contactJid";
  roles[Data_Name] = "contactName";
  roles[Data_Resource] = "contactResource";
  roles[Data_Presence] = "contactPresence";
}

int
XContactModel::rowCount(const QModelIndex &parent) const
{
  return contacts.count();
}

QVariant
XContactModel::data (const QModelIndex &index, int role) const
{
  qDebug () << __PRETTY_FUNCTION__ << index << role;
  int row = index.row();
  if (row < 0 || row >= contacts.count()) {
    return QVariant();
  }
  switch (role) {
  case Data_Jid:
    return contacts.at(row).jid();
    break;
  case Data_Name:
    return contacts.at(row).name();
    break;
  case Data_Resource:
    return contacts.at(row).resource();
    break;
  case Data_Presence:
    return contacts.at(row).presence();
    break;
  default:
    break;
  }
  return QVariant();
}

void
XContactModel::addContact (const XContactItem &item)
{
  int nr = contacts.count();
  for (int r=0; r<nr; r++) {
    if (contacts.at(r).jid() == item.jid()
        && contacts.at(r).resource() == item.resource()) {
      contacts[r] = item;
      emit dataChanged(index(r,0),index(r,0));
      return;
    }
  }
  beginInsertRows(QModelIndex(),nr,nr);
  contacts.append (item);
  endInsertRows();
}

void
XContactModel::removeContact (const QString &longId)
{
  QStringList parts = longId.split ('/',QString::SkipEmptyParts);
  QString jid = parts.at(0);
  QString resource;
  if (parts.size () > 1) {
    resource = parts.at(1);
  }
  int nr = contacts.count();
  for (int r=0; r<nr; r++) {
    if (contacts.at(r).jid() == jid
        && contacts.at(r).resource() == resource) {
      beginRemoveRows(QModelIndex(),r,r);
      contacts.removeAt(r);
      endRemoveRows();
      return;
    }
  }
}

void
XContactModel::updateState(const QString &longId, const QString &name, 
                           QXmppPresence::Status::Type stype)
{
  QStringList parts = longId.split ('/',QString::SkipEmptyParts);
  QString jid = parts.at(0);
  QString resource;
  if (parts.size () > 1) {
    resource = parts.at(1);
  }
  int nr = contacts.count();
  for (int r=0; r<nr; r++) {
    if (contacts.at(r).jid() == jid
        && contacts.at(r).resource() == resource) {
      contacts[r].setPresence (stype);
      contacts[r].setName(name);
      emit dataChanged (index (r,0),index(r,0));
      return;
    }
  }
}

void
XContactModel::updateStateAll (const QString & longId, QXmppPresence::Status::Type stype)
{
  QStringList parts = longId.split ('/',QString::SkipEmptyParts);
  QString jid = parts.at(0);
  
  int nr = contacts.count();
  int firstChanged (nr);
  int lastChanged (-1);
  for (int r=0; r<nr; r++) {
    if (contacts.at(r).jid() == jid) {
      contacts[r].setPresence (stype);
      if (r < firstChanged) {
        firstChanged = r;
      }
      if (r > lastChanged) {
        lastChanged = r;
      }
      return;
    }
  }
  if (firstChanged <= lastChanged) {
    emit dataChanged (index (firstChanged, 0), index (lastChanged, 0));
  }
  
}

} // namespace
