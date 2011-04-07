

import QtQuick 1.0

Rectangle {
  id: channelGroup
  height: 300
  width: 300
  color: "yellow"
  function addChannel () {
    console.log ("  channelGroup add ")
    var compo =  Qt.createComponent("IrcChannelBox.qml")
    console.log (" create returns " + compo + "  status " + compo.status)
    if (compo.status == Component.Error) {
      console.log ("    create error " + compo.errorString())
    }
    if (compo.status == Component.Ready) {
      var newBox = compo.createObject (channelGroup)
      newBox.color = "green"
      return newBox
    }
    return null
  }
  Rectangle {
    id: emptyBox
    width: 200; height: 100
    anchors.centerIn: parent
    Text {
      anchors.centerIn: parent
      text: "empty channel group"
    }
  }
  Component.onCompleted: {
    console.log ("loaded ChannelGroup.qml")
    addChannel ()
  }
}