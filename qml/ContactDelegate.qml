import QtQuick 1.0

Rectangle {
  id: mainBox
  width: 400
  color: "#eeeecc"
  border.color: Qt.lighter(color); border.width: 2
  property string imageDir: ":/icons/64x64/status/"
  property real imageWidth: 48
  property real imageHeight: 48
  property bool showMyJid: false
  property bool showLogins: false
  property real mainWidth: width
  property bool showOfflines: true
  property real heightWhenVisible: 48
  
  height: visible ? heightWhenVisible : 0
  visible: showOfflines || isOnline
  
  signal clickedImage (string contactJid, string loginJid)
  signal clickedJid (string contactJid, string loginJid)
  signal clickedLogin (string contactJid, string loginJid, string contactResource)
  
  Flow {
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

    Text {
      id: loginCountText
      text: " (" + contactLoginCount + ") "
      MouseArea {
        anchors.fill: parent
        onPressAndHold: {
          mainBox.showLogins = !mainBox.showLogins
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
      text: "Name: " + contactOtherName
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
      width: 300
      height: mainBox.showLogins ? 96 : 0
      visible: mainBox.showLogins
      Text {
        id: resourceListText
        text: "resource list goes here"
      }
    }
  }
}
