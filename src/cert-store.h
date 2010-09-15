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
#include "ui_cert-edit.h"
#include "ui_list-direct.h"
#include "ui_contact-edit.h"
#include "cert-types.h"

class QStandardItemModel;
class QStandardItem;

namespace egalite
{


class CertGenerate;

/** \brief CertStore keep track of SSL identities defined by certificates, and
  *  keep track of contact addresses for direct contacts.
  *
  * This serves as a Singleton storage container. It also has a gui edit facility.
  * 
  */

class CertStore : public QObject
{
Q_OBJECT

public:

  enum  RemoteType { Remote_White = 0,
                     Remote_Black = 1
                   };


  static CertStore & IF();
  static CertStore * Object();

  /** \brief Load identity cert data and connect to parent Widget.
    * Only the first call to Init does anything, any additional calls are 
    * ignored.
    */
  void Init (QWidget *parent = 0);

  CertRecord  Cert (QString id);
  bool        HaveCert (QString id);
  QStringList NameList ();
  QString     ContactAddress (QString id);
  int         ContactPort    (QString id);
  QStringList ContactList ();
  QStringList CertList (RemoteType rt);
  bool        GetWhite (QByteArray pem, QString & nick);
  bool        IsBlocked (QByteArray pem);
  bool        GetRemotePem (RemoteType rt, const QString & nick,
                            QByteArray & pem);
  void RemoveCert ( RemoteType   rt,
                   const QString & nick,
                   const QByteArray & pem);
  void StoreCert ( RemoteType   rt,
                   const QString & nick,
                   const QByteArray & pem);

  bool        SaveAccount (QString jid, QString server, QString pass);
  bool        RecallAccount (QString jid, QString & server, QString &pass);
  bool        DeleteAccount (QString jid);
  QStringList AccountList ();

public slots:

  /** \brief CertDialog - gui to add/remove/edit identities. */
  void CertDialog ();
  /** \brief ContactDialog - gui to add/remove/edit contact addresses. */
  void ContactDialog ();
  void StoreWhite (const QString & nick, const QByteArray & pem);
  void StoreBlack (const QString & nick, const QByteArray & pem);
  void CreateCertificate ();

private slots:

  void SaveCerts ();
  void NewIdent ();
  void SaveIdent ();
  void DeleteIdent ();
  void EditIdent (bool isnew = false);
  void ShowCertDetails (bool showCooked);
  void LoadKey ();
  void LoadCert ();
  void PasteKey ();
  void PasteCert ();

  void SaveContacts ();
  void NewContact ();
  void DeleteContact ();

  void SelectIdentity (const QModelIndex &index);
  void ToggleView ();

  void StartNewCert (QString name, QString pass, 
                     QString keyPEM, QString certPEM);

private:

  CertStore ();

  void Connect ();
  void ReadDB ();
  void WriteCert (CertRecord certRec);
  void DeleteCert (QString id);
  void WriteCerts (const QString filename);
  void WriteContacts (const QString filename);
  void CheckExists (const QString filename);
  void CheckDBComplete (const QString filename);
  void MakeElement (const QString name);
  void RefreshContactMap ();
  void RefreshContactModel ();
  void StoreCert  (const QString & table,
                   const QString & nick,
                   const QByteArray & pem);
  void RemoveCert (const QString & table,
                   const QString & nick,
                   const QByteArray & pem);
  QStringList  CertList (const QString &table);

  QString ElementType (QString name);

  QWidget         *parentWidget;
  Ui_ListIdentity  uiListCert;
  QDialog         *certListDialog;
  Ui_EditCertEntry uiEditCert;
  QDialog         *certEditDialog;
  Ui_ContactEdit   uiContact;
  QDialog         *contactDialog;
  CertGenerate    *certGenerate;
  QString          dbFileName;
  CertMap          homeCertMap;
  ContactHostMap   contactHostMap;
  QSqlDatabase     certDB;
  QString          conName;
  CertRecord       currentRec;
  QSslCertificate  currentCert;
  bool             viewDetails;
  QModelIndex      currentIdentity;

  QStandardItemModel  *identityModel;
  QStandardItem       *editItem;
  QStandardItemModel  *contactModel;

  QStringList     dbElementList;

  QString         lastDirUsed;

  static CertStore * instance;

};

} // namespace

#endif
