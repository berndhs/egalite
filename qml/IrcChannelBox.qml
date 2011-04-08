
import QtQuick 1.0

Rectangle {
  id: channelBox
  property real labelHeight:32
  property real countWidth: 0.3 * width
  property alias userListModel: userList.model
  property alias boxLabel: channelBoxLabel.text
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
    anchors { top: historyBox.top; right: historyBox.right }
    height: historyBox.height
    width: countWidth
    Text {
      id: userListCount
      anchors { top: parent.top; left: parent.left }
      height: labelHeight
      width: parent.width
      text: "User Count"
    }
    ListView {
      id: userList
      height: parent.height - userListCount.height
      anchors {top: userListCount.bottom; left: parent.left}
    }
  } 
  Component.onCompleted: console.log ("Loaded ChannelBox.qml")
}