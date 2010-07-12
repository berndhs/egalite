
#ifndef CHAT_CONTENT_H
#define CHAT_CONTENT_H

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

#include "ui_chat-content.h"
#include <QDialog>
#include <QByteArray>

class QXmppMessage;

namespace egalite
{

class ChatContent : public QDialog 
{
Q_OBJECT

public:

  ChatContent (QWidget * parent=0);

  void SetRemoteName (const QString & name);
  void SetLocalName  (const QString & name);

  QString RemoteName () { return remoteName; }
  QString LocalName () { return localName; }

public slots:

  void Incoming (const QByteArray &data);

private slots:

  void Send ();
  void EndChat ();

signals:

  void Outgoing (const QByteArray &data);
  void Disconnect ();

private:

  void GetMessage (const QXmppMessage & msg);

  Ui_ChatContent   ui;

  QString          remoteName;
  QString          localName;
};

} // namespace

#endif
