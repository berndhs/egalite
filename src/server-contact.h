
#ifndef SERVER_CONTACT_H
#define SERVER_CONTACT_H

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

#include <map>

namespace egalite
{

class ServerContact
{
  public:

    ServerContact ()
                 :name(""),
                  state(""),
                  resource(""),
                  modelRow(0),
                  recentlySeen(true)  
                 {}
    
    QString    name;
    QString    state;
    QString    resource;
    int        modelRow;
    bool       recentlySeen;
};

typedef  std::map <QString, ServerContact*>   ContactMap;

} // namespace

#endif

