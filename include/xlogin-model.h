#ifndef EGALITE_LOGIN_MODEL_H
#define EGALITE_LOGIN_MODEL_H


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
#include <QMap>
#include <QVariant>
#include "xcontact-model.h"

namespace egalite
{
class XLoginItem
{
public:
  
  XLoginItem () {}
  XLoginItem (const QString & jid, const QString & name)
    :theJid (jid),
      theName (name)
  {}
  XLoginItem (const XLoginItem & other)
    :theJid (other.theJid),
     theName (other.theName)
  {}
  
  QString jid () const 
    { return theJid; }
  QString name () const 
    { return theName; }
  
  void setJid (const QString & jid)
    { theJid = jid; }
  void setName (const QString & name)
    { theName = name; }

private:
  
  QString theJid;
  QString theName;
  
};

class XLoginModel : public QAbstractListModel
{
Q_OBJECT
 
public:
  XLoginModel (QObject *parent=0);
  
  int rowCount(const QModelIndex &parent) const;
  QVariant data (const QModelIndex &index, int role) const;
  
  void clear ();
  
  Q_INVOKABLE QObject * contacts (const QString & jid) const;
  
  void addLogin (const QString & jid, const QString & name);
  void addContact (const QString & ownJid, 
                   const QString & otherJid,
                   const QString & otherName,
                   const QString & otherResource,
                   QXmppPresence::Status::Type  stype);
  
  void removeLogin (const QString & jid);
  void updateState (const QString & ownJid, 
                    const QString & otherLongId, 
                    const QString & otherName,
                    QXmppPresence::Status::Type stype);
  void updateStateAll (const QString & ownJid,
                       const QString & otherJid,
                       QXmppPresence::Status::Type stype);
  
private:
  
  enum DataType {
    Data_Jid = Qt::UserRole +1,
    Data_Name = Qt::UserRole +2
  };
  
  QList <XLoginItem>              logins;
  QMap <QString, XContactModel*>  contactModels;
  
};

} // namespace

#endif
