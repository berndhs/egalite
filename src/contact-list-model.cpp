
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
#include "contact-list-model.h"
#include "deliberate.h"
#include <QDebug>

using namespace deliberate;

namespace egalite
{

ContactListModel::ContactListModel (QObject *parent)
  :QStandardItemModel (parent),
   nameTag ("remoteName"),
   resTag ("resource"),
   stateTag ("state")
{
}

void
ContactListModel::Setup ()
{
  iconSize = QString ("22x22");
  iconSize = Settings ().value ("style/iconSize",iconSize).toString();
  Settings().setValue ("style/iconSize",iconSize);
  QString iconDir = QString (":/icons");
  iconDir = Settings().value ("style/iconDir",iconDir).toString ();
  Settings().setValue ("style/iconDir",iconDir);
  iconPath = iconDir;
  iconPath.append ('/');
  iconPath.append (iconSize);
  iconPath.append ("/status/");
}

void
ContactListModel::PickedItem (const QModelIndex &index)
{
  QStandardItem * item = itemFromIndex (index);
  if (item) {
    if (item->data().toString() == nameTag) {
      QString target (item->text());
      emit StartServerChat (target);
    }
  }
}
    

bool 
ContactListModel::SetStatus (const QString & ownId,
                             const QString & remoteId,
                             const QString & resource,
                            QXmppPresence::Status::Type stype,
                             const QString & statusText)
{
  QStandardItem * accountHead = FindAccountGroup (ownId);
  QStandardItem * stateItem;
  QStandardItem * chase;
  bool  nameFound, resourceFound;
  QModelIndex  accIndex;
  accIndex = indexFromItem (accountHead);
  if (hasChildren (accIndex)) {
    int row;
    int nrows = accountHead->rowCount ();
    int ncols = accountHead->columnCount ();
    for (row = 0; row < nrows; row++) {
      stateItem = 0;
      nameFound = false;
      resourceFound = false;
      for (int col = 0; col < ncols; col++) {
        chase = accountHead->child (row, col);
        if (!chase) {
          continue;
        }
        QString tag = chase->data().toString();
        if (tag == nameTag && chase->text () == remoteId) {
          nameFound = true;
        } else if (tag == resTag && chase->text () == resource) {
          resourceFound = true;
        } else if (tag == stateTag) {
          stateItem = chase;
        }
      }
      if (nameFound && resourceFound && stateItem) { // in this row
        QString stext (statusText);
        if (stext.length() == 0) {
          stext = StatusName (stype);
        }
        stateItem->setText (stext);
        stateItem->setIcon (StatusIcon (stype));
        return true;
      }
    }
  }
  return false;
}

QString
ContactListModel::StatusName (QXmppPresence::Status::Type stype)
{
  switch (stype) {
  case QXmppPresence::Status::Offline: 
    return tr ("offline");
  case QXmppPresence::Status::Online:
    return tr ("On");
  case QXmppPresence::Status::Away:
    return tr ("away");
  case QXmppPresence::Status::XA:
    return tr ("XA");
  case QXmppPresence::Status::DND:
    return tr ("DND");
  case QXmppPresence::Status::Chat:
    return tr ("Chatty");
  case QXmppPresence::Status::Invisible:
    return tr ("Hiding");
  default:
    return tr ("?");
  }
}

QIcon
ContactListModel::StatusIcon (QXmppPresence::Status::Type stype)
{
  switch (stype) {
  case QXmppPresence::Status::Offline: 
    return QIcon (iconPath + QString ("user-offline.png"));
  case QXmppPresence::Status::Online:
    return QIcon (iconPath + QString("user-online.png"));
  case QXmppPresence::Status::Away:
    return QIcon (iconPath + QString ("user-away.png"));
  case QXmppPresence::Status::XA:
    return QIcon(iconPath + QString ("user-away-extended.png"));
  case QXmppPresence::Status::DND:
    return QIcon (iconPath + QString ("user-busy.png"));
  case QXmppPresence::Status::Chat:
    return QIcon(iconPath + QString ("user-online.png"));
  case QXmppPresence::Status::Invisible:
    return QIcon(iconPath + QString ("user-invisible"));
  default:
    return QIcon();
  }
}

void
ContactListModel::UpdateState (const QString & ownId,
                        const QString & remoteId, 
                        const QXmppPresence::Status & status)
{
  QXmppPresence::Status::Type stype = status.type ();
  QString statusText = status.statusText (); 
  QStringList parts = remoteId.split ('/',QString::SkipEmptyParts);
  QString remoteJid = parts.at(0);
  QString remoteResource;
  if (parts.size () > 1) {
    remoteResource = parts.at(1);
  }
  if (remoteResource.length() < 1) { // not really online anywhere
    stype = QXmppPresence::Status::Offline;
    statusText = tr("not found");
  }
  parts = ownId.split ('/',QString::SkipEmptyParts);
  QString ownJid = parts.at(0);
  bool old =  SetStatus (ownJid, remoteJid, remoteResource, stype, statusText);
  if (!old) {
    AddContact (remoteJid, remoteResource, ownId, stype, statusText);
  }
}

void
ContactListModel::AddAccount (const QString & id)
{
  FindAccountGroup (id);
}

void
ContactListModel::RemoveAccount (const QString & id)
{
  QStandardItem * deadAccount = FindAccountGroup (id);
  if (deadAccount) {
     int row = deadAccount->row ();
     if (row >= 0) {
       removeRow (row);
     }
  }
}

void
ContactListModel::AddContact (const QString & id, 
                       const QString & res, 
                       const QString & friendOf,
                       QXmppPresence::Status::Type stype,
                       const QString & statusText)
{
  QStringList parts = friendOf.split ('/',QString::SkipEmptyParts);
  QString ownId = parts.at(0);
  QStandardItem * groupItem = FindAccountGroup (ownId);
  QStandardItem * nameItem = new QStandardItem (id);
  nameItem->setData (nameTag);
  QStandardItem * stateItem = new QStandardItem (QString("?"));
  stateItem->setData (stateTag);
  QStandardItem * resourceItem = new QStandardItem (res);
  resourceItem->setData (resTag);
  QList <QStandardItem*> row;
  row << stateItem;
  row << nameItem;
  row << resourceItem;
  groupItem->appendRow (row);
  QString stext (statusText);
  if (stext.length() == 0) {
    stext = StatusName (stype);
  }
  stateItem->setText (stext);
  stateItem->setIcon (StatusIcon (stype));
  QString  bigId = id + QString("/") + res;
}

QStandardItem *
ContactListModel::FindAccountGroup (QString accountName, bool makeit)
{
  int nrows = rowCount ();
  QStandardItem * rowHead;
  for (int r = 0; r< nrows; r++) {
    rowHead = item (r,0);
    if (rowHead) {
      if (rowHead->text () == accountName) {
        return rowHead;
      }
    }
  }
  // not found - make a new one
  if (makeit) {
    rowHead = new QStandardItem (accountName);
    rowHead->setData (QString("accounthead"));
    appendRow (rowHead);
    return rowHead;
  } else {
    return 0;
  }
}


} // namespace

