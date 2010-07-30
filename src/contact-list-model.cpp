
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

int ContactListModel::tagData ( Qt::UserRole + 1 );
int ContactListModel::statusData ( Qt::UserRole + 2);

ContactListModel::ContactListModel (QObject *parent)
  :QStandardItemModel (parent),
   nameTag ("remoteName"),
   resTag ("resource"),
   stateTag ("state"),
   nickTag ("nick"),
   discardOfflines (true),
   presentColor (Qt::blue),
   absentColor (Qt::black),
   cleanTimer (this)
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
  discardOfflines = Settings().value ("network/hideOffline",discardOfflines)
                              .toBool ();
  Settings().setValue ("network/hideOffline",discardOfflines);
  QVariant color = Settings().value ("style/presentColor",presentColor);
  if (color.canConvert<QColor> ()) {
    presentColor = color.value<QColor>();                       
  }
  Settings().setValue ("style/presentColor",presentColor);
  color = Settings().value ("style/absentColor",absentColor);
  if (color.canConvert<QColor> ()) {
    absentColor = color.value<QColor>();
  }
  Settings().setValue ("style/absentColor",absentColor);
  cleanTimer.stop ();
  connect (&cleanTimer, SIGNAL (timeout()), this, SLOT (HighlightStatus()));
  cleanTimer.start (10000);
}

void
ContactListModel::PickedItem (const QModelIndex &index)
{
  QStandardItem * item = itemFromIndex (index);
  if (item) {
    if (item->data(tagData).toString() == nameTag) {
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
                             const QString & statusText,
                             bool  allResources)
{
  QStandardItem * accountHead = FindAccountGroup (ownId);
  QStandardItem * contactHead = FindContactGroup (accountHead, remoteId);
  if (contactHead == 0) { // data structure is corrupted if this happens
    return false;
  }
  QStandardItem * stateItem;
  QStandardItem * chase;
  bool resourceFound;
  if (contactHead->hasChildren()) {
    int row;
    int nrows = contactHead->rowCount ();
    int ncols = contactHead->columnCount ();
    for (row = 0; row < nrows; row++) {
      stateItem = 0;
      resourceFound = false;
      for (int col = 0; col < ncols; col++) {
        chase = contactHead->child (row, col);
        if (!chase) {
          continue;
        }
        QString tag = chase->data(tagData).toString();
        if (tag == resTag && (allResources || chase->text () == resource)) {
          resourceFound = true;
        } else if (tag == stateTag) {
          stateItem = chase;
        }
      }
      if (resourceFound && stateItem) { // in this row
        QString stext (statusText);
        if (stext.length() == 0) {
          stext = StatusName (stype);
        }
        stateItem->setText (stext);
        stateItem->setIcon (StatusIcon (stype));
        stateItem->setData (IsOnline (stype),statusData);
        return true;
      }
    }
  }
  return false;
}

void
ContactListModel::RemoveContact (const QString & ownId,
                                 const QString & remoteId,
                                 const QString & resource,
                                 bool  allResources)
{  
  bool resourceFound;
  int row;
  QStandardItem * chase;
  QStandardItem * accountHead = FindAccountGroup (ownId);
 
  QStandardItem * nameItem = FindContactGroup (accountHead, remoteId);
  if (!nameItem) { // not here, can't remove
    return;
  }
  QStandardItem * stateItem;
  for (row = 0; row < nameItem->rowCount (); row++) {
    resourceFound = false;
    for (int col = 0; col < nameItem->columnCount (); col++) {
      chase = nameItem->child (row, col);
      if (!chase) {
        continue;
      }
      QString tag = chase->data(tagData).toString();
      if (tag == resTag && (allResources || chase->text () == resource)) {
        resourceFound = true;
      } else if (tag == stateTag) {
        stateItem = chase;
      }
    }
    if (resourceFound) { // in this row
      nameItem->removeRow (row);
      if (!nameItem->hasChildren()) {  // we removed the last one
        int ownRow = nameItem->row ();
        if (ownRow >= 0) {
          nameItem = 0;
          accountHead->removeRow (ownRow);
        }
      }
      return;
    }
  }
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

bool
ContactListModel::IsOnline (QXmppPresence::Status::Type stype)
{
  switch (stype) {
  case QXmppPresence::Status::Offline: 
    return false;
  case QXmppPresence::Status::Online:
  case QXmppPresence::Status::Away:
  case QXmppPresence::Status::XA:
  case QXmppPresence::Status::DND:
  case QXmppPresence::Status::Chat:
    return true;
  case QXmppPresence::Status::Invisible:
    return false;
  default:
    return false;
  }
}

void
ContactListModel::UpdateState (const QString &remoteName,
                        const QString & ownId,
                        const QString & remoteId, 
                        const QXmppPresence::Status & status,
                        bool  allResources)
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
  if (discardOfflines && stype == QXmppPresence::Status::Offline) {
    RemoveContact (ownJid, remoteJid, remoteResource, allResources);
  } else {
    bool old =  SetStatus (ownJid, remoteJid, remoteResource, 
                           stype, statusText, allResources);
    if (!old) {
      AddContact (remoteJid, remoteName, remoteResource, 
                  ownId, stype, statusText);
    }
  }
}

void
ContactListModel::AddAccount (const QString & id)
{
  QStandardItem * newAccount = FindAccountGroup (id);
  QModelIndex newIndex = indexFromItem (newAccount);
  emit NewAccountIndex (newIndex);
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
                       const QString & remoteName,
                       const QString & res, 
                       const QString & friendOf,
                       QXmppPresence::Status::Type stype,
                       const QString & statusText)
{
  QStringList parts = friendOf.split ('/',QString::SkipEmptyParts);
  QString ownId = parts.at(0);
  QStandardItem * groupItem = FindAccountGroup (ownId);
  QStandardItem * nameItem = FindContactGroup (groupItem, id);
  if (!nameItem) {
    return;
  }

  // add nick 
  QStandardItem * nickItem = groupItem->child (nameItem->row(), 1);
  if (nickItem) {
    nickItem->setText (remoteName);
  }
  
  // add resources and state 
  QStandardItem * stateItem = new QStandardItem (QString("?"));
  stateItem->setData (stateTag, tagData);
  QStandardItem * resourceItem = new QStandardItem (res);
  resourceItem->setData (resTag, tagData);
  QList <QStandardItem*> row;
  row << stateItem;
  row << resourceItem;
  nameItem->appendRow (row);
  QString stext (statusText);
  if (stext.length() == 0) {
    stext = StatusName (stype);
  }
  stateItem->setText (stext);
  stateItem->setIcon (StatusIcon (stype));
  stateItem->setData (IsOnline (stype), statusData);
}


void
ContactListModel::HighlightStatus ()
{
  QStandardItem * accountHead, *contactHead;
  int nrows = rowCount ();
  for (int row = 0; row < nrows; row++) {
    accountHead = item (row, 0);
    if (accountHead) {
      int narows = accountHead->rowCount();
      for (int arow = 0; arow < narows; arow++) {
        contactHead = accountHead->child (arow,0);
        if (contactHead) {
          HighlightContactStatus (contactHead);
        }
      }
    }
  }
}

void
ContactListModel::HighlightContactStatus (QStandardItem *item)
{
  bool isOnline (false);
  if (item) {
    QStandardItem * chase;
    int nrows = item->rowCount ();
    int ncols = item->columnCount ();
    for (int row = 0; row < nrows; row++) {
      for (int col = 0; col < ncols; col++) {
        chase = item->child (row,col);
        if (chase && (chase->data(tagData).toString() == stateTag)) {
          isOnline |= chase->data(statusData).toBool ();
        }
      }
    }
    if (isOnline) {
      item->setForeground (QBrush (presentColor));
    } else {
      item->setForeground (QBrush (absentColor));
    }
  }
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
    rowHead->setData (QString("accounthead"), tagData);
    appendRow (rowHead);
    return rowHead;
  } else {
    return 0;
  }
}

QStandardItem *
ContactListModel::FindContactGroup (QStandardItem * accountHead,
                                    QString         contactJid,
                                    bool            makeit)
{
  if (!accountHead) {
    return 0;
  }
  int row(0);
  QStandardItem * chase;
  if (accountHead->hasChildren()) {
    for (row = 0; row < accountHead->rowCount(); row++) {
      for (int col = 0; col < accountHead->columnCount(); col++) {
        chase = accountHead->child (row,col);
        if (!chase) {
          continue;
        }
        QString tag = chase->data(tagData).toString();
        if (tag == nameTag) {
          if (chase->text() == contactJid) {
            return chase;
          }
        }
      }
    }
  }
  if (makeit) {
    QStandardItem * nameItem = new QStandardItem (contactJid);
    QStandardItem * nickItem = new QStandardItem (QString("?"));
    nameItem->setData (nameTag, tagData);
    nickItem->setData (nickTag, tagData);
    QList<QStandardItem*> newItems;
    newItems << nameItem << nickItem;
    accountHead->appendRow (newItems);
    return nameItem;
  } else {
    return 0;
  } 
}


} // namespace

