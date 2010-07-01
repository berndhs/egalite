#ifndef PICK_CERT_H
#define PICK_CERT_H

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
#include "ui_cert-candidate.h"
#include <QList>
#include <QSslCertificate>

namespace egalite
{

class PickCert : public QDialog
{
Q_OBJECT

public:

  PickCert (QWidget *parent);

  bool Pick (const QList <QSslCertificate> & clist);

private slots:

  void Up ();
  void Down ();
  void Reject ();
  void Accept ();

private:

  void Display (int index);
  bool StillGood ();

  Ui_CertCandidate      ui;

  QList <QSslCertificate>  certs;
  int                      ndx;
  QSslCertificate          goodCert;
  bool                     haveGoodCert;

};


} // namespace

#endif
