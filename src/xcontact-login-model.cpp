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

XContactLoginModel::XContactLoginModel (QObject *parent)
  :QAbstractListModel (parent)
{
  qDebug () << __PRETTY_FUNCTION__;
  QHash<int, QByteArray> roles;
  roles[Data_Status] = "statusString";
  roles[Data_Resource] = "resource";
  roles[Data_Presence] = "contactPresence";
  roles[Data_IconName] = "statusIconName";
  setRoleNames(roles);
}

int
XContactLoginModel::rowCount(const QModelIndex &parent) const
{
  qDebug () << __PRETTY_FUNCTION__ ;
  return logins.count();
}

QVariant
XContactLoginModel::data (const QModelIndex &index, int role) const
{
  qDebug () << __PRETTY_FUNCTION__ << index << role;
  int row = index.row();
  if (row < 0 || row >= logins.count()) {
    return QVariant();
  }
  QVariant retVar;
  switch (role) {
  case Data_Resource:
    retVar = logins.at(row).resource();
    break;
  case Data_Presence:
    retVar = logins.at(row).presence();
    break;
  case Data_IconName:
    retVar = iconFileName (logins.at(row).presence());
    break;
  case Data_Status:
    retVar = logins.at(row).status();
  default:
    break;
  }
  qDebug () << "    model-update XContactLoginModel returning " << retVar;
  return retVar;
}

QString
XContactLoginModel::iconFileName(QXmppPresence::Status::Type stype) const
{
  switch (stype) {
  case QXmppPresence::Status::Offline:
    return "user-offline.png";
    break;
  case QXmppPresence::Status::Online:
    return "user-online.png";
    break;
  case QXmppPresence::Status::Away:
    return "user-away.png";
    break;
  case QXmppPresence::Status::DND:
    return "user-busy.png";
    break;
  case QXmppPresence::Status::Chat:
    return "user-online.png";
    break;
  case QXmppPresence::Status::XA:
    return "user-away-extended.png";
    break;
  case QXmppPresence::Status::Invisible:
    return "user-invisible.png";
    break;
  default:
    break;
  }
  return "";
}

void
XContactLoginModel::addLogin (const XContactLoginItem &item)
{
  qDebug () << __PRETTY_FUNCTION__ << item.resource () << item.presence();
  int nr = logins.count();
  for (int r=0; r<nr; r++) {
    if (logins.at(r).resource() == item.resource()) {
      logins[r].setPresence(item.presence());
      emit dataChanged(index(r,0),index(r,0));
      return;
    }
  }
  beginInsertRows(QModelIndex(),nr,nr);
  logins.append (item);
  endInsertRows();
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
      beginRemoveRows(QModelIndex(),r,r);
      logins.removeAt(r);
      endRemoveRows();
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
      emit dataChanged (index (r,0),index(r,0));
      return;
    }
  }
  beginInsertRows (QModelIndex(),nr,nr);
  logins.append (XContactLoginItem (resource,status,stype));
  endInsertRows ();
}

void
XContactLoginModel::updateStateAll (QXmppPresence::Status::Type stype)
{
  int nr = logins.count();
  for (int r=0; r<nr; r++) {
    logins[r].setPresence (stype);
  }
  if (nr > 0) {
    emit dataChanged (index (0, 0), index (nr-1, 0));
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
