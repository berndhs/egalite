


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
  id: channelGroup
  property real channelTopMargin: 24
  height: 300
  width: 300
  color: "#f0f0f3"

  signal selectedChannel (string link)
  signal changedHeadHeight (int newHeight)

  function addChannel () {
    console.log ("  channelGroup add ")
    var compo =  Qt.createComponent("IrcChannelBox.qml")
    console.log (" create returns " + compo + "  status " + compo.status)
    if (compo.status == Component.Error) {
      console.log ("    create error " + compo.errorString())
    }
    if (compo.status == Component.Ready) {
      var newBox = compo.createObject (channelGroup)
      newBox.parentHeightReserve = channelList.height
      newBox.anchors.top = channelGroup.top
      return newBox
    }
    return null
  }
  function setChannelList (theList) {
    console.log ("Channel Group list " + theList)
    channelListText.text = theList
  }
  Rectangle {
    id: channelList
    width: channelGroup.width
    height: childrenRect.height
    color: "transparent"
    z: channelGroup.z + 2
    anchors {top: channelGroup.top; left: channelGroup.left}
    Text {
      id: channelListText
      anchors {top: parent.top; left: parent.left}
      width: parent.width
      wrapMode: Text.Wrap
      text: ""
      onLinkActivated: {
        console.log (" ChannelGroup link activated " + link)
        channelGroup.selectedChannel (link)
      }
      onHeightChanged: {
        channelGroup.changedHeadHeight (height)
      }
    }
  }
  Rectangle {
    id: emptyBox
    width: 200; height: 100
    anchors.centerIn: parent
    Text {
      anchors.centerIn: parent
      text: qsTr ("No Channels")
    }
  }
  Component.onCompleted: {
    console.log ("loaded IrcChannelGroup")
  }
}
