#ifndef EGALITE_XCONTACT_LOGIN_MODEL_H
#define EGALITE_XCONTACT_LOGIN_MODEL_H


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

class XContactLoginItem
{
public:
 
  XContactLoginItem () {}
  XContactLoginItem (const QString & resource,
                const QString & statusText,
                QXmppPresence::Status::Type presence)
    :theStatus (statusText),
     theResource (resource),
     thePresence (presence)
  {}
  XContactLoginItem (const XContactLoginItem & other)
    :theStatus (other.theStatus),
     theResource (other.theResource),
     thePresence (other.thePresence)
  {}
  
  QString status () const { return theStatus; }
  QString resource () const { return theResource; }
  QXmppPresence::Status::Type presence () const { return thePresence; }
  
  void setStatus (const QString & status) { theStatus = status; }
  void setResource (const QString & resource) { theResource = resource; }
  void setPresence (QXmppPresence::Status::Type presence) 
    { thePresence = presence; }
  
private:
  
  QString                      theStatus;
  QString                      theResource;
  QXmppPresence::Status::Type  thePresence;
};

class XContactLoginModel: public QAbstractListModel
{
Q_OBJECT

public:

  XContactLoginModel (QObject *parent=0);
  
  int rowCount (const QModelIndex &parent = QModelIndex()) const;
  QVariant data (const QModelIndex &index, int role) const;
  
  void clear ();
  
  void addLogin (const XContactLoginItem & item);
  void removeLogin (const QString & resource);
  void updateState (const QString & resource,
                    const QString & status,
                    QXmppPresence::Status::Type stype);
  void updateStateAll (QXmppPresence::Status::Type stype);
  
  void copyLogins (const XContactLoginModel & other);
  
  QList<XContactLoginItem>  & itemsRef () { return logins; }


private:

  enum Data_Types {
    Data_Resource = Qt::UserRole +1,
    Data_Status = Qt::UserRole +2,
    Data_Presence = Qt::UserRole +3,
    Data_IconName = Qt::UserRole +4
  };
  
  QString iconFileName (QXmppPresence::Status::Type stype) const;

  QList<XContactLoginItem>  logins;
  
};

} // namespace

#endif
