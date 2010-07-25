#ifndef CONTACT_LIST_MODEL_H
#define CONTACT_LIST_MODEL_H

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

#include <QStandardItemModel>
#include <QXmppPresence.h>

namespace egalite
{

class ContactListModel : public QStandardItemModel 
{
Q_OBJECT

public:

  ContactListModel (QObject * parent=0);

  void Setup ();

  void AddAccount (const QString & id);  /// when logging in call this
  void RemoveAccount (const QString & id);
  void UpdateState (const QString & ownId,
                    const QString & remoteId,
                    const QXmppPresence::Status & status);
  void AddContact (const QString & accountId,  /// who are we
                   const QString & remoteId,   /// who is our friend
                   const QString & loginId,    /// where is friend logged in
                   QXmppPresence::Status::Type  stype, /// on/off/sleeping
                   const QString & statusText);
  bool SetStatus (const QString & ownId,
                  const QString & remoteId,
                  const QString & resource,
                     QXmppPresence::Status::Type stype,
                  const QString & statusText);

  
public slots:

  void PickedItem (const QModelIndex & index );

signals:

  void StartServerChat (QString target);

private:

  QStandardItem * FindAccountGroup (QString accountName, bool makeit=true);
  QString StatusName (QXmppPresence::Status::Type stype);
  QIcon   StatusIcon (QXmppPresence::Status::Type stype);

  QString      iconPath;
  QString      iconSize;

  QString      nameTag;
  QString      resTag;
  QString      stateTag;

  //std::map <QString, ContactMap> serverContacts;

};

} // namespace


#endif
