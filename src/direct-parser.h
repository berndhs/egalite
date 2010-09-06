#ifndef DIRECT_PARSER_H
#define DIRECT_PARSER_H

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

#include <QObject>
#include <QByteArray>
#include <QBuffer>
#include <QMutex>

#include "direct-message.h"

class QXmlStreamReader;
class QXmlStreamAttributes;

namespace egalite
{

enum ProtocolError 
    {
      Proto_NotStarted = -1,
      Proto_NoError    = 0,
      Proto_OutOfOrder = 1,
      Proto_BadTag     = 2,
      Proto_EmptyInput = 3,
      Proto_WrongProtocol = 4,
      Proto_BadOp      = 5,
      Proto_BadSubop   = 6,
      Proto_Unknown    = 9999
    };

class DirectParser : public QObject
{
Q_OBJECT

public:

  DirectParser (QObject * parent=0);
  ~DirectParser (); 

  bool Read (DirectMessage & msg);
  void Flush ();
  int  Error ();
  void Reset ();


public slots:

  void TryRead ();
  void AddData  (const QByteArray & moreBytes);

signals:

  void Message (DirectMessage msg);

private:

  void ParseMessage  (QXmlStreamReader & xread, 
                      DirectMessage    & msg,
                      qint64           & offset,
                      bool             & complete,
                      bool             & good);
  void ParseOld      (QXmlStreamReader & xread, 
                      DirectMessage    & msg,
                      qint64           & offset,
                      bool             & complete,
                      bool             & good);
  void ParseXmpp     (QXmlStreamReader & xread, 
                      DirectMessage    & msg,
                      qint64           & offset,
                      bool             & complete,
                      bool             & good,
                      QXmlStreamAttributes & atts);
  void ParseCtl      (QXmlStreamReader & xread, 
                      DirectMessage    & msg,
                      qint64           & offset,
                      bool             & complete,
                      bool             & good,
                      QXmlStreamAttributes & atts);
  void ParseSendfile (QXmlStreamReader & xread, 
                      DirectMessage    & msg,
                      qint64           & offset,
                      bool             & complete,
                      bool             & good,
                      QXmlStreamAttributes & atts);

  QBuffer     inbuf;
  QMutex      bufLock;
  int         lastErr;
  QString     topTag;
  QString     oldTopTag;

};

} // namespace

#endif
