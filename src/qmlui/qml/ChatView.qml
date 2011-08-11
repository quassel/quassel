import QtQuick 1.0
// import Qt.components 1.0

Rectangle {
  id: container

  ListView {
    id: chatView
    anchors.fill: parent
    model: msgModel
    delegate: Component {
      ChatLine { }
    }

    Connections {
      target: msgModel
      onRowsInserted: chatView.positionViewAtEnd();
    }

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
