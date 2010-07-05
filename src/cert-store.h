#ifndef CERT_STORE_H
#define CERT_STORE_H

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

#include <QString>
#include <map>
#include <QDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>
#include <QModelIndex>
#include <QSslCertificate>
#include "ui_cert-store.h"
#include "cert-types.h"

class QStandardItemModel;
class QStandardItem;

namespace egalite
{

/** \brief CertStore keep track of SSL identities defined by certificates.
  *
  * This serves as a storage container. It also has a gui edit facility.
  */

class CertStore : public QDialog
{
Q_OBJECT

public:

  CertStore (QWidget *parent = 0);

  void Connect ();

  /** \brief Init - load from cert storage file. */
  void Init ();

  CertRecord  Cert (QString id);
  bool        HaveCert (QString id);
  QStringList NameList ();

public slots:

  /** \brief Dialog - gui to add/remove/edit identities. */
  void Dialog ();

private slots:

  void SaveChanges ();
  void NewIdent ();
  void SaveIdent ();
  void ShowCertDetails (bool showCooked);

  void SelectItem (const QModelIndex &index);
  void ToggleView ();

private:
  
  void ReadDB ();
  void WriteDB (const QString filename);
  void CheckExists (const QString filename);
  void CheckDBComplete (const QString filename);
  void MakeElement (const QString name);

  QString ElementType (QString name);

  Ui_CertStore     ui;
  QString          certFileName;
  CertMap          certMap;
  QSqlDatabase     certDB;
  QString          conName;
  CertRecord       currentRec;
  QSslCertificate  currentCert;
  bool             viewDetails;

  QStandardItemModel  *identListModel;
  QStandardItem       *editItem;

  QStringList     dbElementList;

};

} // namespace

#endif
