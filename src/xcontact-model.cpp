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
  Q_UNUSED (parent);
  qDebug () << __PRETTY_FUNCTION__;
  QHash<int, QByteArray> roles;
  roles[Data_OtherJid] = "contactOtherJid";
  roles[Data_OtherName] = "contactOtherName";
  roles[Data_LoginCount] = "contactLoginCount";
  roles[Data_MyJid] = "contactMyJid";
  roles[Data_BestStatus] = "contactBestStatus";
  setRoleNames (roles);
}

int
XContactModel::rowCount(const QModelIndex &parent) const
{
  return contacts.count();
}

QVariant
XContactModel::data (const QModelIndex &index, int role) const
{
  int row = index.row();
  if (row < 0 || row >= contacts.count()) {
    qDebug () << __PRETTY_FUNCTION__ << " bad row "  << row;
    return QVariant();
  }
  QVariant retVar;
  switch (role) {
  case Data_OtherJid:
    retVar = contacts.at(row).jid();
    break;
  case Data_OtherName:
    retVar = contacts.at(row).name();
    break;
  case Data_LoginCount:
    retVar = contacts.at(row).loginCount();
    break;
  case Data_BestStatus:
    retVar = bestStatus(row);
    break;
  case Data_MyJid:
    if (!contacts.at(row).identities().isEmpty()) {
      retVar = contacts.at(row).identities().at(0);
    } else {
      retVar = tr ("unkown identify");
    }
    break;
  default:
    qDebug () << __PRETTY_FUNCTION__ << " bad role " << role;
    break;
  }
  return retVar;
}

int
XContactModel::bestStatus (int row) const
{
  static QMap<QXmppPresence::Status::Type,int> levels (initStatusPriorities());
  
  if (row < 0 || row >= contacts.count()) {
    return int (QXmppPresence::Status::Offline);
  }
  XContactItem item = contacts[row];
  int best(0);
  QXmppPresence::Status::Type bestStat (QXmppPresence::Status::Offline);
  QList<XContactLoginItem> & logs = item.loginsRef().itemsRef();
  for (auto lo = logs.begin(); lo != logs.end(); lo++) {
    QXmppPresence::Status::Type stat = lo->presence();
    if (levels.contains(stat)) {
      int level = levels[stat];
      if (level > best) {
        best = level;
        bestStat = stat;
      }
    }
  }
  qDebug () << __PRETTY_FUNCTION__ << " best is " << bestStat;
  return int (bestStat);
}

QMap<QXmppPresence::Status::Type,int>
XContactModel::initStatusPriorities ()
{
  QMap<QXmppPresence::Status::Type,int> levels;
  
  levels[QXmppPresence::Status::Offline] = 0;
  levels[QXmppPresence::Status::Online] = 10;
  levels[QXmppPresence::Status::Away] = 9;
  levels[QXmppPresence::Status::DND] = 8;
  levels[QXmppPresence::Status::Chat] = 11;
  levels[QXmppPresence::Status::XA] = 7;
  levels[QXmppPresence::Status::Invisible] = 5;
  qDebug () << __PRETTY_FUNCTION__ << levels;
  return levels;
}

QString
XContactModel::imageName(const int statusCode) const
{
  QXmppPresence::Status::Type stype 
      = static_cast <QXmppPresence::Status::Type> (statusCode);
  
  QString name ;
  switch (stype) {
  case QXmppPresence::Status::Offline:
    name = "user-offline.png";
    break;
  case QXmppPresence::Status::Online:
    name = "user-online.png";
    break;
  case QXmppPresence::Status::Away:
    name = "user-away.png";
    break;
  case QXmppPresence::Status::DND:
    name = "user-busy.png";
    break;
  case QXmppPresence::Status::Chat:
    name = "user-online.png";
    break;
  case QXmppPresence::Status::XA:
    name = "user-away-extended.png";
    break;
  case QXmppPresence::Status::Invisible:
    name = "user-invisible.png";
    break;
  default:
    break;
  }
  qDebug () << __PRETTY_FUNCTION__ << " Status image " << name;
  return name;
}

void
XContactModel::addContact (const XContactItem &item)
{
  qDebug () << __PRETTY_FUNCTION__ << item.jid() << item.name ();
  int nr = contacts.count();
  for (int r=0; r<nr; r++) {
    if (contacts.at(r).jid() == item.jid()) {
      contacts[r].setName (item.name());
      emit dataChanged(index(r,0),index(r,0));
      return;
    }
  }
  beginInsertRows(QModelIndex(),nr,nr);
  contacts.append (item);
  endInsertRows();
  emit dataChanged (index(0,0),index(nr,0));
  emit newContact (item.jid());
}

void
XContactModel::removeContact (const QString & longId)
{
  QStringList parts = longId.split ('/',QString::SkipEmptyParts);
  QString jid = parts.at(0);
  QString resource;
  if (parts.size () > 1) {
    resource = parts.at(1);
  }
  int nr = contacts.count();
  for (int r=0; r<nr; r++) {
    if (contacts.at(r).jid() == jid) {
      beginRemoveRows(QModelIndex(),r,r);
      contacts.removeAt(r);
      endRemoveRows();
      return;
    }
  }
}

void
XContactModel::updateState (const QString & longOtherJid, 
                            const QString & myJid,
                            const QString & name, 
                            const QString & statusText,
                           QXmppPresence::Status::Type stype)
{
  QStringList parts = longOtherJid.split ('/',QString::SkipEmptyParts);
  QString otherJid = parts.at(0);
  QString resource;
  if (parts.size () > 1) {
    resource = parts.at(1);
  }
  int nr = contacts.count();
  qDebug () << __PRETTY_FUNCTION__ << "  model-update bottom "
            << longOtherJid << resource << name << stype ;
  for (int r=0; r<nr; r++) {
    if (contacts.at(r).jid() == otherJid) {
      contacts[r].loginsRef().updateState(resource,statusText,stype);
      emit dataChanged (index (r,0),index(r,0));
      if (!contacts[r].identities().contains (myJid)) {
        contacts[r].identities().append (myJid);
      }
      return;
    }
  }
  beginInsertRows (QModelIndex(),nr,nr);
  contacts.append (XContactItem (otherJid,name));
  contacts[nr].loginsRef().updateState (resource,statusText,stype);
  contacts[nr].identities().append(myJid);
  endInsertRows ();
  emit dataChanged (index(0,0),index(nr,0));
  emit newContact(otherJid);
}

void
XContactModel::updateStateAll (const QString & longId, 
                               QXmppPresence::Status::Type stype)
{
  QStringList parts = longId.split ('/',QString::SkipEmptyParts);
  QString jid = parts.at(0);
  
  int nr = contacts.count();
  int firstChanged (nr);
  int lastChanged (-1);
  for (int r=0; r<nr; r++) {
    if (contacts.at(r).jid() == jid) {
      contacts[r].loginsRef().updateStateAll (stype);
      if (r < firstChanged) {
        firstChanged = r;
      }
      if (r > lastChanged) {
        lastChanged = r;
      }
      return;
    }
  }
  qDebug () << __PRETTY_FUNCTION__ << longId << firstChanged << lastChanged;
  if (firstChanged <= lastChanged) {
    emit dataChanged (index (firstChanged, 0), index (lastChanged, 0));
  } else {
    beginInsertRows(QModelIndex(),nr,nr);
    contacts.append (XContactItem (jid,""));
    contacts[nr].loginsRef().updateStateAll (stype);
    endInsertRows();
    emit dataChanged (index(0,0),index(nr,0));
    emit newContact(jid);
  }
  
}

void
XContactModel::removeMyJid (const QString & myJid)
{
  for (auto cont = contacts.begin(); cont != contacts.end(); cont++) {
    QStringList & idents = cont->identities();
    idents.removeAll(myJid);
    if (idents.isEmpty()) {
      contacts.erase (cont);
    }
  }
}

} // namespace
