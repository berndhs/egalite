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
#include <QColor>
#include <QTimer>
#include <QXmppPresence.h>

namespace egalite
{

class ContactListModel : public QStandardItemModel 
{
Q_OBJECT

public:

  ContactListModel (QObject * parent=0);

  void Setup ();
  /** when logging in call AddAccount */
  void AddAccount (const QString & id);  
  void RemoveAccount (const QString & id);
  void UpdateState (const QString & remoteName,
                    const QString & ownId,
                    const QString & remoteId,
                    const QXmppPresence::Status & status,
                    bool  allResources=false);
  void AddContact (const QString & accountId  /** who are they */  ,
                   const QString & remoteName /** who is their name */  ,
                   const QString & res        /** resource tag */ ,
                   const QString & friendOf   /** who is friend on this end */ ,
                   QXmppPresence::Status::Type  stype /** on/off/sleeping */ ,
                   const QString & statusText);
  bool SetStatus (const QString & ownId,
                  const QString & remoteId,
                  const QString & resource,
                     QXmppPresence::Status::Type stype,
                  const QString & statusText,
                  bool  allResources=false);

  
public slots:

  void PickedItem (const QModelIndex & index );
  void HighlightStatus ();

private slots:

  void HighlightContactStatus (QStandardItem * item);

signals:

  void StartServerChat (QString target);
  void NewAccountIndex (QModelIndex newIndex);

private:

  QStandardItem * FindAccountGroup (QString accountName, 
                                    bool makeit=true    /// create if not there
                                  );
  QStandardItem * FindContactGroup (QStandardItem * accountHead,
                                    QString         contactJid,
                                    bool            makeit=true);
  QString StatusName (QXmppPresence::Status::Type stype);
  QIcon   StatusIcon (QXmppPresence::Status::Type stype);
  bool    IsOnline   (QXmppPresence::Status::Type stype);
  void RemoveContact (const QString & ownId,
                      const QString & remoteId,
                      const QString & resource,
                      bool  allResources=false);

  QString      iconPath;
  QString      iconSize;

  QString      nameTag;
  QString      resTag;
  QString      stateTag;
  QString      nickTag;

  bool         discardOfflines;
  QColor       presentColor;
  QColor       absentColor;
  QTimer       cleanTimer;

  static int   tagData;
  static int   statusData;

};

} // namespace


#endif
