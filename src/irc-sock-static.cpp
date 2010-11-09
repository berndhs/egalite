
#include "irc-sock.h"

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

#include <QRegExp>
#include <QString>

namespace egalite
{

void
IrcSock::TransformDefault (IrcSock * context,
                           QString & result, 
                           QString & first, 
                           QString & rest)
{
  Q_UNUSED (context)
  result = first + rest;
}

void
IrcSock::TransformPRIVMSG (IrcSock * context,
                           QString & result, 
                           QString & first, 
                           QString & rest)
{
  Q_UNUSED (context)
  first = "PRIVMSG";
  QRegExp wordRx ("(\\S+)");
  int pos = wordRx.indexIn (rest, 0);
  if (pos >= 0) {
    int len = wordRx.matchedLength ();
    rest.insert (pos + len, " :");
  }
  result = first + " " + rest;
}

void 
IrcSock::TransformJOIN (IrcSock * context,
                        QString & result,
                        QString & first,
                        QString & rest)
{
  result = first + rest;
}

void
IrcSock::ReceivePING (IrcSock * context,
                      const QString & first,
                      const QString & cmd,
                      const QString & rest)
{
  Q_UNUSED (cmd)
  Q_UNUSED (rest)
  QString answer = QString ("PONG %1 %2").arg(first);
  context->SendData (answer);
}

void
IrcSock::ReceiveJOIN (IrcSock * context,
                      const QString & first,
                      const QString & cmd,
                      const QString & rest)
{
  Q_UNUSED (cmd)
  QRegExp wordRx ("(\\S+)");
  int pos, len;
  QString user, chan;
  pos = wordRx.indexIn (first, 0);
  if (pos >= 0) {
    len = wordRx.matchedLength ();
    user = first.mid (pos,len);
    QRegExp leadRx ("([^!]+)");
    pos = leadRx.indexIn (user,0);
    if (pos >= 0) {
      user = user.mid (pos, leadRx.matchedLength());
    }
    if (user.startsWith (QChar (':'))) {
      user.remove (0,1);
    }
  }
  pos = wordRx.indexIn (rest,0);
  if (pos >= 0) {
    len = wordRx.matchedLength ();
    chan = rest.mid (pos,len);
qDebug () << " JOIN received,  " << first << cmd << rest;
qDebug () << "user " << user << " currentUser " 
          << context->currentUser << " chan " << chan;
    if (chan.startsWith (QChar(':'))) {
      chan.remove (0,1);
    }
    if (user == context->currentUser) {
      if (!context->channels.contains (chan)) {
        context->AddChannel (chan);
      }
    } else {
      context->AddName (chan, user);
      context->mainUi.logDisplay->append (QString ("user %1 JOINs %2")
                                    .arg (user) . arg (chan));
    }
  }
}

void
IrcSock::ReceivePART (IrcSock * context,
                     const QString & first,
                     const QString & cmd,
                     const QString & rest)
{
  qDebug () << " PART received " << first << cmd << rest;
  QRegExp wordRx ("(\\S+)");
  int pos, len;
  QString user, chan;
  pos = wordRx.indexIn (first, 0);
  if (pos >= 0) {
    len = wordRx.matchedLength ();
    user = first.mid (pos,len);
    QRegExp leadRx ("([^!]+)");
    pos = leadRx.indexIn (user,0);
    if (pos >= 0) {
      user = user.mid (pos, leadRx.matchedLength());
    }
    if (user.startsWith (QChar (':'))) {
      user.remove (0,1);
    }
  }
  pos = wordRx.indexIn (rest,0);
  if (pos >= 0) {
    len = wordRx.matchedLength ();
    chan = rest.mid (pos,len);
    if (chan.startsWith (QChar (':'))) {
      chan.remove (0,1);
    }
qDebug () << " PART received for channel " << chan;
qDebug () << "user " << user << " currentUser " 
          << context->currentUser << " chan " << chan;
    if (user == context->currentUser) {
      context->DropChannel (chan);
    } else {
      context->DropName (chan, user);
      context->mainUi.logDisplay->append (QString ("user %1 PARTs %2")
                                    .arg (user) . arg (chan));
    }
  }
}

void
IrcSock::ReceivePRIVMSG (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest)
{
  Q_UNUSED (cmd)
  int pos, len;
  QRegExp sourceRx ("(\\S+)!");
  QString source (first);
  QString dest;
  QString msg;
  QRegExp wordRx ("(\\S+)");
  pos = sourceRx.indexIn (first,0);
  if (pos >= 0) {
    len = sourceRx.matchedLength ();
    source = first.mid (pos,len);
    source.chop (1);
    if (source.startsWith (QChar (':'))) {
      source.remove (0,1);
    }
  }
  pos = wordRx.indexIn (rest,0);
  if (pos >= 0) {
    len = wordRx.matchedLength ();
    dest = rest.mid (pos,len);
    msg = rest.mid (pos+len,-1);
    if (context->channels.contains (dest)) {
      context->InChanMsg (dest, source, msg);
    } else {
      context->InUserMsg (source, dest, msg);
    }
  }
}

void
IrcSock::ReceiveNumeric (IrcSock * context,
                        const QString & first,
                        const QString & num,
                        const QString & rest)
{
  context->LogRaw (QString ("numeric %1  %2 %3").arg(first).arg(num).arg(rest));
}

void
IrcSock::ReceiveDefault (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest)
{
  qDebug () << "Default Receiver " << context << first << cmd << rest;
}

void
IrcSock::ReceiveIgnore (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest)
{
  qDebug () << " Ignoring command " << cmd;
}

void
IrcSock::Receive332 (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest)
{
  qDebug () << " Received 332 " << first << cmd << rest;
  QRegExp wordRx ("(\\S+)");
  int pos, len;
  QString user, chan, topic;
  pos = wordRx.indexIn (rest, 0);
  if (pos >= 0) {
    len = wordRx.matchedLength();
    user = rest.mid (pos, len);
    pos = wordRx.indexIn (rest, pos+len);
    if (pos >= 0) {
      len = wordRx.matchedLength ();
      chan = rest.mid (pos, len);
      topic = rest.mid (pos+len, -1).trimmed ();
      if (topic.startsWith (QChar (':'))) {
        topic.remove (0,1);
      }
      context->SetTopic (chan, topic);
    }
  }
}

void
IrcSock::Receive353 (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest)
{
  qDebug () << " Received 353 " << first << cmd << rest;
  QRegExp wordRx ("(\\S+)");
  int pos, len;
  QString user, marker, chan;
  pos = wordRx.indexIn (rest, 0);
  if (pos >= 0) {
    len = wordRx.matchedLength();
    user = rest.mid (pos, len);
    pos = wordRx.indexIn (rest, pos+len);
    if (pos >= 0) {
      len = wordRx.matchedLength ();
      marker = rest.mid (pos, len);
      pos = wordRx.indexIn (rest, pos+len);
      if (pos >= 0) {
        len = wordRx.matchedLength ();
        chan = rest.mid (pos,len);
        QString nameData = rest.mid (pos+len,-1).trimmed ();
        if (nameData.startsWith (QChar(':'))) {
          nameData.remove (0,1);
        }
        context->AddNames (chan, nameData);
      }
    }
  }
}


void
IrcSock::Receive366 (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest)
{
  qDebug () << " Received 366 " << first << cmd << rest;
}

} // namespace
