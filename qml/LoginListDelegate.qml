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
  id: loginListDelegate
  property real mainWidth: 512
  property real rowHeight: 48

  Row {
    id: contactNameRow
    Rectangle {
      id: showHideMarker
      width: loginListDelegate.rowHeight
      height: loginListDelegate.rowHeight
      property bool showDetails: false
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
      width: loginListDelegate.mainWidth * 0.5
      height: loginListDelegate.rowHeight
      Text {
        id: jidBoxText
        text: loginJid
      }
    }
    Rectangle {
      id: nameBox
      width: loginListDelegate.mainWidth * 0.5
      height: loginListDelegate.rowHeight
      Text {
        id: nameBoxText
        text: loginName
      }
    }
  }

  Rectangle {
    id: contactLoginBox
    property real indent: 24
    width: loginListDelegate.mainWidth - indent
    height: showHideMarker.showDetails ? detailColumn.height : 0
    anchors {
      top: contactNameRow.bottom
      left: loginListDelegate.horizontalCenter
      leftMargin: indent - 0.5 * loginListDelegate.mainWidth
    }
    Column {
      id: detailColumn
      Repeater {
        model: cppLoginModel.contacts (loginJid)
        delegate: Rectangle {
          width: contactLoginBox.width
          height: loginListDelegate.rowHeight
          Flow {
            Text {
              text: contactLoginStatus
            }
            Text {
              text: contactLoginResource
            }
          }
        }
      }
    }
  }

}
