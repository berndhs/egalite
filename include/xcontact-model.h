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
#include <QStringList>
#include <QList>
#include "xcontact-login-model.h"

namespace egalite
{

class XContactItem
{
public:
 
  XContactItem () {}
  XContactItem (const QString & jid,
                const QString & name)
    :theJid (jid),
     theName (name)
  {}
  XContactItem (const XContactItem & other)
    :theJid (other.theJid),
     theName (other.theName),
     theLogins (),
     theIdentities (other.theIdentities)
  {
    theLogins.copyLogins (other.theLogins);
  }
  
  XContactItem & operator = (const XContactItem & other) 
  {
    if (&other != this) {
      theJid = other.theJid;
      theName = other.theName;
      theLogins.copyLogins (other.theLogins);
      theIdentities = other.theIdentities;
    }
    return *this;
  }
  
  QString jid () const { return theJid; }
  QString name () const { return theName; }
  
  void setJid (const QString & jid) { theJid = jid; }
  void setName (const QString & name) { theName = name; }
  
  int loginCount () const { return theLogins.rowCount(); }
  
  const XContactLoginModel & loginsRef () const 
    { return theLogins; }
  XContactLoginModel & loginsRef () 
    { return theLogins; }
  
  Q_INVOKABLE const XContactLoginModel * logins () const 
    { return &theLogins; }

  QStringList & identities () { return theIdentities; }
  const QStringList & identities () const { return theIdentities; }
  
  Q_INVOKABLE QString imageName (const int statusCode) const;
  
private:
  
  QString                      theJid;
  QString                      theName;
  XContactLoginModel           theLogins;
  QStringList                  theIdentities;
};

class XContactModel: public QAbstractListModel
{
Q_OBJECT

public:

  XContactModel (QObject *parent=0);
  
  int rowCount (const QModelIndex &parent = QModelIndex()) const;
  QVariant data (const QModelIndex &index, int role) const;
  
  const XContactLoginModel * loginModel (const QString & otherJid) const;
      
  void clear ();
  
  void addContact (const XContactItem & item);
  void removeContact (const QString & longId);
  void updateState (const QString & longOtherJid, 
                    const QString & myJid,
                    const QString & name, 
                    const QString & statusText,
                    QXmppPresence::Status::Type stype);
  void updateStateAll (const QString & longId, 
                    QXmppPresence::Status::Type stype);
  void removeMyJid (const QString & myJid);

  Q_INVOKABLE  QString imageName(const int statusCode) const;
  
signals:
  
  void newContact (const QString & jid);
  
private:

  enum Data_Types {
    Data_OtherJid = Qt::UserRole +1,
    Data_OtherName = Qt::UserRole +2,
    Data_LoginCount = Qt::UserRole +3,
    Data_MyJid = Qt::UserRole +4,
    Data_BestStatus = Qt::UserRole +5
  };
  
  int bestStatus (int row) const;
  
  static QMap<QXmppPresence::Status::Type,int> initStatusPriorities ();

  QList<XContactItem>  contacts;
 
};

} // namespace

#endif
