#ifndef XEGAL_CLIENT_H
#define XEGAL_CLIENT_H
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

#include <QXmppClient.h>

namespace egalite
{

class XEgalClient : public QXmppClient
{
Q_OBJECT

public:

  XEgalClient (QObject *parent =0,
               QString user = QString());

  ~XEgalClient ();

  void Disconnect ();
  void Announce (QXmppPresence::Type  newState,
                 QXmppPresence::Status::Type subStatus,
                 QString  message);

public slots:

  void PresenceChange (const QXmppPresence & presence);

signals:

  void UpdateState (QString remoteName, 
               QString ownId,
               QString remoteId,
               QXmppPresence::Status status);
  void ChangeRequest (QString ownId,
                 QXmppPresence presence);

private:

  QString  thisUser;

};

} // namespace

#endif
