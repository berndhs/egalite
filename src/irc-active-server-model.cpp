#include "irc-active-server-model.h"


/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2011, Bernd Stramm
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

#include <QHash>
#include <QByteArray>
#include <QDebug>

namespace egalite
{

ActiveServerModel::ActiveServerModel (QObject *parent)
  :QAbstractListModel (parent)
{
  QHash<int, QByteArray>  roles;
  roles[Role_BaseName] = "basename";
  roles[Role_RealName] = "realname";
  roles[Role_Address] = "address";
  roles[Role_Port] = "port";
  roles[Role_State] = "connectstate";
  setRoleNames (roles);
}

void
ActiveServerModel::clear ()
{
  beginResetModel ();
  servers.clear ();
  endResetModel ();
}

int
ActiveServerModel::rowCount (const QModelIndex & parent) const
{
  Q_UNUSED (parent)
  return servers.count ();
}

void
ActiveServerModel::disconnectServer (int row)
{
  int ns = servers.count();
  if (row < 0 || row >= ns) {
    return;
  }
  emit wantDisconnect (servers.at(row).socket);
}

void
ActiveServerModel::removeServer (IrcSocket * sock)
{
  int row = findServer (sock);
  if (row < 0) {
    return;
  }
  beginRemoveRows (QModelIndex (), row, row);
  servers.removeAt (row);
  endRemoveRows ();
}

QVariant
ActiveServerModel::data (const QModelIndex & index, int role) const
{
qDebug () << " ActiveServerModel data " << index << " role " << role;
  if (!index.isValid()) {
    return QVariant ();
  }
  int row = index.row();
  int nr = servers.count ();
  if (row < 0 || row >= nr) {
    return QVariant ();
  }
  QVariant retVar;
  switch (role) {
  case Qt::DisplayRole:
  case Role_BaseName:
    retVar = servers.at(row).baseName;
    break;
  case Role_RealName:
    retVar = servers.at(row).realName;
    break;
  case Role_Address:
    retVar = servers.at(row).address.toString();
    break;
  case Role_Port:
    retVar = QString::number (servers.at(row).port);
    break;
  case Role_State:
    if (servers.at(row).socket) {
      retVar = QString::number (servers.at(row).socket->state());
    } else {
      retVar = QString ("??");
    }
    break;
  default:
    break;
  }
  qDebug () << "           return " << retVar;
  return retVar;
}

void
ActiveServerModel::addServer (IrcSocket *sock,
                  const QString & baseName, 
                  const QString & realName,
                  const QHostAddress & address,
                    int port)
{
qDebug () << " ActiveServerModel::addServer " << baseName << port;
  int nr = rowCount();
  beginInsertRows (QModelIndex(), nr, nr);
  servers << ServerStruct (sock, baseName, realName, address, port);
  endInsertRows ();
}

int
ActiveServerModel::findServer (const IrcSocket *sock) const
{
  if (sock) {
    int ns = servers.count();
    for (int i=0; i<ns; i++) {
      if (servers.at(i).socket == sock) {
        return i;
      }
    }
  }
  return -1;
}

void
ActiveServerModel::setPort (const IrcSocket *sock, int port)
{
  int row = findServer (sock);
  if (row < 0) {
    return;
  }
  int oldport = servers.at(row).port;
  if (port != oldport) {
    servers[row].port = port;
    QModelIndex changedIndex = createIndex (row, 0);
    emit dataChanged (changedIndex, changedIndex);
  }
}

void
ActiveServerModel::setBaseName (const IrcSocket *sock, const QString & name)
{
  int row = findServer (sock);
  if (row < 0) {
    return;
  }
  servers[row].baseName = name;
  QModelIndex changedIndex = createIndex (row, 0);
  emit dataChanged (changedIndex, changedIndex);
}

void
ActiveServerModel::setRealName (const IrcSocket *sock, const QString & name)
{
  int row = findServer (sock);
  if (row < 0) {
    return;
  }
  servers[row].realName = name;
  QModelIndex changedIndex = createIndex (row, 0);
  emit dataChanged (changedIndex, changedIndex);
}

void
ActiveServerModel::setAddress (const IrcSocket *sock, 
                               const QHostAddress & addr)
{
  int row = findServer (sock);
  if (row < 0) {
    return;
  }
  servers[row].address = addr;
  QModelIndex changedIndex = createIndex (row, 0);
  emit dataChanged (changedIndex, changedIndex);
}

int
ActiveServerModel::port (const IrcSocket *sock) const
{
  int row = findServer (sock);
  if (row < 0) {
    return -1;
  }
  return servers.at(row).port;
}

QString
ActiveServerModel::baseName (const IrcSocket *sock) const
{
  int row = findServer (sock);
  if (row < 0) {
    return QString();
  }
  return servers.at(row).baseName;
}

QString
ActiveServerModel::realName (const IrcSocket *sock) const
{
  int row = findServer (sock);
  if (row < 0) {
    return QString();
  }
  return servers.at(row).realName;
}

QHostAddress
ActiveServerModel::address (const IrcSocket *sock) const
{
  int row = findServer (sock);
  if (row < 0) {
    return QHostAddress();
  }
  return servers.at(row).address;
}

ActiveServerModel::ServerStruct::ServerStruct ()
  :socket(0),
   baseName (""),
   realName (""),
   address (QHostAddress()),
   port (0)
{
}

ActiveServerModel::ServerStruct::ServerStruct (IrcSocket * sck,
                  const QString & bn,
                  const QString & rn,
                  const QHostAddress & ad, 
                  int p)
  :socket (sck),
   baseName(bn),
   realName (rn),
   address (ad),
   port (p)
{
}

ActiveServerModel::ServerStruct::ServerStruct (
         const ActiveServerModel::ServerStruct & other)
  :socket (other.socket),
   baseName (other.baseName),
   realName (other.realName),
   port (other.port)
{
}


} // namespace
