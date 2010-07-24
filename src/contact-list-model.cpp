
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
  :QStandardItemModel (parent)
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
  qDebug () << " picked model item " << index;
  int row = index.row ();
  QStandardItem *nameCell = item (row,1);
  QStandardItem *resoCell = item (row,2);
  if (nameCell) {
    QString  target (nameCell->text());
    if (resoCell) {
      target.append ("/");
      target.append (resoCell->text());
    }
    emit StartServerChat (target);
  }
}
    

void 
ContactListModel::SetStatus (int row, 
                      QXmppPresence::Status::Type stype,
                      QString statusText)
{
  QStandardItem * stateItem = item (row,0);
  if (stateItem == 0) {
    stateItem = new QStandardItem;
    setItem (row,0,stateItem);
  }
  if (statusText.length() == 0) {
    statusText = StatusName (stype);
  }
  stateItem->setIcon (StatusIcon (stype));
  stateItem->setText (statusText);
  
qDebug () << " did set status row " << row << " text " << stateItem->text();
}

QString
ContactListModel::StatusName (QXmppPresence::Status::Type stype)
{
  switch (stype) {
  case QXmppPresence::Status::Offline: 
    return tr ("-");
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
  QString id = parts.at(0);
  QString resource;
  if (parts.size () > 1) {
    resource = parts.at(1);
  }
  ContactMap::iterator contactit = serverContacts.find (remoteId);
  if (contactit != serverContacts.end()) {
    if (contactit->second) {
      SetStatus (contactit->second->modelRow, stype, statusText);
      contactit->second->recentlySeen = true;
    }
  } else {
    parts = ownId.split ('/', QString::SkipEmptyParts);
    AddContact (id, resource, parts.at(0), stype, statusText);
  }
}

void
ContactListModel::AddContact (const QString & id, 
                       const QString & res, 
                       const QString & friendOf,
                       QXmppPresence::Status::Type stype,
                       const QString & statusText)
{
  ServerContact * newContact = new ServerContact;
  newContact->name = id;
  newContact->state = QString("?");
  newContact->friendOf = friendOf,
  newContact->resource = res;
  newContact->recentlySeen = true;
  QStandardItem * nameItem = new QStandardItem (newContact->name);
  QStandardItem * stateItem = new QStandardItem (newContact->state);
  QStandardItem * resourceItem = new QStandardItem (newContact->resource);
  QList <QStandardItem*> row;
  row << stateItem;
  row << nameItem;
  row << resourceItem;
  appendRow (row);
  newContact->modelRow = stateItem->row ();
  SetStatus (newContact->modelRow, stype, statusText);
  QString  bigId = id + QString("/") + res;
  serverContacts [bigId] = newContact;
}


} // namespace

