

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


import QtQuick 1.0

Rectangle {
  id: channelBox
  property real labelHeight:32
  property real userNameHeight: 20
  property real countWidth: 0.3 * width
  property real rollDelay: 250
  property alias userListModel: userList.model
  property alias boxLabel: channelBoxLabel.text
  property alias userListCounter: userListCount.text

  function selectUser (user) {
    console.log ("selected user " + user)
  }

  function setModel (theModel) { userList.model = theModel }

  height: parent.height
  width: parent.width
  color: "red"
  Text {
    id: channelBoxLabel
    height: channelBox.labelHeight
    width: channelBox.width
    anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
    text: "Channel Group Box"
  }
  TextEdit {
    id: historyBox
    anchors { top: channelBoxLabel.bottom; left: parent.left }
    text : "Chat Hstory"
  }
  TextEdit {
    id: rawLogBox
    anchors { top: historyBox.bottom; left: parent.left }
    text : "Chat Raw Log"
  }
  TextEdit {
    id: userInfoBox
    anchors { top: rawLogBox.bottom; left: parent.left }
    text : "User Query Info"
  }
  TextInput {
    id: textEnter
    anchors { top: userInfoBox.bottom; left: parent.left }
    text: "User Input to Send"
  }
  Rectangle {
    id: userListBox
    color: "transparent"
    anchors { top: channelBox.top; right: channelBox.right }
    height: channelBox.height
    width: countWidth
    property bool shortView: false
    Rectangle {
      id: userListCountRect
      height: userListCount.height
      width: parent.width
      color: "#0099ee"
      Text {
        id: userListCount
        horizontalAlignment: Text.AlignHCenter
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
        //height: labelHeight
        width: parent.width
        text: "User Count"
      }
      MouseArea {
        anchors.fill: parent
        onClicked: {
          userListBox.shortView = !userListBox.shortView
          if (userListBox.shortView) {
            userListDataRect.yScale = 0
          } else {
            userListDataRect.yScale = 1
          }
        }
      }
    }
    Component {
      id: horizontalDelegate
      Item {
        width: userListBox.width; height: userNameHeight
        Column {
          id: nameColumn
          Rectangle {
            id: userNameRect
            width: userListBox.width; height: userListBox.height
            color: "transparent"
            Text { width: userListBox.width; text: userName }
            MouseArea {
              anchors.fill: parent
              onClicked : selectUser (userName)
            }
          }
        }
      }
    }
    Rectangle {
      id: userListDataRect
      property alias yScale: rollupScale.yScale
      height: userListBox.height - userListCountRect.height
      width: parent.width
      color: Qt.lighter(channelBox.color)
      anchors {top: userListCountRect.bottom; left: parent.left}
      transform: Scale {
        id: rollupScale
        xScale: 1
        yScale: 1
        Behavior  on yScale {
          NumberAnimation { duration: rollDelay }
        }
      }
      ListView {
        id: userList
        anchors.fill: parent
        clip: true
        delegate: horizontalDelegate
      }
    }
  } 
  Component.onCompleted: console.log ("Loaded ChannelBox.qml")
}