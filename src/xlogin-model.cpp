

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

#include "xlogin-model.h"
#include <QHash>
#include <QByteArray>
#include <QDebug>

namespace egalite
{

XLoginModel::XLoginModel (QObject *parent)
  :QAbstractListModel(parent)
{
  QHash<int, QByteArray> roles;
  roles[Data_Jid] = "loginJid";
  roles[Data_Name] = "loginName";
  setRoleNames(roles);
}

int
XLoginModel::rowCount(const QModelIndex &parent) const
{
  return logins.count();
}

QVariant
XLoginModel::data (const QModelIndex &index, int role) const
{
  qDebug () << __PRETTY_FUNCTION__ << index << role;
  int row = index.row();
  if (row < 0 || row >= logins.count()) {
    return QVariant();
  }
  switch (role) {
  case Data_Jid:
    return logins.at(row).jid();
    break;
  case Data_Name:
    return logins.at(row).name();
    break;
  default:
    break;
  }
  return QVariant();
}


QObject *
XLoginModel::contacts (const QString &jid) const
{
  if (contactModels.contains (jid)) {
    return contactModels[jid];
  } else {
    return 0;
  }
}

void
XLoginModel::addLogin (const QString & jid, const QString & name)
{
  for (int r=0; r<logins.count(); r++) {
    if (logins.at(r).jid() == jid) {
      return;
    }
  }
  beginInsertRows (QModelIndex(),logins.count(),logins.count());
  logins.append (XLoginItem(jid,name));
  if (contactModels.contains(jid)) {
    endInsertRows();
    return;
  }
  contactModels[jid] = new XContactModel;
  endInsertRows();
}

void
XLoginModel::addContact (const QString &ownJid, 
                         const QString &otherJid, 
                         const QString &otherName, 
                         const QString &otherResource, 
                         QXmppPresence::Status::Type stype)
{
  if (!contactModels.contains(ownJid)) {
    contactModels[ownJid] = new XContactModel;
  }
  contactModels[ownJid]->addContact(XContactItem (otherJid,otherName,
                                               otherResource, stype));
}

void
XLoginModel::removeLogin(const QString &jid)
{
  for (int r=logins.count()-1; r >=0 ; r--) {
    if (logins.at(r).jid() == jid) {
      beginRemoveRows (QModelIndex(),r,r);
      logins.removeAt(r);
      endRemoveRows ();
    }
  }
  contactModels.remove (jid);
}

void
XLoginModel::updateState (const QString & ownJid, 
                  const QString & otherLongId, 
                  const QString & otherName,
                  QXmppPresence::Status::Type stype)
{
  if (contactModels.contains (ownJid)) {
    XContactModel * model = contactModels[ownJid];
    if (model) {
      model->updateState (otherLongId,otherName, stype);
    }
  }
}

void
XLoginModel::updateStateAll (const QString &ownJid, 
                             const QString &otherJid, 
                             QXmppPresence::Status::Type stype)
{
  if (contactModels.contains (ownJid)) {
    XContactModel * model = contactModels[ownJid];
    if (model) {
      model->updateStateAll (otherJid, stype);
    }
  }
  
}

} // namespace
