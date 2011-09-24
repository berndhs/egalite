#ifndef SUBSCRIPTION_CHANGE_H
#define SUBSCRIPTION_CHANGE_H

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
#include "ui_subscription.h"
#include <QDialog>
#include <QXmppPresence.h>
#include <QXmppClient.h>

namespace egalite
{

class SubscriptionChange : public QDialog
{
Q_OBJECT

public:

  SubscriptionChange (QWidget *parent = 0);

  void RemoteAskChange (QXmppClient * client, const QXmppPresence & presence);

private slots:

  void Allow ();
  void Deny ();
  void Ignore ();

private:

  QString PresenceTypeMessage (QXmppPresence::Type t);
  void    Reply (QXmppPresence::Type t);
  void    SetButtonEnable (bool enable);

  QXmppClient    * xclient;
  QXmppPresence    request;

  Ui_SubscriptionChange    ui;
};

} // namespace

#endif
