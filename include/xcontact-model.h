#ifndef EGALITE_XCONTACT_MODEL_H
#define EGALITE_XCONTACT_MODEL_H


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


#include <QAbstractListModel>
#include <QXmppPresence.h>
#include <QString>
#include <QList>

namespace egalite
{

class XContactItem
{
public:
 
  XContactItem () {}
  XContactItem (const QString & jid,
                const QString & name,
                const QString & resource,
                QXmppPresence::Status::Type presence)
    :theJid (jid),
     theName (name),
     theResource (resource),
     thePresence (presence)
  {}
  XContactItem (const XContactItem & other)
    :theJid (other.theJid),
     theName (other.theName),
     theResource (other.theResource),
     thePresence (other.thePresence)
  {}
  
  QString jid () const { return theJid; }
  QString name () const { return theName; }
  QString resource () const { return theResource; }
  QXmppPresence::Status::Type presence () const { return thePresence; }
  
  void setJid (const QString & jid) { theJid = jid; }
  void setName (const QString & name) { theName = name; }
  void setResource (const QString & resource) { theResource = resource; }
  void setPresence (QXmppPresence::Status::Type presence) 
    { thePresence = presence; }
  
private:
  
  QString                      theJid;
  QString                      theName;
  QString                      theResource;
  QXmppPresence::Status::Type  thePresence;
};

class XContactModel: public QAbstractListModel
{
Q_OBJECT

public:

  XContactModel (QObject *parent=0);
  
  int rowCount (const QModelIndex &parent) const;
  QVariant data (const QModelIndex &index, int role) const;
  
  void clear ();
  
  void addContact (const XContactItem & item);
  void removeContact (const QString & longId);
  void updateState (const QString & longId, const QString & name, 
                    QXmppPresence::Status::Type stype);
  void updateStateAll (const QString & longId, QXmppPresence::Status::Type stype);


private:

  enum Data_Types {
    Data_Jid = Qt::UserRole +1,
    Data_Name = Qt::UserRole +2,
    Data_Resource = Qt::UserRole +3,
    Data_Presence = Qt::UserRole +4
  };

  QList<XContactItem>  contacts;
  
};

} // namespace

#endif
