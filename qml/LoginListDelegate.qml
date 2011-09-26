import QtQuick 1.0



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
  property real mainWidth: 512
  property real rowHeight: 48
  width: 800
  height: 400

  Row {
    id: contactNameRow
    Rectangle {
      id: showHideMarker
      width: mainBox.rowHeight
      height: mainBox.rowHeight
      color: "red"
      radius: height * 0.25
      property bool showDetails: true
      Text {
        id: showHideMarkerText
        anchors.centerIn: parent
        font.bold: true
        font.pointSize: 30
        text: showHideMarker.showDetails ? "-" : "+"
      }
      MouseArea {
        anchors.fill: parent
        onPressed: {
          showHideMarker.showDetails = ! showHideMarker.showDetails
        }
      }
    }
    Rectangle {
      id: jidBox
      width: mainBox.mainWidth * 0.5
      height: mainBox.rowHeight
      color: "#ffff20"
      Text {
        id: jidBoxText
        text: loginJid + " excount " + nameBox.explicitCount
      }
    }
    Rectangle {
      id: nameBox
      width: mainBox.mainWidth * 0.5
      height: mainBox.rowHeight
      color: "#ffff40"
      property real explicitCount: -1
      Text {
        id: nameBoxText
        text: "name [" + loginName + "] #contacts " + contactRepeater.count + "/"+nameBox.explicitCount
      }
    }
  }
  
  Connections {
    target: cppLoginModel
    onCountChanged: {
      nameBox.explicitCount = rows
      console.log (" qml model-update explicit " + rows + " model-count " + contactRepeater.count)
      console.log (" qml model-update sizes: contactLoginBox " + contactLoginBox.width + "/"
                   + contactLoginBox.width 
                   + "  contactRepeater   " + contactRepeater.width + "/" + contactRepeater.height
                   + "\n ")
    }
  }

  Rectangle {
    id: contactLoginBox
    property real indent: 24
    width: mainBox.mainWidth - indent
    height: showHideMarker.showDetails ? detailColumn.height : 20
    visible: showHideMarker.showDetails
    color: "salmon"
    border.color:"red"; border.width: 1
    anchors {
      top: contactNameRow.bottom
      left: mainBox.horizontalCenter
      leftMargin: indent - 0.5 * mainBox.mainWidth
    }

    Rectangle {
      id: detailColumn
      width: contactLoginBox.width
      height: 300
      color: "#ddffee"
      anchors { 
        top: parent.top
        horizontalCenter: parent.horizontalCenter
      }
      Rectangle {
        id: detailTopSpacer
        width: parent.width * 0.75
        height: 3
        color: "red"
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
      }
      Rectangle {
        height:contactRepeater.height; width:contactLoginBox.width
        color:"red"
        anchors { top: detailTopSpacer.bottom; horizontalCenter: parent.horizontalCenter }
        GridView {
          id: contactRepeater
          model: cppLoginModel.contacts (loginJid)
          width: contactLoginBox.width
          height: 200
          //contentWidth:contactLoginBox.width
          //contentHeight:1000
          anchors { top: detailTopSpacer.bottom; horizontalCenter: parent.horizontalCenter }
          clip: true

          delegate: Rectangle {
            id: contactDelegateBox
            width: contactLoginBox.width
            height: mainBox.rowHeight * 2
            color: "yellow"
            border.color:"#00007f"; border.width:8
            property real spacing: 4
            Column {
              spacing: 2
              width: contactLoginBox.width
              Rectangle {
                id: topMarker
                width: parent.width * 0.75
                height: 6
                color: "black"
              }
              Text { text: "item " + index + " of " + contactRepeater.count}
              Row {
                id: contactHeadRow
                height:mainBox.rowHeight
                spacing: 4
                anchors.fill:parent
                
                Rectangle {
                  id: contactJidBox
                  width: contactJidText.width + contactDelegateBox.spacing
                  height: mainBox.rowHeight
                  color:"blue"
                  Text {
                    id:contactJidText
                    anchors { 
                      left: parent.left; 
                      verticalCenter: parent.verticalCenter 
                    }
                    text: "contactJid: ["+ index + "]" //+ contactJid
                  }
                  
                }
                Rectangle {
                  id: contactNameBox
                  width: contactNameText.width + contactDelegateBox.spacing
                  height: mainBox.rowHeight
                  color:"red"
                  Text {
                    id: contactNameText
                    anchors { 
                      left: parent.left; 
                      verticalCenter: parent.verticalCenter 
                    }
                    text: "contactName " //+ contactName
                  }                
                } 
              }
              
              Repeater {
                model: cppLoginModel.logins (loginJid,contactJid)
                width: contactDelegateBox.width
                height: mainBox.rowHeight * loginCount
                Text { text: "repeat " + index }
               // contentWidth: width
               // contentHeight: 1024
                /*
                delegate: ContactLoginDelegate {
                  width: contactDelegateBox.width 
                  height: mainBox.rowHeight
                }
                */
                
              }
              
            }
          }
        }
      }

    }
  }

}
