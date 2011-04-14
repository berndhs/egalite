

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
  id: activeServerList

  property real nameWidth: 200
  property real addressWidth: 150
  property real portWidth:  72
  property real buttonWidth: 100
  property real rowWidth: portWidth + 2.0*nameWidth + addressWidth + buttonWidth
  property real rowHeight: 32

  signal disconnectServer (int index)
  signal selectedServer (int index)

  Component {
    id: landscapeDelegate 
    Row {
      width: activeServerList.rowWidth; height: activeServerList.rowHeight
      Rectangle {
        id: connectButtonCol
        width: activeServerList.buttonWidth
        color: "transparent"
        ChoiceButton {
          id: connectButton
          height: activeServerList.rowHeight
          width: parent.width * 0.6666
          anchors { left: parent.left; horizontalCenter: parent.horizontalCenter }
          radius: 0.5*height
          labelText: qsTr ("Leave")
          onClicked: {
            activeServerList.disconnectServer (index)
          }
        }
       }
      
      Rectangle {
        id: baseNameCol
        anchors  {left: connectButtonCol.right }
        width: activeServerList.nameWidth
        height: activeServerList.rowHeight
        color: "transparent"
        MouseArea {
          anchors.fill: parent
          onPressed: {
            activeServerList.currentIndex = index;
            activeServerList.selectedServer (index);
          }
        }
        Text {
          anchors { 
            left: parent.left; leftMargin: 3 ; 
            verticalCenter: parent.verticalCenter 
          }
          text: basename
        }
      }      
      Rectangle {
        id: realNameCol
        anchors  {left: baseNameCol.right }
        width: activeServerList.nameWidth
        height: activeServerList.rowHeight
        color: "transparent"
        MouseArea {
          anchors.fill: parent
          onPressed: {
            activeServerList.currentIndex = index;
            activeServerList.selectedServer (index);
          }
        }
        Text {
          anchors { 
            left: parent.left; leftMargin: 3 ; 
            verticalCenter: parent.verticalCenter 
          }
          text: realname
        }
      }
      Rectangle {
        id: addressCol
        anchors  {left: realNameCol.right }
        width: activeServerList.addressWidth
        height: activeServerList.rowHeight
        color: "transparent"
        MouseArea {
          anchors.fill: parent
          onPressed: {
            activeServerList.currentIndex = index;
            activeServerList.selectedServer (index);
          }
        }
        Text {
          anchors { 
            left: parent.left; leftMargin: 3 ; 
            verticalCenter: parent.verticalCenter 
          }
          text: address
        }
      }
      Rectangle {
        id: portCol
        anchors { 
          left : addressCol.right
        }
        width: activeServerList.portWidth
        height: activeServerList.rowHeight
        color: "transparent"
        MouseArea {
          anchors.fill: parent
          onPressed: {
            activeServerList.currentIndex = index;
            activeServerList.selectedServer (index);
          }
        }
        Text {
          id: thePortText
          anchors { 
            left: parent.left
            verticalCenter: parent.verticalCenter  
          }
          horizontalAlignment: TextInput.AlignRight
          text: port
        }
      }
    }
    
  }

 
  delegate: landscapeDelegate
  highlight: Rectangle { color: "#77ddff" } 
  Component.onCompleted: console.log ("Done loading ActiveServerList")

}
