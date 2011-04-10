

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
import net.sf.egalite 1.0

Rectangle {
  id: channelBox
  property string channelName: qsTr ("no channel")
  property real labelHeight:32
  property real inputHeight: 28
  property real userNameHeight: 20
  property real countWidth: 0.25 * width
  property real rollDelay: 200
  property real parentHeightReserve: 0
  property real parentWidthReserve: 0
  property alias userListModel: userList.model
  property alias boxLabel: channelBoxLabel.text
  property alias userListCounter: userListCount.text
  property alias channelTopic: topicBox.topicText

  signal userSend ()
  signal userUp ()
  signal userDown ()
  signal activatedLink (string link)

  function selectUser (user) {
    console.log ("selected user " + user)
  }
  function deallocateSelf () {
    console.log ("deallocate channelBox")
    channelBox.destroy()
  }
  function setCookedLog (theText) {
    cookedLogBox.setHtml (theText)
    cookedFlickBox.alignBottom ()
    console.log ("  After update text height " + cookedLogBox.height)
  }
  function userData () { return textEnter.text }
  function writeUserData (theText) { textEnter.text = theText }
  function clearUserData () { textEnter.text = "" }

  function setModel (theModel) { userList.model = theModel }
  function cookedBoundingRect () { return cookedLogBox.boundingRect () }

  objectName: "ChannelBox_" + channelName
  height: parent.height - parentHeightReserve
  width: parent.width - parentWidthReserve
  anchors { topMargin: parentHeightReserve }
  color: "#f3f6f6"

  onChannelNameChanged: { 
    objectName = "ChannelBox_" + channelName 
    topicBox.setName (channelName)
  }

  Rectangle {
    id: channelBoxLabelRect
    height: childrenRect.height + 3
    width: childrenRect.width
    anchors { top: parent.top; left: parent.left }
    color: "#ff99aa"
    z: parent.z+1
    MouseArea { 
      anchors.fill: parent
      onClicked: cookedFlickBox.alignBottom()
      onPressAndHold: topicBox.toggleHeight()
    }
    Text {
      anchors { leftMargin: 2; rightMargin: 2; verticalCenter: parent.verticalCenter }
      id: channelBoxLabel
      text: " "
    }
  }
  Rectangle {
    id: topicBox
    property real maxHeight: channelBoxLabelRect.height
    property bool bigHeight: false
    property string topicText: qsTr ("no topic set")
    clip: true
    color: "#f3f6f6"
    border.color: "transparent"
    border.width: 0
    z: cookedFlickBox.z + 1
    anchors { left: channelBoxLabelRect.right; top: parent.top }
    width: parent.width - channelBoxLabelRect.width - countWidth
    height: maxHeight
    Behavior on height  { PropertyAnimation { duration: 1.5*rollDelay } }
    Behavior on color { PropertyAnimation { duration: 1.5*rollDelay } }
    function toggleHeight () {
      bigHeight = !bigHeight
      if (bigHeight)  setBig ()
      else            setSmall ()
    }
    function setBig () {
      maxHeight = topicBoxContent.height
      color = Qt.lighter ("#f3f6f6", 1.9)
    }
    function setSmall () {
      maxHeight = channelBoxLabelRect.height
      color = "#f3f6f6"
    }
    function setName (newName) {
      topicBoxContent.name = newName
    }
    Item {
      anchors { top: parent.top; left: parent.left}
      height: 64 //Math.min (32, childrenRect.height)
      width: parent.width
      IrcTextBrowser {
        id: topicBoxContent
        name: "Topic_" + channelBox.channelName
        onActivatedLink: {
          channelBox.activatedLink (link)
        }
      }
    }
    onWidthChanged: topicBoxContent.setWidth (width)
    onTopicTextChanged: {
      console.log (" topic changed for " + channelBox.objectName + " to " + topicBox.topicText)
      topicBox.setSmall ()
      topicBoxContent.setHtml (topicBox.topicText)
    }
    Component.onCompleted: {
      topicBoxContent.setWidth (topicBox.width) 
      topicBoxContent.setHtml (topicBox.topicText)
      console.log ("Topic box loaded for " + channelBox.objectName)
    }
  }
  Flickable {
    id: cookedFlickBox
    objectName: "CookedFlickBox"
    anchors { top: channelBoxLabelRect.bottom; left: parent.left; leftMargin: 2 }
    width: parent.width-2
    interactive: true
    height: channelBox.height - channelBoxLabelRect.height - textEnterBox.height
    clip: true
    contentWidth: Math.max(parent.width,cookedLogBox.width)
    contentHeight: Math.max(parent.height,cookedLogBox.height)
    boundsBehavior: Flickable.DragAndOvershootBounds

    function alignBottom () {
      if (flicking) return   // not while moving
      contentY = Math.max (0, cookedLogBox.height - cookedFlickBox.height - 2)
      console.log ("cooked box contentY set to " + contentY)
    }
    IrcTextBrowser {
      id: cookedLogBox
      name: "Cooked_" + channelBox.channelName
      onActivatedLink: { 
        channelBox.activatedLink (link)
      }
    }

    onWidthChanged: cookedLogBox.setWidth (width)
    Component.onCompleted: {
      cookedLogBox.setWidth (cookedFlickBox.width)
    }
  }
  Rectangle {
    id: textEnterBox
    height:inputHeight
    width: parent.width
    anchors { left: parent.left; top: cookedFlickBox.bottom }
    color: "#ddffee"
    TextInput {
      id: textEnter
      anchors.fill: parent
      text: ""
      Keys.onEnterPressed: channelBox.userSend ()
      Keys.onReturnPressed: channelBox.userSend ()
      Keys.onUpPressed: channelBox.userUp ()
      Keys.onDownPressed: channelBox.userDown ()
    }
    ChoiceButton {
      id: sendButton
      labelText: qsTr ("Send")
      width: labelWidth
      height: inputHeight -4
      anchors { verticalCenter: parent.verticalCenter; right: parent.right }
      onLabelChanged: { width = labelWidth }
      onClicked: { channelBox.userSend () }
    }
  }
  Rectangle {
    id: userListBox
    color: "transparent"
    anchors { top: channelBox.top; right: channelBox.right }
    height: channelBox.height - textEnterBox.height
    width: countWidth
    property bool shortView: true
    Rectangle {
      id: userListCountRect
      height: childrenRect.height
      width: parent.width
      color: "#00aaff"
      opacity: 0.6666
      Text {
        id: userListCount
        horizontalAlignment: Text.AlignHCenter
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
        //height: labelHeight
        width: parent.width
        text: qsTr("No Users")
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
      property alias xScale: rollupScale.xScale
      height: userListBox.height - userListCountRect.height
      width: parent.width
      color: Qt.lighter(channelBox.color)
      opacity: 0.7
      anchors {top: userListCountRect.bottom; right: parent.right}
      transform: Scale {
        id: rollupScale
        xScale: 1
        yScale: 0
        Behavior  on xScale {
          NumberAnimation { duration: rollDelay }
        }
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