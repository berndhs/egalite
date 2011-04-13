

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
  property real buttonWidth: 100
  property real rowWidth: portWidth + nameWidth + buttonWidth
  property real rowHeight: 32

  signal selectServer (string name, int port)
  signal connectServer (string name, int port)

  Component {
    id: landscapeDelegate 
    Row {
      width: knownServerList.rowWidth; height: knownServerList.rowHeight
      Rectangle {
        id: connectButtonCol
        width: knownServerList.buttonWidth
        color: "transparent"
        ChoiceButton {
          id: connectButton
          height: knownServerList.rowHeight
          width: parent.width * 0.6666
          anchors { left: parent.left; horizontalCenter: parent.horizontalCenter }
          radius: 0.5*height
          labelText: qsTr ("Connect")
          onClicked: {
            knownServerList.connectServer (sname, sport)
          }
        }
       }
      
      Rectangle {
        id: serverNameCol
        anchors  {left: connectButtonCol.right }
        width: knownServerList.nameWidth
        height: knownServerList.rowHeight
        color: "transparent"
        MouseArea {
          anchors.fill: parent
          onClicked: {
            knownServerList.currentIndex = index
            console.log ("setting currentindex to " + index)
            knownServerList.selectServer (sname, sport)
          }
        }
        Text {
          anchors { 
            left: parent.left; leftMargin: 3 ; 
            verticalCenter: parent.verticalCenter 
          }
          text: sname
        }
      }
      Rectangle {
        id: serverPortCol
        anchors { 
          left : serverNameCol.right
        }
        width: knownServerList.portWidth
        height: knownServerList.rowHeight
        color: "transparent"
        MouseArea {
          anchors.fill: parent
          onClicked: {
            knownServerList.currentIndex = index
            knownServerList.selectServer (sname, sport)
          }
        }
        TextInput {
          anchors { 
            left: parent.left
            verticalCenter: parent.verticalCenter  
          }
          horizontalAlignment: TextInput.AlignRight
          text: sport
        }
      }
    }
    
  }

 
  delegate: landscapeDelegate
  highlight: Rectangle { color: "#7777ff" } 
  Component.onCompleted: console.log ("Done loading KnownServerList")

}
