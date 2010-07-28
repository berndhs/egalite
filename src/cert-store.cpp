
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
 *****************************************************************/

#include "cert-store.h"
#include "deliberate.h"

#include <QDesktopServices>
#include <QStandardItemModel>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDateTime>
#include <QDebug>

using namespace deliberate;

namespace egalite
{

CertStore * CertStore::instance (0);

CertStore & 
CertStore::IF ()
{
  if (instance == 0) {
    instance = new CertStore;
  }
  return *instance;
}

CertStore *
CertStore::Object ()
{
  if (instance == 0) {
    instance = new CertStore;
  }
  return instance;
}

CertStore::CertStore ()
  :QObject (0),
   viewDetails (false),
   lastDirUsed ("")
{
  dbElementList << "identities"
                << "uniqueident"
                << "directcontacts"
                << "uniquenick"
                << "remotecerts"
                << "uniqueremote"
                << "uniqueremotecert";
}

void
CertStore::Connect ()
{
  connect (uiListCert.identList, SIGNAL (clicked (const QModelIndex &)),
           this, SLOT (SelectIdentity (const QModelIndex &)));
  connect (uiListCert.newButton, SIGNAL (clicked()), 
           this, SLOT (NewIdent()));
  connect (uiListCert.editButton, SIGNAL (clicked()),
           this, SLOT (EditIdent()));
  connect (uiListCert.closeButton, SIGNAL (clicked()),
           certListDialog, SLOT (reject ()));
  connect (uiEditCert.closeButton, SIGNAL (clicked()), 
           certEditDialog, SLOT (reject ()));
  connect (uiEditCert.saveButton, SIGNAL (clicked()), 
           this, SLOT (SaveIdent()));
  connect (uiEditCert.changeViewButton, SIGNAL (clicked()),
           this, SLOT (ToggleView()));
  connect (uiEditCert.loadKeyButton , SIGNAL (clicked()), 
           this, SLOT (LoadKey ()));
  connect (uiEditCert.loadCertButton , SIGNAL (clicked()), 
           this, SLOT (LoadCert ()));

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
CertStore::Init (QWidget *parentWidget)
{
  bool initDone (false);
  if (initDone) {
    return;
  }
  initDone = true;
  certListDialog = new QDialog (parentWidget);
  uiListCert.setupUi (certListDialog);
  certEditDialog = new QDialog (parentWidget);
  uiEditCert.setupUi (certEditDialog);
  contactDialog = new QDialog (parentWidget);
  uiContact.setupUi (contactDialog);
  Connect ();
  identityModel = new QStandardItemModel (certEditDialog);
  uiListCert.identList->setModel (identityModel);
  contactModel = new QStandardItemModel (contactDialog);
  uiContact.addressTable->setModel (contactModel);
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
  identityModel->clear ();
  currentIdentity = QModelIndex();
  CertMap::iterator  certit;
  for (certit = homeCertMap.begin(); certit != homeCertMap.end(); certit++) {
    QStandardItem *item = new QStandardItem (certit->first);
    item->setEditable (false);
    identityModel->appendRow (item);
  }
  QStandardItem *firstItem = identityModel->item (0,0);
  if (firstItem) {
    QModelIndex index = identityModel->indexFromItem (firstItem);
    uiListCert.identList->scrollTo (index);
    currentIdentity = index;
  }
  certListDialog->show ();
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
  ContactHostMap::iterator addrit = contactHostMap.find (id);
  if (addrit != contactHostMap.end()) {
    return addrit->second.Address();
  } else {
    return QString ("::1");
  }
}

int
CertStore::ContactPort (QString id)
{
  Q_UNUSED (id);
  return 0;
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
  ContactHostMap::const_iterator addrit;
  for (addrit = contactHostMap.begin (); addrit != contactHostMap.end ();
       addrit++) {
    list << addrit->first;
  }
  return list;
}

void
CertStore::NewContact ()
{
  int newRow = contactModel->rowCount ();
  QStandardItem *item = new QStandardItem (tr("New Contact"));
  contactModel->setItem (newRow, 0, item);
  item = new QStandardItem (tr("::1"));
  contactModel->setItem (newRow, 1, item);
  item = new QStandardItem (tr("0"));
  contactModel->setItem (newRow, 2, item);
}

void
CertStore::DeleteContact ()
{
  QModelIndex current = uiContact.addressTable->currentIndex ();
  int row = current.row ();
  contactModel->removeRow (row);
  RefreshContactMap ();
}

void
CertStore::LoadKey ()
{
  QString keyval;
  QString filename;
  bool    isok;
  filename = QFileDialog::getOpenFileName (certEditDialog, tr("Select Key File"),
                lastDirUsed);
  if (filename.length() > 0) {
    QFile  keyfile (filename);
    isok = keyfile.open (QFile::ReadOnly);
    if (isok) {
      QByteArray keydata = keyfile.readAll();
      keyval = QString (keydata);
      uiEditCert.keyEdit->setPlainText (keyval);
      lastDirUsed = QFileInfo (keyfile).absolutePath();
    }
  }
}

void
CertStore::LoadCert ()
{
  QString certval;
  QString filename;
  bool    isok;
  filename = QFileDialog::getOpenFileName (certEditDialog, 
                tr("Select Certificate File"),
                lastDirUsed);
  if (filename.length() > 0) {
    QFile  certfile (filename);
    isok = certfile.open (QFile::ReadOnly);
    if (isok) {
      QByteArray certdata = certfile.readAll();
      certval = QString (certdata);
      uiEditCert.certEdit->setPlainText (certval);
      lastDirUsed = QFileInfo (certfile).absolutePath();
    }
  }
}

void
CertStore::SaveContacts ()
{
  WriteContacts (dbFileName);
}

void
CertStore::SelectIdentity (const QModelIndex &index)
{
  editItem = identityModel->itemFromIndex (index);
  if (editItem) { 
    currentRec = homeCertMap[editItem->text()];
    uiEditCert.nameEdit->setText (currentRec.Id());
    uiEditCert.keyEdit->setPlainText (currentRec.Key ());
    uiEditCert.certEdit->setPlainText (currentRec.Cert ());
    currentCert = QSslCertificate (currentRec.Cert().toAscii());
    viewDetails = false;
    uiEditCert.changeViewButton->setEnabled (true);
    ShowCertDetails (viewDetails);
    currentIdentity = index;
  }
}

void
CertStore::ToggleView ()
{
  viewDetails = ! viewDetails;
  ShowCertDetails (viewDetails);
  uiEditCert.certEdit->setReadOnly (viewDetails);
}

void
CertStore::ShowCertDetails (bool showCooked)
{
  QSslCertificate cert = currentCert;
  QStringList lines;
  if (showCooked) {
    uiEditCert.certEdit->setReadOnly (true);
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
        << tr("Valid from: %1")
              .arg(cert.effectiveDate().toString ())
        << tr("Valid to: %1")
              .arg(cert.expiryDate().toString ())
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
    uiEditCert.certEdit->setReadOnly (false);
    lines << currentRec.Cert();
  }
  QStringList::iterator lit;
  uiEditCert.certEdit->clear ();
  for (lit = lines.begin(); lit != lines.end(); lit++) {
    uiEditCert.certEdit->appendPlainText (*lit);
  }
}

void
CertStore::NewIdent ()
{
  uiEditCert.nameEdit->setText (tr("New Identity"));
  uiEditCert.keyEdit->clear ();
  uiEditCert.certEdit->clear ();
  uiEditCert.certEdit->setReadOnly (false);
  viewDetails = false;
  uiEditCert.changeViewButton->setEnabled (false);
  EditIdent (true);
}

void
CertStore::EditIdent (bool isnew)
{
  if (isnew || currentIdentity.isValid ()) {
    certListDialog->accept ();
    certEditDialog->show ();
  }
}

void
CertStore::SaveIdent ()
{
  QString id = uiEditCert.nameEdit->text ();
  CertMap::iterator  certit = homeCertMap.find (id);
  bool isnew = (certit == homeCertMap.end());
  if (isnew) {
    editItem = new QStandardItem (id);
    identityModel->appendRow (editItem);
  }
  currentRec.SetId (id);
  currentRec.SetPassword (uiEditCert.passwordEdit->text ());
  currentRec.SetKey (uiEditCert.keyEdit->toPlainText ());
  currentRec.SetCert (uiEditCert.certEdit->toPlainText ());
  homeCertMap [id] = currentRec;
  QModelIndex index = identityModel->indexFromItem (editItem);
  uiListCert.identList->scrollTo (index);
  SelectIdentity (index);
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
  certQuery.exec (QString ("select * from identities"));
  int identNdx = certQuery.record().indexOf ("ident");
  int passNdx  = certQuery.record().indexOf ("password");
  int keyNdx   = certQuery.record().indexOf ("privatekey");
  int certNdx  = certQuery.record().indexOf ("pemcert");
  QString ident,password,key,cert;
  homeCertMap.clear ();
  while (certQuery.next()) {
    ident = certQuery.value (identNdx).toString();
    password = certQuery.value (passNdx).toString();
    key   = certQuery.value (keyNdx).toString();
    cert  = certQuery.value (certNdx).toString();
    CertRecord rec (ident, password, key, cert);
    homeCertMap.insert (std::pair<QString,CertRecord>(ident,rec));
  }

  QSqlQuery contactQuery (certDB);
  contactQuery.exec (QString ("select * from directcontacts"));
  int nickNdx = contactQuery.record().indexOf ("nick");
  int addrNdx = contactQuery.record().indexOf ("address");
  int portNdx = contactQuery.record().indexOf ("port");
  QString nick, addr;
  int     port;
  contactHostMap.clear ();
  while (contactQuery.next()) {
    nick = contactQuery.value (nickNdx).toString();
    addr = contactQuery.value (addrNdx).toString();
    port = contactQuery.value (portNdx).toInt ();
    contactHostMap [nick] = ContactHost (nick, addr, port);
  }
}


void
CertStore::WriteCerts (const QString filename)
{
  CheckDBComplete (filename);
  CertMap::iterator  certit;
  CertRecord  certRec;
  QString  qryString ("insert or replace into identities "
                       " (ident, password, privatekey, pemcert) "
                       " values (?, ?, ?, ?)");
  for (certit = homeCertMap.begin(); certit != homeCertMap.end(); certit++ ) {
    certRec = certit->second;
    QSqlQuery qry (certDB);
    qry.prepare (qryString);
    qry.bindValue (0,QVariant (certRec.Id ()));
    qry.bindValue (1,QVariant (certRec.Password ()));
    qry.bindValue (2,QVariant (certRec.Key ()));
    qry.bindValue (3,QVariant (certRec.Cert ()));
    qry.exec ();
  }
}

void
CertStore::WriteContacts (const QString filename)
{

  RefreshContactMap ();
  CheckDBComplete (filename);
  ContactHostMap::iterator  addrit;
  QString nick, addr;
  int     port;
  QString  qryString ("insert or replace into directcontacts "
                       " (nick, address, port) "
                       " values (?, ?, ?)");
  for (addrit = contactHostMap.begin(); 
       addrit != contactHostMap.end(); 
       addrit++ ) {
    QSqlQuery qry (certDB);
    qry.prepare (qryString);
    nick = addrit->first;
    addr = addrit->second.Address();
    port = addrit->second.Port ();
    qry.bindValue (0,QVariant (nick));
    qry.bindValue (1,QVariant (addr));
    qry.bindValue (2,QVariant (port));
    qry.exec ();
  }
}

void
CertStore::StoreRemote (const QString &nick, const QByteArray &pem)
{ 
  CheckDBComplete (dbFileName);
  QString  qryString ("insert or replace into remotecerts "
                       " (ident, pemcert) "
                       " values (?, ?)");
  QSqlQuery qry (certDB);
  qry.prepare (qryString);
  qry.bindValue (0, QVariant (nick));
  qry.bindValue (1, QVariant (QString (pem)));
  qry.exec ();
}

bool
CertStore::RemoteNick (QByteArray  pem, QString & nick)
{
  QString qryString = QString ("select ident from remotecerts "
                      "where pemcert = \"%1\"").arg (QString(pem));
  QSqlQuery query (certDB);
  bool ok = query.exec (qryString);
qDebug () << " query good " << ok << " for RemoteNick " << QString(pem).left (40);
  if (ok && query.next()) {
    nick = query.value (0).toString();
qDebug () << " found nick " << nick;
    return true;
  } else {
    return false;
  }
}

void
CertStore::RefreshContactModel ()
{
  contactModel->clear ();
  QStringList labels ;
  labels << tr ("Name")
         << tr ("Address")
         << tr ("Port");
  contactModel->setHorizontalHeaderLabels (labels);
qDebug () << " headers set to " << labels;
  ContactHostMap::iterator addrit;
  QStandardItem  *nickItem, *addrItem, *portItem;
  int row (0);
  for (addrit = contactHostMap.begin (), row=0; 
       addrit != contactHostMap.end();
       addrit++, row++) {
    nickItem = new QStandardItem (addrit->first);
    addrItem = new QStandardItem (addrit->second.Address());
    portItem = new QStandardItem (QString::number (addrit->second.Port()));
    contactModel->setItem (row,0,nickItem);
    contactModel->setItem (row,1,addrItem);
    contactModel->setItem (row,2,portItem);
  }
}

void
CertStore::RefreshContactMap ()
{
  contactHostMap.clear ();
  int row(0);
  QStandardItem *nickItem, *addrItem, *portItem;
  QString        nick,      addr;
  int            port (0);
  int numrows = contactModel->rowCount ();
  for (row = 0; row<numrows; row++) { 
    nickItem = contactModel->item (row,0);
    if (nickItem) {
      nick = nickItem->text ();
    }
    addrItem = contactModel->item (row, 1);
    if (addrItem) {
      addr = addrItem->text ();
    }
    portItem = contactModel->item (row, 2);
    if (portItem) {
      port = portItem->text().toInt ();
    }
    if (nickItem && addrItem) {
      ContactHost remoteHost (nick, addr, port);
      contactHostMap [nick] = remoteHost;
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
  QString filename = QString (":/schemas/%1.sql").arg (elem);
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

