
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
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDebug>

using namespace deliberate;

namespace egalite
{

CertStore::CertStore (QWidget *parent)
  :QDialog (parent),
   viewDetails (false)
{
  ui.setupUi (this);
  Connect ();
  dbElementList << "certificates"
                << "uniqueident";

  identListModel = new QStandardItemModel (this);
  ui.identList->setModel (identListModel);
}

void
CertStore::Connect ()
{
  connect (ui.exitButton, SIGNAL (clicked()), this, SLOT (accept()));
  connect (ui.saveButton, SIGNAL (clicked()), this, SLOT (SaveChanges()));
  connect (ui.identList, SIGNAL (clicked (const QModelIndex &)),
           this, SLOT (SelectItem (const QModelIndex &)));

  connect (ui.newIdButton, SIGNAL (clicked()), this, SLOT (NewIdent()));
  connect (ui.saveIdButton, SIGNAL (clicked()), this, SLOT (SaveIdent()));
  connect (ui.changeViewButton, SIGNAL (clicked()),
           this, SLOT (ToggleView()));
}

void
CertStore::Init ()
{
  certFileName =  QDesktopServices::storageLocation 
              (QDesktopServices::DataLocation)
              + QDir::separator()
              + QString ("certificates.sql");
  certFileName = Settings().value ("files/certificates",certFileName).toString();
  Settings().setValue ("files/certificates",certFileName);

  conName = QString ("certificateDB");
  certDB = QSqlDatabase::addDatabase ("QSQLITE",conName);
  CheckExists (certFileName);
  certDB.setDatabaseName (certFileName);
  certDB.open ();
  ReadDB ();
}

void
CertStore::Dialog ()
{
  identListModel->clear ();
  CertMap::iterator  certit;
  for (certit = certMap.begin(); certit != certMap.end(); certit++) {
    QStandardItem *item = new QStandardItem (certit->first);
    item->setEditable (false);
    identListModel->appendRow (item);
  }
  exec ();
}

bool
CertStore::HaveCert (QString id)
{
  CertMap::const_iterator certit = certMap.find (id);
  return (certit != certMap.end());
}

CertRecord 
CertStore::Cert (QString id)
{
  CertMap::const_iterator certit = certMap.find (id);
  if (certit != certMap.end()) {
    return certit->second;
  } else {
    CertRecord empty;
    return empty;
  }
}

void
CertStore::SelectItem (const QModelIndex &index)
{
  editItem = identListModel->itemFromIndex (index);
  if (editItem) { 
    currentRec = certMap[editItem->text()];
    ui.nameEdit->setText (currentRec.Id());
    ui.keyEdit->setPlainText (currentRec.Key ());
    ui.certEdit->setPlainText (currentRec.Cert ());
    currentCert = QSslCertificate (currentRec.Cert().toAscii());
    viewDetails = false;
    ui.changeViewButton->setEnabled (true);
    ShowCertDetails (viewDetails);
  }
}

void
CertStore::ToggleView ()
{
  viewDetails = ! viewDetails;
  ShowCertDetails (viewDetails);
  ui.certEdit->setReadOnly (viewDetails);
}

void
CertStore::ShowCertDetails (bool showCooked)
{
  QSslCertificate cert = currentCert;
  QStringList lines;
  if (showCooked) {
    ui.certEdit->setReadOnly (true);
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
    ui.certEdit->setReadOnly (false);
    lines << currentRec.Cert();
  }
  QStringList::iterator lit;
  ui.certEdit->clear ();
  for (lit = lines.begin(); lit != lines.end(); lit++) {
    ui.certEdit->appendPlainText (*lit);
  }
}

void
CertStore::NewIdent ()
{
  ui.nameEdit->setText (tr("New Identity"));
  ui.keyEdit->clear ();
  ui.certEdit->clear ();
  ui.certEdit->setReadOnly (false);
  viewDetails = false;
  ui.changeViewButton->setEnabled (false);
}

void
CertStore::SaveIdent ()
{
  QString id = ui.nameEdit->text ();
  CertMap::iterator  certit = certMap.find (id);
  bool isnew = (certit == certMap.end());
  if (isnew) {
    editItem = new QStandardItem (id);
    identListModel->appendRow (editItem);
  }
  currentRec.SetId (id);
  currentRec.SetKey (ui.keyEdit->toPlainText ());
  currentRec.SetCert (ui.certEdit->toPlainText ());
  certMap [id] = currentRec;
  QModelIndex index = identListModel->indexFromItem (editItem);
  ui.identList->scrollTo (index);
  SelectItem (index);
}

void
CertStore::SaveChanges ()
{
  WriteDB (certFileName);
}

void
CertStore::ReadDB ()
{
  QSqlQuery allquery (certDB);
  allquery.exec (QString ("select * from certificates"));
  int identNdx = allquery.record().indexOf ("ident");
  int keyNdx   = allquery.record().indexOf ("privatekey");
  int certNdx  = allquery.record().indexOf ("pemcert");
  QString ident,key,cert;
  certMap.clear ();
  while (allquery.next()) {
    ident = allquery.value (identNdx).toString();
    key   = allquery.value (keyNdx).toString();
    cert  = allquery.value (certNdx).toString();
    CertRecord rec (ident, key, cert);
    certMap.insert (std::pair<QString,CertRecord>(ident,rec));
  }
}


void
CertStore::WriteDB (const QString filename)
{
  CheckDBComplete (filename);
  CertMap::iterator  certit;
  CertRecord  certRec;
  QString  qryString ("insert or replace into certificates "
                       " (ident, privatekey, pemcert) "
                       " values (?, ?, ?)");
  for (certit = certMap.begin(); certit != certMap.end(); certit++ ) {
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
}

} // namespace

