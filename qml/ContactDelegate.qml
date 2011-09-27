import QtQuick 1.0

Rectangle {
  id: mainBox
  width: 400
  clip: false
  color: "#eeeecc"
  border.color: Qt.lighter(color); border.width: 2
  property string imageDir: ":/icons/64x64/status/"
  property real imageWidth: 48
  property real imageHeight: 48
  property bool showMyJid: false
  property bool showLogins: false
  property real mainWidth: width
  property bool showOfflines: true
  property real minHeightWhenVisible: 48
  
  height: visible ? Math.max (minHeightWhenVisible,mainFlow.height) : 0
  visible: showOfflines || isOnline
  
  signal clickedImage (string contactJid, string loginJid)
  signal clickedJid (string contactJid, string loginJid)
  signal clickedLogin (string contactJid, string loginJid, string contactResource)
  signal clickedLink (string contactJid, string linkText)
  
  Flow {
    id: mainFlow
    spacing: 3
    Rectangle {
      id: statusImageBox
      width: mainBox.imageWidth; height: mainBox.imageHeight
      color: "transparent"
      Image {
        id: bestStatusImage
        width: mainBox.imageWidth; height: mainBox.imageHeight
        source: imageDir + cppContactModel.imageName (contactBestStatus)
        fillMode: Image.Stretch
      }
      MouseArea {
        anchors.fill: parent
        onClicked: {
          mainBox.clickedImage (contactOtherJid, contactMyJid)
        }
      }
    }

    Rectangle {
      id: loginCountBox
      width: mainBox.imageHeight
      height: mainBox.imageHeight
      color: Qt.lighter (mainBox.color)
      radius: height * 0.3
      Text {
        id: loginCountText
        anchors.centerIn: parent
        text: " (" + contactLoginCount + ") "
      }
      MouseArea {
        anchors.fill: parent
        onPressAndHold: {
          mainBox.showLogins = !mainBox.showLogins
          console.log ("qml showLogins " + mainBox.showLogins)
        }
      }      
    }


    Text {
      id: jidText
      text: contactOtherJid
      MouseArea {
        anchors.fill: parent
        onClicked: {
          mainBox.clickedJid (contactOtherJid, contactMyJid)
        }
        onPressAndHold: {
          mainBox.showMyJid = !mainBox.showMyJid
        }
      }
    }
    Text {
      id: nameText
      text: contactOtherName
    }
    Rectangle {
      id: ownJidBox
      width: 300
      height: mainBox.showMyJid ? 48 : 0
      visible: mainBox.showMyJid
      color: Qt.lighter (mainBox.color)
      Text {
        anchors.centerIn: parent
        width: parent.width
        elide: Text.ElideMiddle
        text: qsTr("from: ") + contactMyJid
      }
    }
    Rectangle {
      id: resoureListBox
      width: resourceListText.width
      height: mainBox.showLogins ? resourceListText.height : 0
      visible: mainBox.showLogins
      color:"white"
      opacity: 0.9
      radius: 4
      z: parent.z + 2
      Text {
        id: resourceListText
        text: resourceList
        onLinkActivated: {
          mainBox.clickedLink (contactOtherJid, link)
          console.log ("clicked link " + link)
        }
      }
    }
  }
}
