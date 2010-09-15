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

#include "cert-list-edit.h"
#include <QSslCertificate>
#include <QDateTime>
#include <QDebug>

namespace egalite 
{

CertListEdit::CertListEdit (QWidget *parent)
  :QDialog (parent),
   keepStateRole (Qt::UserRole+1),
   tagType (Qt::UserRole+2),
   pemData (Qt::UserRole+3),
   nickIndex (1),
   pemIndex (3)
{
  ui.setupUi (this);
  connect (ui.exitButton, SIGNAL (clicked()),
           this, SLOT (Done ()));
  connect (ui.applyButton, SIGNAL (clicked()),
           this, SLOT (Apply ()));
  ui.certListView->setModel (&certModel);
  connect (ui.certListView, SIGNAL (doubleClicked (const QModelIndex &)),
           this, SLOT (Picked (const QModelIndex &)));
}

void
CertListEdit::EditWhitelist ()
{
  setWindowTitle (tr("White Listed Certificates"));
  tableType = CertStore::Remote_White;
  otherTableType = CertStore::Remote_Black;
  UpdateList ();
  show ();
}

void
CertListEdit::EditBlacklist ()
{
  setWindowTitle (tr("Black Listed Certificates"));
  tableType = CertStore::Remote_Black;
  otherTableType = CertStore::Remote_White;
  UpdateList ();
  show ();
}

void
CertListEdit::UpdateList ()
{
  QStringList idlist = CertStore::IF().CertList (tableType);
  SetModelData (idlist);
}

void
CertListEdit::Done ()
{
  accept ();
}

void
CertListEdit::SetModelData (const QStringList & idlist)
{
  certModel.clear ();
  SetHeaders ();
  for (int i=0; i<idlist.size(); i++) {
    QByteArray pem;
    bool havePem = CertStore::IF().GetRemotePem (tableType, idlist.at(i), pem);
    if (!havePem) {
      continue;
    }
    QSslCertificate  cert (pem);
    QList<QStandardItem*> newRow;
    QStandardItem * headItem = new QStandardItem (StateString (Cert_Keep));
    headItem->setData (QVariant (Cert_Keep),keepStateRole);
    headItem->setData (QVariant (Tag_State), tagType);
    QStandardItem * identItem = new QStandardItem (idlist.at(i));
    identItem->setData (QVariant (Tag_Id), tagType);
    QStandardItem * expireItem = new QStandardItem (
                           cert.expiryDate().toString(tr("ddd dd MMM yyyy")));
    expireItem->setData (QVariant (Tag_Expire), tagType);
    QStandardItem * pemItem  = new QStandardItem (
                           cert.subjectInfo (QSslCertificate::CommonName));
    pemItem->setData (QVariant (Tag_Pem), tagType);
    pemItem->setData (QVariant (pem), pemData);
    newRow << headItem << identItem << expireItem << pemItem;
    certModel.appendRow (newRow);
  }
}

void
CertListEdit::SetHeaders ()
{
  QStringList labels;
  labels << tr ("Status")
         << tr ("Nick")
         << tr ("Expires")
         << tr ("Common Name");
  certModel.setHorizontalHeaderLabels (labels);
}

void
CertListEdit::Picked (const QModelIndex &index)
{
  QStandardItem * item = certModel.itemFromIndex (index);
  if (item) {
    if (item->data (tagType).toInt() == Tag_State) {
      CertState oldState = CertState (item->data (keepStateRole).toInt ());
      CertState newState (oldState);
      if (oldState == Cert_Keep) {
        newState = Cert_Delete;
      } else if (oldState == Cert_Delete) {
        newState = Cert_Change;
      } else if (oldState == Cert_Change) {
        newState = Cert_Keep;
      }
      item->setData (QVariant (newState), keepStateRole);
      item->setText (StateString (newState));
    }
  }
}

QString
CertListEdit::StateString ( CertState st)
{
  switch (st) {
  case Cert_Keep:
    return tr ("keep");
    break;
  case Cert_Delete:
    return tr ("erase");
    break;
  case Cert_Change:
    if (otherTableType == CertStore::Remote_White) {
       return tr ("-> white");
    } else {
       return tr ("-> black");
    }
    break;
  default:
    break;
  }
  return QString ("??");
}

void
CertListEdit::Apply ()
{
  int nrows = certModel.rowCount ();
  for (int r=0; r<nrows; r++) {
    QStandardItem * headItem = certModel.item (r,0);
    if (headItem) {
      CertState state = CertState (headItem->data (keepStateRole).toInt());
      if (headItem->data (tagType).toInt() == Tag_State
          && state != Cert_Keep) {
        QStandardItem * identItem = certModel.item (r,nickIndex);
        QStandardItem * pemItem = certModel.item (r,pemIndex);
        if (identItem && pemItem
          && identItem->data (tagType).toInt() == Tag_Id
          && pemItem->data (tagType).toInt () == Tag_Pem) {
          QString nick = identItem->text ();
          QByteArray pem = pemItem->data (pemData).toByteArray ();
          if (state == Cert_Delete || state == Cert_Change) {
            CertStore::IF().RemoveCert (tableType, nick, pem);
          }
          if (state == Cert_Change) {
            CertStore::IF().StoreCert (otherTableType, nick, pem);
          }
        }
      }
    }
  }
  UpdateList ();
}

} // namespace

