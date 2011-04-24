
import QtQuick 1.0

/** \brief Container for a single floating IRC channel */

Rectangle {
  id: ircFloat
  property real channelTopMargin: 0
  height: 400
  width: 600
  color: "#f2f2f5"

  property alias floatName: floatingChannel.objectName
  
  signal hideMe ()
  signal dockMe ()
  
  IrcChannelBox {
    id: floatingChannel
    parentHeightReserve: 0
  }

  Rectangle {
    id: fake
    objectName: "FakeObject"
  }

  Component.onCompleted: {
    console.log ("have float box")
  }
}
