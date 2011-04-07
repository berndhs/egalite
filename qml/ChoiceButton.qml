import QtQuick 1.0

Rectangle {
  id: button
  signal clicked ()
  signal pressAndHold ()
  property alias labelText : label.text
  property alias labelHeight: label.height
  property real commonMargin: 4
  width: 100
  height: 100
  radius: 5
  color: "#d3d3d3"
  anchors { 
    topMargin: commonMargin; bottomMargin: commonMargin; 
    leftMargin: commonMargin; rightMargin: commonMargin
  }
  MouseArea {
    anchors.fill: parent
    onClicked: { parent.clicked () }
    onPressAndHold: { parent: pressAndHold () }
  }
  Text { 
    id: label
    text: "Button"
    z: parent.z
    wrapMode:Text.Wrap
    width: parent.width
    anchors.centerIn: parent 
    horizontalAlignment: Text.AlignHCenter
 }
}