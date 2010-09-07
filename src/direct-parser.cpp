
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
  bool isopen = inbuf.open (QBuffer::ReadOnly);
  if (!isopen) {
    qDebug () << "Cannot open direct input buffer!";
  }
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
    qDebug () << " Good Direct REad, emit message with op " << msg.Op();
  } else {
    qDebug () << " Bad Direct Read, no emit";
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
  QString  bottomtag;
  QString  version;
  qint64 offset (0);
  while (!finished) {
    tokt = ReadNext (xread);
    offset = xread.characterOffset ();
    qDebug () << " Direct token " << xread.tokenString();
    switch (tokt) {
    case QXmlStreamReader::NoToken :
      qDebug () << " no token found";
      finished = true; complete = false; good = false;
      lastErr = Proto_EmptyInput;
      break;
    case QXmlStreamReader::Invalid :
      qDebug () << " bad token";
      qDebug () << " text until here: " << inbuf.buffer().left(offset);
      finished = true; complete = false; good = false;
      lastErr = Proto_BadTag;
      break;
    case QXmlStreamReader::StartElement:
      topname = xread.name().toString().toLower();
      qDebug () << " TOP NAME " << topname;
      if (topname == oldTopTag) {
        qDebug () << " text is " << inbuf.buffer();
        ParseOld (xread, msg, offset, complete, good);
        qDebug () << " After ParseOld good/complete " << good << "/" << complete;
        msg.SetAttribute ("version","0.1");
      } else if (topname == topTag) {
qDebug () << " topname is egalite " << topTag;
        version = xread.documentVersion ().toString();
        msg.SetAttribute ("version",version);
        ParseMessage (xread, msg, offset, complete, good);
      } else {
qDebug () << " topname unknown: " << topname;
        finished = true;  complete = false; good = false;
        lastErr = Proto_WrongProtocol;
      }
      break;
    case QXmlStreamReader::EndElement:
      bottomtag = xread.name().toString().toLower();
      if (bottomtag == topname) {
        finished = true;
      }
      break;
    case QXmlStreamReader::EndDocument :
      finished = true;
      break;
    case QXmlStreamReader::Characters:
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
qDebug () << " direct message top parse good/complete " 
          << good << "/" << complete;
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
qDebug () << " ParseMessage";
  QXmlStreamReader::TokenType tokt = ReadNext (xread);
  offset = xread.characterOffset ();
  QXmlStreamAttributes atts;
  QString op;
  QString tag;
  if (tokt == QXmlStreamReader::Characters) {
    tokt = ReadNext (xread);
    offset = xread.characterOffset();
  }
qDebug () << "    ParseMessage token " << xread.tokenString ();
  switch (tokt) {
  case QXmlStreamReader::StartElement:
    atts = xread.attributes();
    tag = xread.name().toString();
    op = atts.value ("op").toString().toLower();
qDebug () << "       ParseMessage op " << op;
    msg.SetOp (op);
    if (tag == "cmd") {
      if (op == "xmpp") {
        ParseXmpp (xread, msg, offset, complete, good, atts);
      } else if (op == "ctl") {
        ParseCtl (xread, msg, offset, complete, good, atts);
      } else if (op == "sendfile") {
        ParseSendfile (xread, msg, offset, complete, good, atts);
      }
      offset = xread.characterOffset ();
qDebug () << "  ParseMessage tail check " << xread.tokenString() << " tag "
               << xread.name () << " for " << tag;
qDebug () << "       stats is " << good << "/" << complete;
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
qDebug () << " ParseMessage return " << good << "/" << complete << " at "<<  xread.tokenString();
}

void
DirectParser::ParseXmpp (QXmlStreamReader & xread, 
                            DirectMessage    & msg,
                            qint64           & offset,
                            bool             & complete,
                            bool             & good,
                            QXmlStreamAttributes & atts)
{
qDebug () << " ParseXmpp ";
  QString subop = atts.value("subop").toString().toLower();
  msg.SetSubop (subop);
qDebug () << " ParseXmpp subop " << subop;
  if (subop == "msg") {
    complete = true; good = true;
  }
  ReadNext (xread);
  if (xread.isCharacters()) {
    msg.SetData (xread.text().toString().toUtf8());
qDebug () << " xmpp message has payload " << msg.Data();
    ReadNext (xread);
  }
  offset = xread.characterOffset ();
  if (xread.tokenType() != QXmlStreamReader::EndElement) {
    complete = false; good = false;
  }
qDebug () << " ParseXmpp return " << good << "/" << complete << " at "<<  xread.tokenString();
}

void
DirectParser::ParseCtl (QXmlStreamReader & xread, 
                            DirectMessage    & msg,
                            qint64           & offset,
                            bool             & complete,
                            bool             & good,
                            QXmlStreamAttributes & atts)
{
qDebug () << " ParseCtl " ;
  QString subop = atts.value("subop").toString().toLower();
  msg.SetSubop (subop);
qDebug () <<  " ParseCtl subop " << subop;
  if (subop == "heartbeat") {
    complete = true;
    good = true;
  }
  ReadNext (xread);
  if (ReadNext (xread) == QXmlStreamReader::Characters) {
    ReadNext (xread);
  }
  offset = xread.characterOffset ();
  if (xread.tokenType() != QXmlStreamReader::EndElement) {
    complete = false;
    good = false;
  }
qDebug () << " ParseCtl return " << good << "/" << complete << " at " << xread.tokenString();
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
qDebug () << " Parse Sendfile subop " << subop;
  bool valid (false);
  if (subop == "chunk-data") {
    ReadNext (xread);
    if (xread.isCharacters()) {
      msg.SetData (xread.text().toString().toUtf8());
      valid = true;
      ReadNext (xread);
    } 
    offset = xread.characterOffset ();
  } else {
    valid= ( subop == "sendreq"
          || subop == "goahead"
          || subop == "deny"
          || subop == "chunk-ack"
          || subop == "snd-done"
          || subop == "abort"
           );
    ReadNext (xread);
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
qDebug () << " ParseSendfile return " << good << "/" << complete << " at " <<  xread.tokenString();
}

void
DirectParser::ParseOld     (QXmlStreamReader & xread, 
                            DirectMessage    & msg,
                            qint64           & offset,
                            bool             & complete,
                            bool             & good)
{
  QXmlStreamReader::TokenType tokt = ReadNext (xread);
  QXmlStreamAttributes atts;
  msg.SetOp ("xmpp");
  msg.SetSubop ("msg");
  ReadNext (xread); // eat the characters
  ReadNext (xread); // eat the end-element
  offset = xread.characterOffset ();
  QByteArray data = inbuf.buffer().left (offset);
  msg.SetData (data);
  complete = true;
  good = true;
qDebug () << " ParseOld has data " << msg.Data ();
qDebug () << " ParseOld good/complete " << good << "/" << complete;
}

QXmlStreamReader::TokenType
DirectParser::ReadNext (QXmlStreamReader & xread)
{
  QXmlStreamReader::TokenType  tokt;
  tokt = xread.readNext ();
  qDebug () << "next token " << xread.tokenString ();
  qDebug () << "     name " << xread.name();
  return tokt;
}

} // namespace

