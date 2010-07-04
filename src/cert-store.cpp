
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
  :QDialog (parent)
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
           this, SLOT (ClickedList (const QModelIndex &)));
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
  ReadDB (certFileName);
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

void
CertStore::ClickedList (const QModelIndex &index)
{
  QStandardItem *item = identListModel->itemFromIndex (index);
  if (item) { 
    CertRecord rec = certMap[item->text()];
  }
}

void
CertStore::SaveChanges ()
{
  WriteDB (certFileName);
}

void
CertStore::ReadDB (const QString filename)
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
  qDebug () << " make " << elem << " from file " << filename;
  QFile schemafile (filename);
  schemafile.open (QFile::ReadOnly);
  QByteArray createcommands = schemafile.readAll ();
  schemafile.close ();
  QString querytext (createcommands);
  QSqlQuery qry (certDB);
  qry.prepare (querytext);
  bool ok = qry.exec ();
  qDebug () << " ok: " << ok << " for " << createcommands;
}

} // namespace

