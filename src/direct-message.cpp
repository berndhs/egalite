
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

#include "direct-message.h"

namespace egalite
{

DirectMessage::DirectMessage ()
{
  attributes["op"] = QString ();
  attributes["subop"] = QString ();
}

QString
DirectMessage::Op ()
{
  return attributes["op"];
}  

QString
DirectMessage::Subop ()
{
  return attributes["subop"];
}

QString
DirectMessage::Attribute (const QString & key)
{
  AttributeMap::iterator  ndx;
  ndx = attributes.find (key);
  if (ndx != attributes.end()) {
    return ndx->second;
  }
  return QString();
}

void
DirectMessage::SetOp (const QString & newOp)
{
  attributes["op"] = newOp;
}

void
DirectMessage::SetSubop (const QString & newSubop)
{
  attributes["subop"] = newSubop;
}

void
DirectMessage::SetAttribute (const QString & key, const QString & value)
{
  attributes[key] = value;
}

void
DirectMessage::AddData (const QByteArray & moreData)
{
  data.append (moreData);
}

void
DirectMessage::SetData (const QByteArray & newData)
{
  data = newData;
}

void 
DirectMessage::SetData (const char * data, int len)
{
  data = QByteArray::fromRawData(data, len);
}

QByteArray &
DirectMessage::Data ()
{
  return data;
}

void
DirectMessage::ClearData ()
{
  data.clear ();
}

DirectMessage::AttributeMap
DirectMessage::Attributes ()
{
  return attributes;
}

void
DirectMessage::Clear ()
{
  data.clear();
  attributes.clear ();
  SetOp ();
  SetSubop ();
}

} // namespace
