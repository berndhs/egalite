
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
  property string baseColor: "yellow"
  property real rollDelay: 175
  color: baseColor

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
  Rectangle {
    width: childrenRect.width; height: childrenRect.height
    color: knownButton.visible ? "transparent " : "green"
    ChoiceButton {
      id: knownButton
      height: 32
      width: knownServerList.nameWidth
      radius: 0.5 * height
      color: Qt.darker (baseColor)
      labelText: qsTr (" - Show Known Servers - ")
      visible: true
      onClicked: {
        visible = false
        knownServerList.show ()
      }
    }
    KnownServerList {
      id: knownServerList
      visible: !knownButton.visible
      model: cppKnownServerModel
      height: (visible ? 3*rowHeight : 0)
      width: (visible ? rowWidth : 0)
      nameWidth: 300
      portWidth: 90
      clip: true
      Behavior on height { PropertyAnimation { duration: rollDelay  } }
      Behavior on width { PropertyAnimation { duration: rollDelay  } }
      onSelectServer: console.log ("picked server " + name + " port " + port)
      onConnectServer: {
        console.log ("connect server " + name + " port " + port )
        knownButton.visible = true
      }
    }
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
