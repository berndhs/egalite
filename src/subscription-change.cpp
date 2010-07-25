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

#include "subscription-change.h"
#include <QTimer>

namespace egalite
{

SubscriptionChange::SubscriptionChange (QWidget *parent)
  :QDialog (parent)
{
  ui.setupUi (this);
  connect (ui.allowButton, SIGNAL (clicked ()), this, SLOT (Allow ()));
  connect (ui.denyButton, SIGNAL (clicked ()), this, SLOT (Deny ()));
  connect (ui.ignoreButton, SIGNAL (clicked ()), this, SLOT (Ignore ()));
}

void
SubscriptionChange::SetButtonEnable (bool enable)
{
  ui.allowButton->setEnabled (enable);
  ui.denyButton->setEnabled (enable);
  ui.ignoreButton->setEnabled (true);   // can always ignore
}

void 
SubscriptionChange::RemoteAskChange  (QXmppClient * client, 
                                const QXmppPresence & presence)
{
  xclient = client;
  request = presence;
  SetButtonEnable (request.type() == QXmppPresence::Subscribe);
  QStringList  parts = presence.to().split ('/',QString::SkipEmptyParts);
  ui.ownId->setPlainText (parts.at(0));
  parts = presence.from ().split ('/',QString::SkipEmptyParts);
  ui.remoteId->setPlainText (parts.at(0));
  ui.changeMessage->setPlainText (PresenceTypeMessage (presence.type()));
  show ();
  QTimer::singleShot (30*1000, this, SLOT (Ignore()));
}

QString
SubscriptionChange::PresenceTypeMessage (QXmppPresence::Type t)
{
  switch (t) {
    case QXmppPresence::Error: 
      return tr ("pres-error");
    case QXmppPresence::Available:
      return tr ("Say they are Available");
    case QXmppPresence::Unavailable:
      return tr ("Say they are Not Available");
    case QXmppPresence::Subscribe:
      return tr ("Want to Subscribe to be notified about your presence status");
    case QXmppPresence::Subscribed:
      return tr ("Have allowed subscription about their presence status");
    case QXmppPresence::Unsubscribe:
      return tr ("Want to Un-Subscribe from notifications "
                  "about your presence status");
    case QXmppPresence::Unsubscribed:
      return tr ("Have Un-Subscribed from notifications " 
                  "about your presence status");
    case QXmppPresence::Probe:
      return tr ("Probe");
    default:
      return QString ("invalid type");
  }
}

void
SubscriptionChange::Reply (QXmppPresence::Type t)
{
  QXmppPresence  answer;
  answer.setTo (request.from());
  answer.setType (t);
  if (xclient) {
    xclient->sendPacket (answer);
  }
}

void
SubscriptionChange::Allow ()
{
  if (request.type() == QXmppPresence::Subscribe) {
    Reply (QXmppPresence::Subscribed);
  }
  xclient = 0;
  accept ();
}

void
SubscriptionChange::Deny ()
{
  if (request.type() == QXmppPresence::Subscribe) {
    Reply (QXmppPresence::Unsubscribed);
  }
  xclient = 0;
  reject ();
}

void
SubscriptionChange::Ignore ()
{
  xclient = 0;
  reject ();
}

} // namespace

