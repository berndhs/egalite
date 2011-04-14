
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
  signal selectActiveServer (int index)
  signal selectChannel (string name)
  signal selectNick (string name)
  signal join ()
  signal login ();

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
    height: childrenRect.height
    color: "blue"
    border.color: "black"
    ActiveServerList {
      id: activeServerList
      anchors { left: parent.left; top: parent.top }
      nameWidth: 200
      addressWidth: 150
      height: 2.5*rowHeight
      width: activeListBox.width
      model: cppActiveServerModel
      onDisconnectServer: {
        console.log ("disconnect from " + index)
        model.disconnectServer (index)
      }
      onSelectedServer: {
        ircControlBox.selectActiveServer (index)
      }
      Connections {
        target: cppActiveServerModel
        onSelectRow: {
          console.log (" Select Row " + row)
          activeServerList.currentIndex = row
        }
      }
    }
  }

  Rectangle {
    id: channelListBox
    color: "transparent"
    border.color: "red"
    border.width: 1
    width: parent.width * 0.4
    height: 4 * 32
    anchors { top: activeListBox.bottom; left: parent.left }
    Rectangle {
      id: channelHeader
      height: childrenRect.height
      width: parent.width
      color: "#d0d0ff"
      Text {
        horizontalAlignment: Text.AlignHCenter
        text: qsTr ("Channel Names")
      }
    }
    ListView {
      Component {
        id: channelDelegate
        Rectangle {
          id: channelDelegateBox 
          width: channelListBox.width
          height: 32
          color: "transparent"
          MouseArea {
            anchors.fill: parent
            onClicked: { 
              channelList.currentIndex = index; 
              ircControlBox.selectChannel (name) 
            }
          }
          Text {
            width: channelListBox,width; text: name
          }
        }
      }
      id: channelList
      width: parent.width
      height: 3 * 32
      clip: true
      anchors { top: channelHeader.bottom; left: channelListBox.left }
      model: cppChannelListModel
      highlight: Rectangle { color: "#ffcccc" }
      delegate: channelDelegate
    }
  }

  Column {
    id: middleButtons
    anchors { 
      left: channelListBox.right; 
      top: channelListBox.top 
    }
    ChoiceButton {
      id: joinButton 
      labelText: qsTr ("<-- Join")
      radius: 8
      height: 64
      onClicked: {
        ircControlBox.join ()
      }
    }
    ChoiceButton {
      id: loginButton 
      labelText: qsTr ("Login -->")
      radius: 8
      height: 64
      onClicked: {
        ircControlBox.login ()
      }
    }
  }
    
  Rectangle {
    id: nickListBox
    color: "transparent"
    border.color: "blue"
    border.width: 1
    width: parent.width * 0.4
    height: 4 * 32
    anchors { top: activeListBox.bottom; left: middleButtons.right }
    Rectangle {
      id: nickHeader
      height: childrenRect.height
      width: parent.width
      color: "#d0ffd0"
      Text {
        horizontalAlignment: Text.AlignHCenter
        text: qsTr ("Nick Names")
      }
    }
    ListView {
      Component {
        id: nickDelegate
        Rectangle {
          id: nickDelegateBox 
          width: nickListBox.width
          height: 32
          color: "transparent"
          MouseArea {
            height: 32
            anchors.fill: parent
            onClicked:  { 
              nickList.currentIndex = index; 
              ircControlBox.selectNick (name) 
            }
          }
          Text {
            width: nickListBox.width
            text: name
          }
        }
      }
      id: nickList
      width: parent.width
      height: 3 * 32
      clip: true
      anchors { top: nickHeader.bottom; left: nickListBox.left }
      model: cppNickListModel
      delegate: nickDelegate
      highlight: Rectangle { color: "#eeccff" }
    }
  }
    
  Component.onCompleted: {
    console.log ("loaded IrcControlBox")
  }
}
