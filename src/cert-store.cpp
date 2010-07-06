
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
 *  along with certDialog program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 *****************************************************************/

#include "cert-store.h"
#include "deliberate.h"

#include <QDesktopServices>
#include <QStandardItemModel>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDebug>

using namespace deliberate;

namespace egalite
{

CertStore::CertStore (QWidget *parent)
  :QObject (parent),
   viewDetails (false)
{
  certDialog = new QDialog (parent);
  uiCert.setupUi (certDialog);
  contactDialog = new QDialog (parent);
  uiContact.setupUi (contactDialog);
  Connect ();
  dbElementList << "certificates"
                << "uniqueident"
                << "directcontacts"
                << "uniquenick";

  identListModel = new QStandardItemModel (certDialog);
  uiCert.identList->setModel (identListModel);
  addressModel = new QStandardItemModel (contactDialog);
  uiContact.addressTable->setModel (addressModel);
}

void
CertStore::Connect ()
{
  connect (uiCert.exitButton, SIGNAL (clicked()), certDialog, SLOT (accept()));
  connect (uiCert.saveButton, SIGNAL (clicked()), this, SLOT (SaveCerts()));
  connect (uiCert.identList, SIGNAL (clicked (const QModelIndex &)),
           this, SLOT (SelectItem (const QModelIndex &)));

  connect (uiCert.newIdButton, SIGNAL (clicked()), this, SLOT (NewIdent()));
  connect (uiCert.saveIdButton, SIGNAL (clicked()), this, SLOT (SaveIdent()));
  connect (uiCert.changeViewButton, SIGNAL (clicked()),
           this, SLOT (ToggleView()));

  connect (uiContact.exitButton, SIGNAL (clicked()), 
           contactDialog, SLOT (accept ()));
  connect (uiContact.saveButton, SIGNAL (clicked()),
           this, SLOT (SaveContacts ()));
  connect (uiContact.newButton, SIGNAL (clicked()),
           this, SLOT (NewContact ()));
  connect (uiContact.deleteButton, SIGNAL (clicked()),
           this, SLOT (DeleteContact ()));
}

void
CertStore::Init ()
{
  dbFileName =  QDesktopServices::storageLocation 
              (QDesktopServices::DataLocation)
              + QDir::separator()
              + QString ("addressing.sql");
  dbFileName = Settings().value ("files/addressing",dbFileName).toString();
  Settings().setValue ("files/addressing",dbFileName);

  conName = QString ("addressingDB");
  certDB = QSqlDatabase::addDatabase ("QSQLITE",conName);
  CheckExists (dbFileName);
  certDB.setDatabaseName (dbFileName);
  certDB.open ();
  ReadDB ();
}

void
CertStore::CertDialog ()
{
  identListModel->clear ();
  CertMap::iterator  certit;
  for (certit = homeCertMap.begin(); certit != homeCertMap.end(); certit++) {
    QStandardItem *item = new QStandardItem (certit->first);
    item->setEditable (false);
    identListModel->appendRow (item);
  }
  certDialog->exec ();
}

void
CertStore::ContactDialog ()
{
  RefreshContactModel ();
  contactDialog->exec ();
}

bool
CertStore::HaveCert (QString id)
{
  CertMap::const_iterator certit = homeCertMap.find (id);
  return (certit != homeCertMap.end());
}

CertRecord 
CertStore::Cert (QString id)
{
  CertMap::const_iterator certit = homeCertMap.find (id);
  if (certit != homeCertMap.end()) {
    return certit->second;
  } else {
    CertRecord empty;
    return empty;
  }
}

QString
CertStore::ContactAddress (QString id)
{
  ContactAddrMap::const_iterator addrit = contactAddrMap.find (id);
  if (addrit != contactAddrMap.end()) {
    return addrit->second;
  } else {
    return QString ("::1");
  }
}

QStringList
CertStore::NameList ()
{
  QStringList list;
  CertMap::const_iterator certit;
  for (certit = homeCertMap.begin(); certit != homeCertMap.end(); certit++) {
    list << certit->first;
  }
  return list;
}

QStringList
CertStore::ContactList ()
{
  QStringList list;
  ContactAddrMap::const_iterator addrit;
  for (addrit = contactAddrMap.begin (); addrit != contactAddrMap.end ();
       addrit++) {
    list << addrit->first;
  }
  return list;
}

void
CertStore::NewContact ()
{
  int newRow = addressModel->rowCount ();
  QStandardItem *item = new QStandardItem (tr("New Contact"));
  addressModel->setItem (newRow, 0, item);
  item = new QStandardItem (tr("0::1"));
  addressModel->setItem (newRow, 1, item);
}

void
CertStore::DeleteContact ()
{
  QModelIndex current = uiContact.addressTable->currentIndex ();
  int row = current.row ();
  addressModel->removeRow (row);
  RefreshContactMap ();
}

void
CertStore::SaveContacts ()
{
  WriteContacts (dbFileName);
}

void
CertStore::SelectItem (const QModelIndex &index)
{
  editItem = identListModel->itemFromIndex (index);
  if (editItem) { 
    currentRec = homeCertMap[editItem->text()];
    uiCert.nameEdit->setText (currentRec.Id());
    uiCert.keyEdit->setPlainText (currentRec.Key ());
    uiCert.certEdit->setPlainText (currentRec.Cert ());
    currentCert = QSslCertificate (currentRec.Cert().toAscii());
    viewDetails = false;
    uiCert.changeViewButton->setEnabled (true);
    ShowCertDetails (viewDetails);
  }
}

void
CertStore::ToggleView ()
{
  viewDetails = ! viewDetails;
  ShowCertDetails (viewDetails);
  uiCert.certEdit->setReadOnly (viewDetails);
}

void
CertStore::ShowCertDetails (bool showCooked)
{
  QSslCertificate cert = currentCert;
  QStringList lines;
  if (showCooked) {
    uiCert.certEdit->setReadOnly (true);
    lines  << tr("Organization: %1")
              .arg(cert.subjectInfo(QSslCertificate::Organization))
        << tr("Subunit: %1")
              .arg(cert.subjectInfo(QSslCertificate::OrganizationalUnitName))
        << tr("Country: %1")
              .arg(cert.subjectInfo(QSslCertificate::CountryName))
        << tr("Locality: %1")
              .arg(cert.subjectInfo(QSslCertificate::LocalityName))
        << tr("State/Province: %1")
              .arg(cert.subjectInfo(QSslCertificate::StateOrProvinceName))
        << tr("Common Name: %1")
              .arg(cert.subjectInfo(QSslCertificate::CommonName))
        << QString("------------")
        << tr("Issuer Organization: %1")
              .arg(cert.issuerInfo(QSslCertificate::Organization))
        << tr("Issuer Unit Name: %1")
              .arg(cert.issuerInfo(QSslCertificate::OrganizationalUnitName))
        << tr("Issuer Country: %1")
              .arg(cert.issuerInfo(QSslCertificate::CountryName))
        << tr("Issuer Locality: %1")
              .arg(cert.issuerInfo(QSslCertificate::LocalityName))
        << tr("Issuer State/Province: %1")
              .arg(cert.issuerInfo(QSslCertificate::StateOrProvinceName))
        << tr("Issuer Common Name: %1")
              .arg(cert.issuerInfo(QSslCertificate::CommonName));
  } else {
    uiCert.certEdit->setReadOnly (false);
    lines << currentRec.Cert();
  }
  QStringList::iterator lit;
  uiCert.certEdit->clear ();
  for (lit = lines.begin(); lit != lines.end(); lit++) {
    uiCert.certEdit->appendPlainText (*lit);
  }
}

void
CertStore::NewIdent ()
{
  uiCert.nameEdit->setText (tr("New Identity"));
  uiCert.keyEdit->clear ();
  uiCert.certEdit->clear ();
  uiCert.certEdit->setReadOnly (false);
  viewDetails = false;
  uiCert.changeViewButton->setEnabled (false);
}

void
CertStore::SaveIdent ()
{
  QString id = uiCert.nameEdit->text ();
  CertMap::iterator  certit = homeCertMap.find (id);
  bool isnew = (certit == homeCertMap.end());
  if (isnew) {
    editItem = new QStandardItem (id);
    identListModel->appendRow (editItem);
  }
  currentRec.SetId (id);
  currentRec.SetKey (uiCert.keyEdit->toPlainText ());
  currentRec.SetCert (uiCert.certEdit->toPlainText ());
  homeCertMap [id] = currentRec;
  QModelIndex index = identListModel->indexFromItem (editItem);
  uiCert.identList->scrollTo (index);
  SelectItem (index);
}

void
CertStore::SaveCerts ()
{
  WriteCerts (dbFileName);
}

void
CertStore::ReadDB ()
{
  QSqlQuery certQuery (certDB);
  certQuery.exec (QString ("select * from certificates"));
  int identNdx = certQuery.record().indexOf ("ident");
  int keyNdx   = certQuery.record().indexOf ("privatekey");
  int certNdx  = certQuery.record().indexOf ("pemcert");
  QString ident,key,cert;
  homeCertMap.clear ();
  while (certQuery.next()) {
    ident = certQuery.value (identNdx).toString();
    key   = certQuery.value (keyNdx).toString();
    cert  = certQuery.value (certNdx).toString();
    CertRecord rec (ident, key, cert);
    homeCertMap.insert (std::pair<QString,CertRecord>(ident,rec));
  }

  QSqlQuery contactQuery (certDB);
  contactQuery.exec (QString ("select * from directcontacts"));
  int nickNdx = contactQuery.record().indexOf ("nick");
  int addrNdx = contactQuery.record().indexOf ("address");
  QString nick, addr;
  contactAddrMap.clear ();
  while (contactQuery.next()) {
    nick = contactQuery.value (nickNdx).toString();
    addr = contactQuery.value (addrNdx).toString();
    contactAddrMap [nick] = addr;
  }
}


void
CertStore::WriteCerts (const QString filename)
{
  CheckDBComplete (filename);
  CertMap::iterator  certit;
  CertRecord  certRec;
  QString  qryString ("insert or replace into certificates "
                       " (ident, privatekey, pemcert) "
                       " values (?, ?, ?)");
  for (certit = homeCertMap.begin(); certit != homeCertMap.end(); certit++ ) {
    certRec = certit->second;
    QSqlQuery qry (certDB);
    qry.prepare (qryString);
    qry.bindValue (0,QVariant (certRec.Id()));
    qry.bindValue (1,QVariant (certRec.Key()));
    qry.bindValue (2,QVariant (certRec.Cert()));
    qry.exec ();
  }
}

void
CertStore::WriteContacts (const QString filename)
{

  RefreshContactMap ();
  CheckDBComplete (filename);
  ContactAddrMap::iterator  addrit;
  QString nick, addr;
  QString  qryString ("insert or replace into directcontacts "
                       " (nick, address) "
                       " values (?, ?)");
  for (addrit = contactAddrMap.begin(); 
       addrit != contactAddrMap.end(); 
       addrit++ ) {
    QSqlQuery qry (certDB);
    qry.prepare (qryString);
    nick = addrit->first;
    addr = addrit->second;
    qry.bindValue (0,QVariant (nick));
    qry.bindValue (1,QVariant (addr));
    qry.exec ();
  }
}

void
CertStore::RefreshContactModel ()
{
  addressModel->clear ();
  ContactAddrMap::iterator addrit;
  QStandardItem  *nickItem, *addrItem;
  int row (0);
  for (addrit = contactAddrMap.begin (), row=0; 
       addrit != contactAddrMap.end();
       addrit++, row++) {
    nickItem = new QStandardItem (addrit->first);
    addrItem = new QStandardItem (addrit->second);
    addressModel->setItem (row,0,nickItem);
    addressModel->setItem (row,1,addrItem);
  }
}

void
CertStore::RefreshContactMap ()
{
  contactAddrMap.clear ();
  int row(0);
  QStandardItem *nickItem, *addrItem;
  QString        nick,      addr;
  int numrows = addressModel->rowCount ();
  for (row = 0; row<numrows; row++) { 
    nickItem = addressModel->item (row,0);
    if (nickItem) {
      nick = nickItem->text ();
    }
    addrItem = addressModel->item (row, 1);
    if (addrItem) {
      addr = addrItem->text ();
    }
    if (nickItem && addrItem) {
      contactAddrMap [nick] = addr;
    }
  }
}

void
CertStore::CheckExists (const QString filename)
{
  QFileInfo dbfileInfo (filename);
  if (!dbfileInfo.exists()) {
    QDir dir (dbfileInfo.absolutePath());
    dir.mkpath (dbfileInfo.absolutePath());
    QFile file (filename);
    file.open (QFile::ReadWrite);
    file.write (QByteArray (""));
    file.close();
  }
}

void
CertStore::CheckDBComplete (const QString filename)
{
  CheckExists (filename);
  QStringList::iterator  elit;
  QString el_name;
  for (elit = dbElementList.begin (); elit != dbElementList.end (); elit++) {
    el_name = *elit;
    QString kind = ElementType (el_name).toUpper();
    if (kind != "TABLE" && kind != "INDEX") {
      MakeElement (el_name);
    }
  }
}

QString
CertStore::ElementType (QString name)
{
  QSqlQuery query (certDB);
  QString cmdPat ("select * from main.sqlite_master where name=\"%1\"");
  QString cmd = cmdPat.arg (name);
  bool ok = query.exec (cmd);
  if (ok && query.next()) {
    QString tipo = query.value (0).toString();
    return tipo;
  }
  return QString ("none");
}

void
CertStore::MakeElement (const QString elem)
{
  QString filename = QString (":/schema-%1.sql").arg (elem);
  QFile schemafile (filename);
  schemafile.open (QFile::ReadOnly);
  QByteArray createcommands = schemafile.readAll ();
  schemafile.close ();
  QString querytext (createcommands);
  QSqlQuery qry (certDB);
  qry.prepare (querytext);
  qry.exec ();
qDebug () << " making " << querytext;
}

} // namespace

