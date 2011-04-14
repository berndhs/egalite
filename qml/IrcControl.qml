
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
  signal tryConnect (string host, int port)

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
    id: knownListRect
    width: childrenRect.width; height: childrenRect.height
    color: knownButton.visible ? "transparent " : "green"
    border.color: "#c0c0c0"
    border.width: knownServerList.visible ? 1 : 0
    property string showString: qsTr (" --- Show Known Servers --- ")
    property string noShowString: qsTr (" Hide List ")
    ChoiceButton {
      id: knownButton
      height: 32
      width: knownServerList.nameWidth
      radius: 0.5 * height
      property string lightColor: "#aaffaa"
      property string darkColor: "#ffaaff"
      property bool seeList: knownServerList.visible
      color: seeList ? lightColor : darkColor
      labelText: knownServerList.visible 
                      ? knownListRect.noShowString 
                      : knownListRect.showString
      visible: true
      Behavior on color { PropertyAnimation { duration: rollDelay } }
      onClicked: {
        knownServerList.visible = !knownServerList.visible
      }
    }
    KnownServerList {
      id: knownServerList
      visible: !knownButton.visible
      model: cppKnownServerModel
      height: (visible ? 3*rowHeight : 0)
      width: (visible ? rowWidth : 0)
      anchors {top : knownButton.bottom; left: knownButton.left }
      nameWidth: 300
      portWidth: 90
      clip: true
      Behavior on height { PropertyAnimation { duration: rollDelay  } }
      Behavior on width { PropertyAnimation { duration: rollDelay  } }
      onSelectServer: console.log ("picked server " + name + " port " + port)
      onConnectServer: {
        console.log ("connect server " + name + " port " + port )
        visible = false
        ircControlBox.tryConnect (name, port)
      }
    }
  }

  Rectangle {
    id: activeListBox
    anchors { top: knownListRect.bottom; left: parent.left; leftMargin: 2 }
    width: ircControlBox.width - 4
    height: 200
    color: "blue"
    border.color: "black"
    ActiveServerList {
      id: activeServerList
      anchors { left: parent.left; top: parent.top }
      nameWidth: 200
      addressWidth: 172
      height: 200
      model: cppActiveServerModel
      onDisconnectServer: {
        console.log ("disconnect from " + index)
        model.disconnectServer (index)
      }
    }
  }
    
  Component.onCompleted: {
    console.log ("loaded IrcControlBox")
  }
}
