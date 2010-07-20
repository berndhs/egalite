
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
#include "pick-cert.h"
#include "cert-store.h"
#include <QDateTime>
#include <QDebug>

namespace egalite
{

PickCert::PickCert (QWidget * parent, QString title)
  :QDialog (parent),
   saveDialog (this),
   haveGoodCert (false)
{
  ui.setupUi (this);
  saveUi.setupUi (&saveDialog);
  setObjectName (QString("PickCert %1").arg(title));
  QString wintitle = windowTitle();
  setWindowTitle (wintitle + " " + title);

  connect (ui.acceptButton, SIGNAL (clicked()), this, SLOT (Accept()));
  connect (ui.storeButton, SIGNAL (clicked()), this, SLOT (AcceptStore()));
  connect (ui.rejectButton, SIGNAL (clicked()), this, SLOT (Reject()));
  connect (ui.nextButton, SIGNAL (clicked()), this, SLOT (Up()));
  connect (ui.nextButton, SIGNAL (clicked()), this, SLOT (Down()));

  connect (saveUi.saveButton, SIGNAL (clicked()), 
           &saveDialog, SLOT (accept ()));
  connect (saveUi.cancelButton, SIGNAL (clicked()), 
           &saveDialog, SLOT (reject ()));
}

void
PickCert::Pick (const QList <QSslCertificate> & clist,
                 bool & pickedOne,
                 QSslCertificate & pickedCert)
{
  certs = clist;
  if (haveGoodCert) {
    if (StillGood ()) {
      pickedOne = true;
      pickedCert = goodCert;
      return;
    }
    haveGoodCert = false;
  }
  if (HaveGoodSavedCert (clist, pickedCert)) {
    pickedOne = true;
    return;
  }
  ndx = 0;
  Display (ndx);
  show ();
  int userChoice = exec ();
  pickedCert = goodCert;
  pickedOne = ( userChoice == 1 );
}

void
PickCert::Accept ()
{
  haveGoodCert = true;
  goodCert = certs.at(ndx);
  done (1);
}

void
PickCert::AcceptStore ()
{
  Accept ();
  saveUi.nickEdit->setText (goodCert.subjectInfo 
                                  (QSslCertificate::CommonName));
  int saveit = saveDialog.exec ();
qDebug () << "AcceptStore " << saveit << " for " << saveUi.nickEdit->text();
  if (saveit == 1) {
    QString nick = saveUi.nickEdit->text ();
    QByteArray pem = goodCert.toPem ();
    emit SaveRemote (nick, pem);
  }
}

bool
PickCert::StillGood ()
{
  int c;
  for (c=0; c<certs.size(); c++) {
    if (goodCert == certs.at(c)) {
      QDateTime now (QDateTime::currentDateTime ());
      return (goodCert.effectiveDate() <= now
          && now <= goodCert.expiryDate ()) ;
    }
  }
  return false;
}

bool
PickCert::HaveGoodSavedCert (const QList <QSslCertificate> & clist,
                            QSslCertificate & pickedCert)
{
  QDateTime now (QDateTime::currentDateTime());
  QList<QSslCertificate>::const_iterator  cit;
  for (cit = clist.begin() ; cit != clist.end (); cit++) {
    if (cit->effectiveDate() <= now
        && now <= cit->expiryDate()) {
      QString nick;
      if (CertStore::IF().RemoteNick (cit->toPem(), nick)) {
        pickedCert = *cit;
        return true;
      }
    }
  }
  return false;
}

void
PickCert::Reject ()
{
  haveGoodCert = false;
  done (-1);
}

void
PickCert::Up ()
{
  if (ndx > 0) {
    ndx--;
    Display (ndx);
  }
}

void
PickCert::Down ()
{
  if (ndx < certs.size()-1) {
    ndx++;
    Display (ndx);
  }
}


void
PickCert::Display (int index)
{
  QSslCertificate cert = certs.at(index);
  QString  toplinePattern ("%1 (%2)");
  ui.certOrgLine->setText (toplinePattern
            .arg (cert.subjectInfo (QSslCertificate::Organization))
            .arg (cert.subjectInfo (QSslCertificate::CommonName)));
  QStringList lines;
  lines << tr("Organization: %1")
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
        << tr ("valid until: %1")
              .arg(cert.expiryDate().toString())
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
  ui.infoList->clear ();
  for (int l = 0; l<lines.size(); l++) {
    ui.infoList->addItem (lines.at(l));
  }
}

} // namespace

