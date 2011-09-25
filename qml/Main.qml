import QtQuick 1.0
//import moui.geuzen.utils.static 1.0


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


Rectangle {
  id: mainBox

  width: 1024; height: 1024
  color: "yellow"
  visible: true

  property bool isPortrait: false
  property real mainWidth: isPortrait ? height : width
  property real mainHeight: isPortrait ? width: height
  property real rowHeight: 48
  
  signal doQuit ()
  
  Rectangle {
    id: brandingBox
    width: mainBox.mainWidth
    color: "blue"
    height: mainBox.rowHeight
    anchors {
      top: mainBox.top
      horizontalCenter: mainBox.horizontalCenter
    }
    Row {
      spacing: 16
      anchors {
        top: brandingBox.top
        horizontalCenter: brandingBox.horizontalCenter
      }
    
      Rectangle {
        id: programNameBox
        width:programNameText.width * 2
        height: brandingBox.height
        color:Qt.lighter (brandingBox.color)
        radius:0.5*height
        Text {
          id: programNameText
          anchors.centerIn: parent
          font.bold: true
          text:"Egalitè"
        }
      }
      
      ChoiceButton {
        id: quitButton
        height: brandingBox.height
        topColor:"#ffaadd"
        radius: height * 0.5
        labelText: qsTr("Quit")
        
        onClicked: {
          mainBox.doQuit()
        }
      }
    }
  }

  ListView {
    id: loginList
    model: cppLoginModel
    anchors {
      top:brandingBox.bottom
      horizontalCenter: mainBox.horizontalCenter
    }
    width: mainBox.mainWidth
    height: mainBox.mainHeight - brandingBox.height

    delegate: LoginListDelegate {
      mainWidth: mainBox.mainWidth
    }
  }
}
