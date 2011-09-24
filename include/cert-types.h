#ifndef CERT_TYPES_H
#define CERT_TYPES_H

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

namespace egalite
{

class CertRecord 
{
public:

   CertRecord (QString id, QString pa, QString pk, QString c)
      :ident (id), password (pa), privateKey (pk), cert (c) {}

   CertRecord ():ident(""),password(""),privateKey(""), cert (""){}

   QString Id () { return ident; }
   QString Password () { return password; }
   QString Key () { return privateKey; }
   QString Cert () { return cert; }

   void SetId (QString s) { ident = s; }
   void SetPassword (QString s) { password = s; }
   void SetKey (QString s) { privateKey = s; }
   void SetCert (QString s) { cert = s; }

private:

    QString    ident;
    QString    password;
    QString    privateKey;
    QString    cert;

};

class ContactHost
{
public:

  ContactHost (QString id, QString ad, int pt)
    :ident (id), addr (ad), port (pt) {}
  ContactHost ()
    :ident (""), addr ("::1"), port (0) {}

  QString Id () { return ident; }
  QString Address () { return addr; }
  int     Port () { return port; }

  void SetId (QString i) { ident = i; }
  void SetAddress (QString a) { addr = a; }
  void SetPort (int p) { port = p; }

private:

  QString ident;
  QString addr;
  int     port;
  
};

typedef std::map <QString, CertRecord> CertMap;
typedef std::map <QString, ContactHost>    ContactHostMap;

} // namespace

#endif
