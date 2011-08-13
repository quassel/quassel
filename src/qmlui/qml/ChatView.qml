import QtQuick 1.1
// import Qt.components 1.0

import eu.quassel.qmlui 1.0

Rectangle {
  id: container

  ListView {
    id: chatView
    anchors.fill: parent
    model: msgModel

    delegate: Component {
      ChatLine {
        chatLineData: chatLineDataRole
      }
    }

    //interactive: false
    boundsBehavior: Flickable.StopAtBounds

    property int timestampWidth: 50
    property int senderWidth: 80
    property int contentsWidth: width - timestampWidth - senderWidth - 30;

    Connections {
      target: msgModel
      onRowsInserted: chatView.positionViewAtEnd();
    }
/*
    MouseArea {
      id: mouseArea
      anchors.fill: parent
      acceptedButtons: Qt.LeftButton

      onClicked: {
        console.log("clicked")
        parent.senderWidth = parent.senderWidth + 10
      }
      onPositionChanged: {
        console.log("changed" + mouseX + mouseY)
      }

    }
*/

    Rectangle {
      id: scrollbar
      anchors.right: chatView.right
      y: chatView.visibleArea.yPosition * chatView.height
      width: 10
      height: chatView.visibleArea.heightRatio * chatView.height
      color: "grey"
    }
  }
}
