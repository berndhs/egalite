
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

#include "xegal-client.h"
#include <QXmppPresence.h>
#include <QXmppRoster.h>
#include <QXmppRosterIq.h>

namespace egalite
{

XEgalClient::XEgalClient (QObject *parent, QString user)
  :QXmppClient (parent),
   thisUser (user)
{
  connect (this, SIGNAL (presenceReceived (const QXmppPresence &)),
           this, SLOT (PresenceChange (const QXmppPresence &)));
}

XEgalClient::~XEgalClient ()
{
  Disconnect ();
}

void
XEgalClient::Disconnect ()
{
  if (isConnected()) {
    Announce (QXmppPresence::Unavailable, 
              QXmppPresence::Status::Offline,
              tr("Logged Off"));
    disconnect ();
  }
}

void
XEgalClient::Announce (QXmppPresence::Type  newType,
                       QXmppPresence::Status::Type  subStatus,
                       QString  message)
{
  QXmppPresence::Status status (subStatus, message);
  QXmppPresence   pres (newType, status);
  setClientPresence (pres);
}

void
XEgalClient::PresenceChange (const QXmppPresence & presence)
{
  QXmppPresence::Type   presType = presence.type ();
  QXmppPresence::Status status = presence.status();
  QStringList parts = presence.from().split('/');
  QString from = parts.at(0);
  if (presType == QXmppPresence::Available 
      || presType == QXmppPresence::Unavailable
      || presType == QXmppPresence::Error) {
    QString fromName = getRoster().getRosterEntry(from).name();
    emit UpdateState (fromName, presence.to(), presence.from(), status);
  } else {
    emit ChangeRequest (thisUser, presence);
  }
}

} // namespace
