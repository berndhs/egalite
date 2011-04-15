

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
  property string backgroundImage: "qrc:///palee6.png"
  property bool   useBackgroundImage: true
  property alias userListModel: userList.model
  property alias boxLabel: channelBoxLabel.text
  property alias userListCounter: userListCount.text
  property alias channelTopic: topicBox.topicText

  signal userSend ()
  signal userUp ()
  signal userDown ()
  signal activatedLink (string link)
  signal wantPart ()

  function selectUser (user) {
    console.log ("selected user " + user)
  }
  function deallocateSelf () {
    console.log ("deallocate channelBox")
    channelBox.destroy()
  }
  function setCookedLog (theText) {
    cookedLogBox.setHtml (theText)
    if (cookedFlickBox.interactive) {
      cookedFlickBox.alignBottom ()
    }
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

  Image {
    anchors.fill: parent
    fillMode: Image.Tile
    source: backgroundImage
    visible: useBackgroundImage
  }
  Rectangle {
    id: channelBoxLabelRect
    height: childrenRect.height + 3
    width: childrenRect.width
    anchors { top: parent.top; left: parent.left }
    color: "#ff99aa"
    radius: 4
    z: parent.z+1
    MouseArea { 
      anchors.fill: parent
      onClicked: {
        if (channelMenu.isShown) {
          channelMenu.hide ()
        } else {
          channelMenu.show ()
        }
      }
      onDoubleClicked: { cookedFlickBox.interactive = !cookedFlickBox.interactive }
      onPressAndHold: topicBox.toggleHeight()
    }
    Text {
      anchors { leftMargin: 2; rightMargin: 2; verticalCenter: parent.verticalCenter }
      id: channelBoxLabel
      text: " "
    }
  }
  DropMenu {
    id: channelMenu
    color: "transparent"
    itemWidth: childrenRect.width
    itemHeight: 36
    property real buttonRadius: 0.4 * itemHeight
    z: parent.z + 1
    isShown: false
    rollDelay: 75
    anchors { 
      top: channelBoxLabelRect.bottom; 
      left: channelBoxLabelRect.horizontalCenter 
    }
    ChoiceButton {
      id: partButton
      height: parent.itemHeight
      radius: buttonRadius
      anchors {top: parent.top; horizontalCenter: parent.horizontalCenter }
      labelText: qsTr (" Leave Channel ")
      onClicked: { channelBox.wantPart (); channelMenu.hide () }
    }
    ChoiceButton {
      id: tpoicButton
      height: parent.itemHeight
      radius: buttonRadius
      anchors {top: partButton.bottom; horizontalCenter: parent.horizontalCenter }
      labelText: qsTr (" Big Topic ")
      onClicked: { topicBox.toggleHeight(); channelMenu.hide () }
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
      border.color = "#c0c0c0"
      border.width = 1
    }
    function setSmall () {
      maxHeight = channelBoxLabelRect.height
      color = "#f3f6f6"
      border.color = "transparent"
      border.width = 0
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
    contentWidth: cookedLogBox.width
    contentHeight: cookedLogBox.height
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
            userListDataRect.color = channelBox.color
          } else {
            userListDataRect.yScale = 1
            userListDataRect.color = Qt.lighter(channelBox.color, 2.0)
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
            Text { width: userListBox.width; text: name }
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
      color: channelBox.color
      opacity: 0.7
      anchors {top: userListCountRect.bottom; right: parent.right}

      Behavior on color { PropertyAnimation { duration: rollDelay } }
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