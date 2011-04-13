

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

ListView {
  id: knownServerList

  property real nameWidth: 250
  property real portWidth:  100
  property real rowWidth: 350
  property real rowHeight: 32

  property string nameColor: "#ff0000"
  property string portColor: "#ff7700"

  signal selectServer (string name, int port)

  Component {
    id: landscapeDelegate 
    Rectangle {
      width: knownServerList.rowWidth; height: knownServerList.rowHeight
      color: "yellow"
      border.color: "black"
      Column {
        id: serverNameCol
        Rectangle {
          id: nameBox
          width: knownServerList.nameWidth
          height: knownServerList.rowHeight
          color: knownServerList.nameColor
          MouseArea {
            anchors.fill: parent
            onClicked: knownServerList.selectServer (sname, sport)
          }
          Text {
            anchors.fill: parent
            text: sname
          }
        }
      }
      Column {
        id: serverPortCol
        anchors.left : serverNameCol.right
        Rectangle {
          id: portBoknownServerListx
          width: knownServerList.portWidth
          height: knownServerList.rowHeight
          color: knownServerList.portColor
          MouseArea {
            anchors.fill: parent
            onClicked: knownServerList.selectServer (sname, sport)
          }
          TextInput {
            anchors.fill: parent
            text: sport
          }
        }
      }
    }
    
  }

  Component {
    id: landscapeHighlight
    Rectangle {
      width: knownServerList.rowWidth
      height: knownServerList.rowHeight 
      radius: 4
      color: Qt.lighter (knownServerList.nameColor)
    }
  }


  delegate: landscapeDelegate
  highlight: landscapeHighlight

  Component.onCompleted: console.log ("Done loading KnownServerList")

}
