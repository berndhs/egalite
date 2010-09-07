
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

#include "direct-parser.h"
#include <QXmlStreamReader>
#include <QDebug>

namespace egalite
{

DirectParser::DirectParser (QObject *parent)
  :QObject(parent),
   bufLock (QMutex::NonRecursive),
   lastErr (-1),
   topTag ("egalite"),
   oldTopTag ("message")
{
}

DirectParser::~DirectParser ()
{
}

void
DirectParser::Start ()
{
  inbuf.open (QBuffer::ReadOnly);
  inbuf.seek (0);
}

int
DirectParser::Error ()
{
  return lastErr;
}

void
DirectParser::AddData (const QByteArray & moreBytes)
{
  bufLock.lock ();
  inbuf.buffer().append (moreBytes);
  bufLock.unlock ();
}

void
DirectParser::TryRead ()
{
  DirectMessage msg;
  if (Read (msg)) {
    emit Message (msg);
  }
}

void
DirectParser::Flush ()
{
  inbuf.buffer().clear();
  inbuf.seek (0);
}

void
DirectParser::Reset ()
{
  Flush ();
  lastErr = 0;
}

bool
DirectParser::Read (DirectMessage & msg)
{
  msg.Clear ();
  if (!bufLock.tryLock (5000)) {
    return false;
  }
  QXmlStreamReader              xread (&inbuf);
  QXmlStreamReader::TokenType   tokt;
  bool finished (false);
  bool complete (false);
  bool good (false);
  QString  topname;
  QString  version;
  qint64 offset (0);
  while (!finished) {
    tokt = xread.readNext ();
    offset = xread.characterOffset ();
    switch (tokt) {
    case QXmlStreamReader::NoToken :
      qDebug () << " no token found";
      finished = true; complete = false; good = false;
      lastErr = Proto_EmptyInput;
      break;
    case QXmlStreamReader::Invalid :
      qDebug () << " bad token";
      finished = true; complete = false; good = false;
      lastErr = Proto_BadTag;
      break;
    case QXmlStreamReader::StartElement:
      topname = xread.name().toString().toLower();
      qDebug () << " TOP NAME " << topname;
      if (topname == oldTopTag) {
        qDebug () << " not supporting old top tag";
        qDebug () << " text is " << inbuf.buffer();
        ParseOld (xread, msg, offset, complete, good);
        msg.SetAttribute ("version","0.1");
      } else if (topname == topTag) {
        version = xread.documentVersion ().toString();
        msg.SetAttribute ("version",version);
        ParseMessage (xread, msg, offset, complete, good);
      } else {
        finished = true;  complete = false; good = false;
        lastErr = Proto_WrongProtocol;
      }
      break;
    case QXmlStreamReader::EndDocument :
    case QXmlStreamReader::Characters:
    case QXmlStreamReader::EndElement:
      qDebug () << " character data";
      lastErr = Proto_OutOfOrder;
      finished = true; complete = false; good = false;
      break;
    case QXmlStreamReader::StartDocument:
    case QXmlStreamReader::Comment:
    case QXmlStreamReader::DTD:
    case QXmlStreamReader::EntityReference:
    case QXmlStreamReader::ProcessingInstruction:
      break;
    default:
      qDebug () << " unknown token type " << tokt;
      lastErr = Proto_Unknown;
      finished = true; complete = false; good = false;
      break;
    }
  }
  if (good && complete) {
    /// we have consumed a message, so get rid of the raw data
    /// so we can read the next message next time
    inbuf.buffer().remove (0,offset);
    inbuf.seek (0);
  } else {
    msg.Clear ();
  }
  bufLock.unlock ();
  return good && complete;
}

void
DirectParser::ParseMessage (QXmlStreamReader & xread, 
                            DirectMessage    & msg,
                            qint64           & offset,
                            bool             & complete,
                            bool             & good)
{
  QXmlStreamReader::TokenType tokt = xread.readNext ();
  offset = xread.characterOffset ();
  QXmlStreamAttributes atts;
  QString op;
  QString tag;
  switch (tokt) {
  case QXmlStreamReader::StartElement:
    atts = xread.attributes();
    tag = xread.name().toString();
    op = atts.value ("op").toString().toLower();
    msg.SetOp (op);
    if (tag == "cmd") {
      if (op == "xmpp") {
        ParseXmpp (xread, msg, offset, complete, good, atts);
      } else if (op == "ctl") {
        ParseCtl (xread, msg, offset, complete, good, atts);
      } else if (op == "sendfile") {
        ParseSendfile (xread, msg, offset, complete, good, atts);
      }
      xread.readNext ();
      offset = xread.characterOffset ();
      if (xread.tokenType() != QXmlStreamReader::EndElement
         || xread.name() != tag) {
        complete = false;
        good = false;
      }
    }
    break;
  default:
    complete = false;
    good = false;
    break;
  }
}

void
DirectParser::ParseXmpp (QXmlStreamReader & xread, 
                            DirectMessage    & msg,
                            qint64           & offset,
                            bool             & complete,
                            bool             & good,
                            QXmlStreamAttributes & atts)
{
  QString subop = atts.value("subop").toString().toLower();
  msg.SetSubop (subop);
  if (subop == "xmpp") {
    complete = true; good = true;
  }
  xread.readNext ();
  if (xread.isCharacters()) {
    xread.readNext ();
    msg.SetData (xread.text().toString().toUtf8());
  }
  offset = xread.characterOffset ();
  if (xread.tokenType() != QXmlStreamReader::EndElement) {
    complete = false; good = false;
  }
}

void
DirectParser::ParseCtl (QXmlStreamReader & xread, 
                            DirectMessage    & msg,
                            qint64           & offset,
                            bool             & complete,
                            bool             & good,
                            QXmlStreamAttributes & atts)
{
  QString subop = atts.value("subop").toString().toLower();
  msg.SetSubop (subop);
  if (subop == "heartbeat") {
    complete = true;
    good = true;
  }
  xread.readNext ();
  offset = xread.characterOffset ();
  if (xread.tokenType() != QXmlStreamReader::EndElement) {
    complete = false;
    good = false;
  }
}

void
DirectParser::ParseSendfile (QXmlStreamReader & xread, 
                            DirectMessage    & msg,
                            qint64           & offset,
                            bool             & complete,
                            bool             & good,
                            QXmlStreamAttributes & atts)
{
  QString subop = atts.value("subop").toString().toLower();
  msg.SetSubop (subop);
  bool valid (false);
  if (subop == "chunk-data") {
    xread.readNext ();
    if (xread.isCharacters()) {
      msg.SetData (xread.text().toString().toUtf8());
      valid = true;
      xread.readNext ();
    } 
    offset = xread.characterOffset ();
  } else {
    valid= ( subop == "sendreq"
          || subop == "goahead"
          || subop == "deny"
          || subop == "chunk-ack"
          || subop == "send-done"
          || subop == "abort"
           );
  }
  if (valid) {
    for (int i=0; i< atts.size(); i++) {
      msg.SetAttribute (atts[i].name().toString().toLower(),
                        atts[i].value().toString());
    }
    good = true;
    complete = true;
    offset = xread.characterOffset ();
  } 
}

void
DirectParser::ParseOld     (QXmlStreamReader & xread, 
                            DirectMessage    & msg,
                            qint64           & offset,
                            bool             & complete,
                            bool             & good)
{
  QXmlStreamReader::TokenType tokt = xread.readNext ();
  QXmlStreamAttributes atts;
  msg.SetOp ("xmpp");
  msg.SetSubop ("msg");
  xread.readNext (); // eat the characters
  xread.readNext (); // eat the end-element
  offset = xread.characterOffset ();
  QByteArray data = inbuf.buffer().left (offset);
  msg.SetData (data);
  complete = true;
  good = true;
}

} // namespace

