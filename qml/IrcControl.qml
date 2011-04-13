
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

import QtQuick 1.0

Rectangle {
  id: ircControlBox
  objectName: "IrcControlBox"

  property alias activeServerModel: activeServerList.model

  color: "yellow"

  signal hideMe ()

  ChoiceButton {
    anchors {top: parent.top; right: parent.right }
    labelText: qsTr ("Hide")
    height: 32
    width: 80
    onClicked: {
      console.log ("Hide Button clicked")
      ircControlBox.hideMe ();300
    }
  }
  KnownServerList {
    id: knownServerList
    model: cppKnownServerModel
    height: 3*rowHeight; 
    rowWidth: 500
    width: rowWidth
    clip: true
    onSelectServer: console.log ("picked server " + name + " port " + port)
  }

  ListView {
    id: activeServerList
  }
    

  Text {
    id: placeHolder
    anchors.centerIn: parent
    text: "IrcControlBox"
  }
  
  Component.onCompleted: {
    console.log ("loaded IrcControlBox")
  }
}
