#ifndef DIRECT_MESSAGE_H
#define DIRECT_MESSAGE_H

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
#include <QByteArray>
#include <map>

namespace egalite
{

class DirectMessage 
{
 
public:

  typedef std::map <QString, QString>  AttributeMap;

  DirectMessage ();

  QString Op ();
  QString Subop ();
  QString Attribute (const QString & key);

  void    Clear ();

  QByteArray & Data ();

  void SetOp (const QString & newOp = QString());
  void SetSubop (const QString & newSubop = QString());
  void SetAttribute (const QString & key, const QString & value);

  void AddData (const QByteArray & moreData);
  void SetData (const QByteArray & newData);
  void SetData (const char *data, int len);
  void ClearData ();

  AttributeMap  Attributes ();

private:


  AttributeMap    attributes;
  QByteArray      data;

};

} // namespace

#endif
