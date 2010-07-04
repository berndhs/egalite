
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
#include <QDir>

using namespace deliberate;

namespace egalite
{

CertStore::CertStore (QWidget *parent)
  :QDialog (parent)
{
  ui.setupUi (this);
  Connect ();
}

void
CertStore::Connect ()
{
  connect (ui.exitButton, SIGNAL (clicked()), this, SLOT (accept()));
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

  ReadFile (certFileName);
}

void
CertStore::Dialog ()
{
  exec ();
}

void
CertStore::SaveChanges ()
{
  WriteFile (certFileName);
}

void
CertStore::ReadFile (const QString filename)
{
}


void
CertStore::WriteFile (const QString filename)
{
}

} // namespace

