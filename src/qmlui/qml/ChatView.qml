import QtQuick 1.0
// import Qt.components 1.0

Rectangle {
  id: container

  Component {
      id: msgDelegate
      Item {
        id: msgDelegateItem
        Row {
          id: chatline
          Text { text: timestamp; wrapMode: Text.NoWrap; width: 100 }
          Text { text: sender; wrapMode: Text.NoWrap; width: 100 }
          Text { text: contents; wrapMode: Text.Wrap; width: flickable.width-200}
        }
        height: chatline.height
      }
  }

  ListView {
    id: flickable
    anchors.fill: parent


    model: msgModel

    delegate: msgDelegate

    Connections {
      target: msgModel
      onRowsInserted: flickable.positionViewAtEnd();
    }

    Rectangle {
      id: scrollbar
      anchors.right: flickable.right
      y: flickable.visibleArea.yPosition * flickable.height
      width: 10
      height: flickable.visibleArea.heightRatio * flickable.height
      color: "black"
    }
  }
}
