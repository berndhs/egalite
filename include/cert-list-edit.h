#ifndef CERT_LIST_EDIT_H
#define CERT_LIST_EDIT_H

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

#include <QDialog>
#include <QStandardItemModel>
#include "ui_cert-list.h"
#include "cert-store.h"

class QModelIndex;

namespace egalite
{

class CertListEdit : public QDialog
{
Q_OBJECT
public:

  CertListEdit (QWidget * parent=0);

public slots:

  void EditWhitelist ();
  void EditBlacklist ();

private slots:

  void Done ();
  void Picked (const QModelIndex & index);
  void Apply ();

private:

  enum CertState  {  Cert_BadState = 0,
                     Cert_Keep = 1,
                     Cert_Delete = 2,
                     Cert_Change = 3
                  };
  enum DataTag    {  Tag_None = 0,
                     Tag_State = 1,
                     Tag_Id = 2,
                     Tag_Expire = 3,
                     Tag_Pem = 4
                   };

  void       SetModelData (const QStringList & idlist);
  void       SetHeaders ();
  QString    StateString (CertState st);
  void       UpdateList ();

  Ui_CertList   ui;

  QStandardItemModel       certModel;
  CertStore::RemoteType    tableType;
  CertStore::RemoteType    otherTableType;
  int                      keepStateRole;
  int                      tagType;
  int                      pemData;
  int                      nickIndex;
  int                      pemIndex;
 
} ;

} // namespace

#endif
